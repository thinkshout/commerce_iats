<?php
/**
 * @file
 * Implements iATS Payments credit card services for use in Drupal Commerce.
 */

/**
 * Payment method callback: settings form.
 */
function commerce_iats_credit_card_settings_form($settings = NULL) {
  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');

  $form = commerce_iats_credit_card_settings_form_base($settings);

  $form['invoice_prefix'] = array(
    '#type' => 'textfield',
    '#title' => t('Invoice Prefix'),
    '#description' => t('Invoice numbers consist of this value followed by the order number. ie. INVOICE-1, INVOICE-2'),
    '#default_value' => 'INVOICE-',
    '#required' => TRUE,
  );

  // customerCode support requires the Card on File module.
  if (module_exists('commerce_cardonfile')) {
    $form['iats_customer_code'] = array(
      '#type' => 'checkbox',
      '#title' => t('Enable management of customer codes.'),
      '#description' => t('This will save credit card data on the iATS server'),
    );
  }

  return $form;
}

/**
 * Payment method callback: checkout form.
 */
function commerce_iats_credit_card_submit_form($payment_method, $pane_values, $checkout_pane, $order) {
  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');

  // Prepare the fields to include on the credit card form.
  $fields = array(
    'code' => '',
  );

  // Limit credit card types, since iATS has a limited selection.
  $card_types = array_diff(array_values($payment_method['settings']['credit_card']['card_types']), array(0));
  if (count($card_types) == 0) {
    $card_types = array_keys(commerce_iats_credit_card_types());
  }

  $fields['type'] = $card_types;

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

  $using_customer_code = FALSE;

  // Determine if the customer is using an exsting card on file (customer code.)
  if (module_exists('commerce_cardonfile') && $payment_method['settings']['iats_customer_code'] &&
    !empty($pane_values['cardonfile']) && $pane_values['cardonfile'] !== 'new') {
    $using_customer_code = TRUE;
  }

  // If possible, determine the credit card type for use in later code.
  $card_type = NULL;
  if (!empty($pane_values['credit_card']['number'])) {
    module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');
    $card_type = commerce_payment_validate_credit_card_type($pane_values['credit_card']['number'], array_keys(commerce_iats_credit_card_types()));
  }

  // Create and populate the request with standard properties.
  $request = array(
    'customerIPAddress' => ip_address(),
    'invoiceNum' => t('@prefix@number', array(
      '@prefix' => $payment_method['settings']['invoice_prefix'],
      '@number' => $order->order_number,
    )),
    'cvv2' => $pane_values['credit_card']['code'],
    'total' => commerce_currency_amount_to_decimal($charge['amount'], $charge['currency_code']),
    'currency' => $charge['currency_code'],
  );

  if ($card_type != NULL) {
    $request['mop'] = commerce_iats_credit_card_mop($card_type);
  }

  // Add the billing information.
  commerce_iats_address($request, $order, 'commerce_customer_billing');

  // Add the credit card details.
  $expiry_string = sprintf('%02d/%02d', $pane_values['credit_card']['exp_month'], ($pane_values['credit_card']['exp_year'] % 100));

  $iats = commerce_iats_get_processlink_service($payment_method);
  if ($iats == NULL) {
    drupal_set_message(t('Unable to load iATS Payments library.'), 'error');
    return FALSE;
  }

  $response = NULL;

  if ($using_customer_code) {
    // Process payment using an existing customer code.
    $card_data = commerce_cardonfile_load($pane_values['cardonfile']);

    // Fail now if it is no longer available or the card is inactive.
    if (empty($card_data) || $card_data->status == 0) {
      drupal_set_message(t('The requested card on file is no longer valid.'), 'error');
      return FALSE;
    }

    $request['customerCode'] = $card_data->remote_id;
    $request['mop'] = commerce_iats_credit_card_mop($card_data->card_type);

    $response = $iats->processCreditCardWithCustomerCode($request);
  }
  elseif ($payment_method['settings']['iats_customer_code']) {
    // Process payment and create a new customer code.
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

  if ($payment_method['settings']['log']['request'] === 'request') {
    commerce_iats_log_request($payment_method, $request);
  }

  if ($response != NULL) {
    if ($payment_method['settings']['log']['response'] === 'response') {
      commerce_iats_log_response($payment_method, $response);
    }

    $auth_result = (isset($response['AUTHORIZATIONRESULT'])) ? trim($response['AUTHORIZATIONRESULT']) : NULL;
    $transaction_id = (isset($response['TRANSACTIONID'])) ? trim($response['TRANSACTIONID']) : NULL;

    if ($using_customer_code) {
      $customer_code = $card_data->remote_id;
    }
    else {
      $customer_code = (isset($response['CUSTOMERCODE'])) ? trim($response['CUSTOMERCODE']) : NULL;
    }

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
      '<b>' . t('@decision', array('@decision' => $auth_result)) . '</b>',
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
  else {
    drupal_set_message(t('We encountered an error contacting our payment processor. Please try submitting your payment again.'), 'error');
    return FALSE;
  }
}