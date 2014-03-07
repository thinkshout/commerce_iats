<?php
/**
 * @file
 * Implements iATS credit card payment services for use in Drupal Commerce.
 */

/**
 * Payment method callback: checkout form.
 */
function commerce_iats_credit_card_submit_form($payment_method, $pane_values, $checkout_pane, $order) {
  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');
  $payment_method['settings'] += commerce_iats_default_settings();

  // Prepare the fields to include on the credit card form.
  $fields = array(
    'code' => '',
  );

  // Limit credit card types, since iATS has a limited selection.
  $card_types = array_diff(array_values($payment_method['settings']['credit_card']['card_types']), array(0));
  if (!empty($card_types)) {
    if ($card_types === array(0)) {
      $card_types = commerce_iats_credit_card_types();
    }
    $fields['type'] = $card_types;
  }

  return commerce_payment_credit_card_form($fields);
}

/**
 * Payment method callback: checkout form validation.
 */
function commerce_iats_credit_card_submit_form_validate($payment_method, $pane_form, $pane_values, $order, $form_parents = array()) {
  // If the customer specified a card on file, skip the normal validation.
  if (module_exists('commerce_cardonfile') && !empty($payment_method['settings']['iats_customer_code']) &&
    !empty($pane_values['cardonfile']) && ($pane_values['cardonfile'] !== 'new')) {
    return;
  }

  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');

  // Validate the credit card fields.
  $settings = array(
    'form_parents' => array_merge($form_parents, array('credit_card')),
  );

  if (!commerce_payment_credit_card_validate($pane_values['credit_card'], $settings)) {
    return FALSE;
  }
}

/**
 * Payment method callback: checkout form submission.
 */
function commerce_iats_credit_card_submit_form_submit($payment_method, $pane_form, $pane_values, $order, $charge) {
  if (empty($charge['amount'])) {
    watchdog('commerce_iats', 'Empty charge amount: !charge for order !order', array(
      '!charge' => '<pre>' . check_plain(print_r($charge, TRUE)) . '</pre>',
      '!order' => '<pre>' . check_plain(print_r($order, TRUE)) . '</pre>',
    ), WATCHDOG_DEBUG);
  }

  // If the customer specified payment using a card on file, attempt that now and simply return the result.
  if (module_exists('commerce_cardonfile') && $payment_method['settings']['iats_customer_code'] &&
    !empty($pane_values['cardonfile']) && $pane_values['cardonfile'] !== 'new') {
    return commerce_iats_credit_card_customer_code_charge_submit_form_submit($payment_method, $pane_form, $pane_values, $order, $charge);
  }

  // If possible, determine the credit card type for use in later code.
  if (!empty($pane_values['credit_card']['number'])) {
    module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');
    $card_type = commerce_payment_validate_credit_card_type($pane_values['credit_card']['number'], array_keys(commerce_iats_credit_card_types()));
  }

  $payment_method['settings'] += commerce_iats_default_settings();

  // Create and populate the request  with standard properties.
  $request = array(
    // TODO: Invoice number needs to be user-editable.
    'invoiceNum' => t('dc-@number', array('@number' => $order->order_number)),
    'total' => commerce_currency_amount_to_decimal($charge['amount'], $charge['currency_code']),
    'customerIPAddress' => ip_address(),
  );

  // Add the billing information.
  commerce_iats_address($request, 'billing', $order, 'commerce_customer_billing');

  // Add the credit card details.
  $expiry_string = sprintf('%02d/%02d', $pane_values['credit_card']['exp_month'], ($pane_values['credit_card']['exp_year'] % 100));

  // TODO: Figure out what's going on with $iats_method here.
  // TODO: Should blank spaces be removed here? Probably.
  //$request[{$iats_method['params']['cc_num']} = $pane_values['credit_card']['number'];
  //$request[{$iats_method['params']['cc_exp']} = $expiry_string;

  // Add the CVV data, if included.
  if (isset($pane_values['credit_card']['code'])) {
    $request['cvv2'] = $pane_values['credit_card']['code'];
  }

  $request['mop'] = commerce_iats_credit_card_mop($pane_values['credit_card']['type']);

  // Instantiate the iATS ProcessLink class to process credit card payment.
  $iats = new ProcessLink($payment_method['settings']['agent_code'], $payment_method['settings']['agent_password']);
  $response = NULL;

  if ($payment_method['settings']['iats_customer_code']) {
    // Process payment using customer code.
    $response = $iats->createCustomerCodeAndProcessCreditCard($request);
  }
  else {
    // Process payment without using customer code.
    $response = $iats->processCreditCard($request);
  }

  if ($response != NULL)
  {
    $auth_result = $response['AUTHORIZATIONRESULT'];

    $customer_code = (isset($response['CUSTOMERCODE']))? $response['CUSTOMERCODE'] : NULL;

    $transaction = commerce_payment_transaction_new('iats_credit_card', $order->order_id);
    $transaction->instance_id = $payment_method['instance_id'];
    $transaction->remote_id = trim($response['TRANSACTIONID']) . '|' . $customer_code;
    $transaction->amount = $charge['amount'];
    $transaction->currency_code = $charge['currency_code'];

    if (substr($auth_result, 0, 2) != IATS_TXN_OK) {
      // Create a failed transaction with the error message.
      $transaction->status = COMMERCE_PAYMENT_STATUS_FAILURE;
    }
    else {
      $transaction->status = COMMERCE_PAYMENT_STATUS_SUCCESS;
    }

    // Build a meaningful response message.
    $message = array(
      '<b>' . t('Credit Card Authorization and Capture') . '</b>',
      '<b>' . t('@decision:', array('@decision' => $auth_result)) . '</b>',
    );

    $transaction->message = implode('<br />', $message);

    // Save the transaction information.
    commerce_payment_transaction_save($transaction);

    // If the payment failed, display an error and rebuild the form.
    if (substr($auth_result, 0, 2) != IATS_TXN_OK) {
      drupal_set_message(t('We received the following error while attempting to process your card: @reason', array('@reason' => $auth_result)), 'error');
      return FALSE;
    }

    // Handle Card on File storage, if enabled.
    if (module_exists('commerce_cardonfile') && !empty($payment_method['settings']['iats_customer_code']) &&
      !empty($pane_values['credit_card']['cardonfile_store']) && $pane_values['credit_card']['cardonfile_store'] && $customer_code) {
      commerce_iats_credit_card_save_card_on_file($order, $payment_method, $pane_values, $customer_code);
    }
  }
  else
  {
    drupal_set_message(t('We encountered an error contacting our payment processor. Please try submitting your payment again.'), 'error');
    return FALSE;
  }
}

/**
 * Imitates the checkout form submission callback for the CustomerCode payment method.
 */
function commerce_iats_credit_card_customer_code_charge_submit_form_submit($payment_method, $pane_form, $pane_values, $order, $charge) {
  // First attempt to load the card on file.
  $card_data = commerce_cardonfile_load($pane_values['cardonfile']);

  // Fail now if it is no longer available or the card is inactive.
  if (empty($card_data) || $card_data->status == 0) {
    drupal_set_message(t('The requested card on file is no longer valid.'), 'error');
    return FALSE;
  }

  return commerce_iats_credit_card_cardonfile_charge($payment_method, $card_data, $order, $charge);
}

function commerce_iats_credit_card_cardonfile_charge($payment_method, $card_data, $order, $charge) {
  $payment_method['settings'] += commerce_iats_default_settings();

  // Create and populate the request object.
  $request = array(
    'invoiceNum' => t('dc-@number', array('@number' => $order->order_number)),
    'total' => commerce_currency_amount_to_decimal($charge['amount'], $charge['currency_code']),
    'customerIPAddress' => ip_address(),
    'customerCode' => $card_data->remote_id,
    'comment' => '',
  );

  // Add the CVV data, if included.
  if (isset($pane_values['credit_card']['code'])) {
    $request['cvv2'] = $pane_values['credit_card']['code'];
  }

  $request['mop'] = commerce_iats_credit_card_mop($pane_values['credit_card']['type']);

  // Instantiate the iATS ProcessLink class to process credit card payment.
  $iats = new ProcessLink($payment_method['settings']['agent_code'], $payment_method['settings']['agent_password']);
  $response = $iats->processCreditCardWithCustomerCode($request);

  if ($response != NULL)
  {
    $auth_result = $response['AUTHORIZATIONRESULT'];

    // Prepare a transaction object to log the API response.
    $transaction = commerce_payment_transaction_new('iats_credit_card', $order->order_id);
    $transaction->instance_id = $payment_method['instance_id'];
    $transaction->remote_id = trim($response['TRANSACTIONID']) . '|' . $card_data->remote_id;
    $transaction->amount = $charge['amount'];
    $transaction->currency_code = $charge['currency_code'];

    if (substr($auth_result, 0, 2) != IATS_TXN_OK) {
      // Create a failed transaction with the error message.
      $transaction->status = COMMERCE_PAYMENT_STATUS_FAILURE;
    }
    else {
      $transaction->status = COMMERCE_PAYMENT_STATUS_SUCCESS;
    }

    // Build a meaningful response message.
    $message = array(
      '<b>' . t('Credit Card Authorization and Capture using Customer Code') . '</b>',
      '<b>' . t('@decision:', array('@decision' => $auth_result)) . '</b>',
    );

    $transaction->message = implode('<br />', $message);

    // Save the transaction information.
    commerce_payment_transaction_save($transaction);

    // If the payment failed, display an error and rebuild the form.
    if (substr($auth_result, 0, 2) != IATS_TXN_OK) {
      drupal_set_message(t('We received the following error while attempting to process your card: @reason', array('@reason' => $auth_result)), 'error');
      return FALSE;
    }
  }
}

/**
 * Creates a new card on file record for the user making this transaction.
 *
 * @param $order
 * @param $payment_method
 * @param $pane_values
 * @param $customer_code
 */
function commerce_iats_credit_card_save_card_on_file($order, $payment_method, $pane_values, $customer_code)
{
  // Build a payment details array for the credit card.
  $payment_details = array(
    'cardNumber' => $pane_values['credit_card']['number'],
    'expirationDate' => $pane_values['credit_card']['exp_year'] . '-' . $pane_values['credit_card']['exp_month'],
  );

  if (isset($pane_values['credit_card']['code'])) {
    $payment_details['cardCode'] = $pane_values['credit_card']['code'];
  }

  $card_data = commerce_cardonfile_new();
  $card_data->uid = $order->uid;
  $card_data->payment_method = $payment_method['method_id'];
  $card_data->instance_id = $payment_method['instance_id'];
  $card_data->remote_id = $customer_code;
  $card_data->card_type = !empty($card_type) ? $card_type : 'card';
  $card_data->card_name = !empty($billing_address['name_line']) ? $billing_address['name_line'] : '';
  $card_data->card_number = substr($pane_values['credit_card']['number'], -4);
  $card_data->card_exp_month = $pane_values['credit_card']['exp_month'];
  $card_data->card_exp_year = $pane_values['credit_card']['exp_year'];
  $card_data->status = 1;

  // Save and log the creation of the new card on file.
  commerce_cardonfile_save($card_data);
  watchdog('commerce_iats', 'iATS Customer Code @customer_code created and saved to user @uid.', array('@customer_code' => (string) $customer_code, '@uid' => $order->uid));
}

/**
 * Replaces the standard function commerce_payment_credit_card_types() with custom types.
 *
 * @return array Array of credit card types.
 */
function commerce_iats_credit_card_types() {
  return array(
    'visa' => t('Visa'),
    'mastercard' => t('MasterCard'),
    'amex' => t('American Express'),
    'discover' => t('Discover Card'),
  );
}

/**
 * Converts a Commerce credit card type to  an iATS-compatible credit card type.
 *
 * @param string $type The Commerce credit card type.
 * @return string The iATS credit card type.
 */
function commerce_iats_credit_card_mop($type) {
  $mop = array(
    'visa' => 'VISA',
    'mastercard' => 'MC',
    'amex' => 'AMX',
    'discover' => 'DSC',
  );
  return $mop[$type];
}