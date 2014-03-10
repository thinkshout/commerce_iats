<?php
/**
 * @file
 * Implements iATS credit card payment services for use in Drupal Commerce.
 */

/**
 * Payment method callback: settings form.
 */
function commerce_iats_credit_card_settings_form($settings = NULL) {
  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');
  $settings += commerce_iats_default_settings();

  $form = commerce_iats_credit_card_settings_form_base($settings);

  // customerCode support requires the Card on File module.
  if (module_exists('commerce_cardonfile')) {
    $form['iats_customer_code'] = array(
      '#type' => 'checkbox',
      '#title' => t('Enable management of customer codes.'),
      '#description' => t('This will save credit card data on the iATS server'),
      '#default_value' => $settings['iats_customer_code'],
    );
  }

  return $form;
}

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

  // TODO: Refactor this.
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
    'customerIPAddress' => ip_address(),
    // TODO: Invoice number needs to be user-editable.
    'invoiceNum' => t('dc-@number', array('@number' => $order->order_number)),
    'mop' => commerce_iats_credit_card_mop($card_type),
    'cvv2' => $pane_values['credit_card']['code'],
    'total' => commerce_currency_amount_to_decimal($charge['amount'], $charge['currency_code']),
    'currency' => $charge['currency_code'],
  );

  // Add the billing information.
  commerce_iats_address($request, $order, 'commerce_customer_billing');

  // Add the credit card details.
  $expiry_string = sprintf('%02d/%02d', $pane_values['credit_card']['exp_month'], ($pane_values['credit_card']['exp_year'] % 100));

  $iats = commerce_iats_get_processlink_service($payment_method);
  if ($iats == NULL)
  {
    drupal_set_message(t('Unable to load iATS Payments library.'), 'error');
    return FALSE;
  }

  $response = NULL;

  if ($payment_method['settings']['iats_customer_code']) {
    // Process payment using customer code.

    $request['ccNum'] = str_replace(' ', '', $pane_values['credit_card']['number']);
    $request['ccExp'] = $expiry_string;

    $response = $iats->createCustomerCodeAndProcessCreditCard($request);
  }
  else {
    // Process payment without using customer code.

    $request['creditCardNum'] = str_replace(' ', '', $pane_values['credit_card']['number']);
    $request['creditCardExpiry'] = $expiry_string;

    $response = $iats->processCreditCard($request);
  }

  if ($response != NULL) {
    $auth_result = (isset($response['AUTHORIZATIONRESULT']))? trim($response['AUTHORIZATIONRESULT']) : NULL;
    $transaction_id = (isset($response['TRANSACTIONID']))? trim($response['TRANSACTIONID']) : NULL;
    $customer_code = (isset($response['CUSTOMERCODE']))? trim($response['CUSTOMERCODE']) : NULL;

    if ($auth_result == NULL) {
      $auth_result = $response;
    }

    // Prepare a transaction object to log the API response.
    $transaction = commerce_payment_transaction_new('iats_credit_card', $order->order_id);
    $transaction->instance_id = $payment_method['instance_id'];
    $transaction->remote_id = $transaction_id . '|' . $customer_code;
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
      commerce_iats_credit_card_save_card_on_file($order, $payment_method, $pane_values, $customer_code, $card_type);
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
    'customerIPAddress' => ip_address(),
    'customerCode' => $card_data->remote_id,
    'invoiceNum' => t('dc-@number', array('@number' => $order->order_number)),
    'mop' => commerce_iats_credit_card_mop($card_data->card_type),
    'total' => commerce_currency_amount_to_decimal($charge['amount'], $charge['currency_code']),
    'comment' => '',
    'currency' => $charge['currency_code'],
  );

  $iats = commerce_iats_get_processlink_service($payment_method);
  if ($iats == NULL)
  {
    drupal_set_message(t('Unable to load iATS Payments library.'), 'error');
    return FALSE;
  }

  $response = $iats->processCreditCardWithCustomerCode($request);

  if ($response != NULL) {
    $auth_result = (isset($response['AUTHORIZATIONRESULT']))? trim($response['AUTHORIZATIONRESULT']) : NULL;
    $transaction_id = (isset($response['TRANSACTIONID']))? trim($response['TRANSACTIONID']) : NULL;

    if ($auth_result == NULL) {
      $auth_result = $response;
    }

    // Prepare a transaction object to log the API response.
    $transaction = commerce_payment_transaction_new('iats_credit_card', $order->order_id);
    $transaction->instance_id = $payment_method['instance_id'];
    $transaction->remote_id = $transaction_id . '|' . $card_data->remote_id;
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