<?php
/**
 * @file
 * Implements iATS Payments credit card services for use in Drupal Commerce
 * with Commerce Card on File.
 */

/**
 * Payment method callback: settings form.
 */
function commerce_iats_cardonfile_credit_card_settings_form($settings = NULL) {
  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');

  $form = commerce_iats_credit_card_settings_form_base($settings);

  return $form;
}

/**
 * Payment method callback: checkout form.
 */
function commerce_iats_cardonfile_credit_card_submit_form($payment_method, $pane_values, $checkout_pane, $order) {
  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');

  // Prepare the fields to include on the credit card form.
  $fields = array(
    'code' => '',
  );

  // Limit credit card types, since iATS has a limited selection.
  $card_types = array_diff(
    array_values($payment_method['settings']['credit_card']['card_types']), array(0));
  if (count($card_types) == 0) {
    $card_types = array_keys(commerce_iats_credit_card_types());
  }

  $fields['type'] = $card_types;

  return commerce_payment_credit_card_form($fields);
}

/**
 * Payment method callback: checkout form validation.
 */
function commerce_iats_cardonfile_credit_card_submit_form_validate($payment_method, $pane_form, $pane_values, $order, $form_parents = array()) {
  // Skip normal validation if the customer specified a card on file.
  if (!empty($pane_values['cardonfile']) && ($pane_values['cardonfile'] !== 'new')) {
    return TRUE;
  }

  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');

  // Validate the credit card fields.
  $settings = array(
    'form_parents' => array_merge($form_parents, array('credit_card')),
  );

  if (!commerce_payment_credit_card_validate($pane_values['credit_card'], $settings)) {
    return FALSE;
  }
  return TRUE;
}

/**
 * Payment method callback: checkout form submission.
 */
function commerce_iats_cardonfile_credit_card_submit_form_submit($payment_method, $pane_form, $pane_values, $order, $charge) {
  // Validate credit card charge.
  if (empty($charge['amount'])) {
    watchdog('commerce_iats', 'Empty charge amount: @charge for order @order', array(
      '@charge' => '<pre>' . check_plain(print_r($charge, TRUE)) . '</pre>',
      '@order' => '<pre>' . check_plain(print_r($order, TRUE)) . '</pre>',
    ), WATCHDOG_DEBUG);
    return FALSE;
  }

  $payment_data = $pane_values;

  if ($pane_values['cardonfile'] === 'new') {
    if ($pane_values['credit_card']['cardonfile_store']) {
      // Create a new card on file record.
      return commerce_iats_process_payment($payment_method, $payment_data, $order, $charge, 'commerce_iats_cardonfile_credit_card_create_and_charge');
    }
    else {
      // Customer has opted not to use card on file.
      module_load_include('inc', 'commerce_iats', 'includes/commerce_iats.credit_card');
      return commerce_iats_process_payment($payment_method, $payment_data, $order, $charge, 'commerce_iats_process_credit_card_payment');
    }
  }
  else {
    // Get the card on file data.
    $card_data = commerce_cardonfile_load($pane_values['cardonfile']);

    // Fail now if it is no longer available or the card is inactive.
    if (empty($card_data) || $card_data->status == 0) {
      drupal_set_message(t('The requested card on file is no longer valid.'), 'error');
      return FALSE;
    }

    $card_data->code = $pane_values['credit_card']['code'];

    return commerce_iats_process_payment($payment_method, $card_data, $order, $charge, 'commerce_iats_cardonfile_credit_card_charge');
  }
}

/**
 * Processes a credit card payment using a stored card on file via the
 * iATS Payments API.
 *
 * @param $payment_method
 * @param $card_data
 * @param $order
 * @param null $charge
 * @return null
 */
function commerce_iats_cardonfile_credit_card_charge($payment_method, $card_data, $order, $charge = NULL) {
  // Create and populate the API request.
  $request = array(
    'customerIPAddress' => ip_address(),
    'customerCode' => $card_data->remote_id,
    'invoiceNum' => t('@prefix@number', array(
      '@prefix' => $payment_method['settings']['invoice_prefix'],
      '@number' => $order->order_number,
    )),
    'cvv2' => $card_data->code,
    'total' => commerce_currency_amount_to_decimal($charge['amount'], $charge['currency_code']),
    'currency' => $charge['currency_code'],
  );

  // Get the API service.
  $iats = commerce_iats_get_service('ProcessLink', $payment_method);
  if ($iats == NULL) {
    drupal_set_message(t('Unable to load iATS Payments library.'), 'error');
    return NULL;
  }

  // Log the request.
  if ($payment_method['settings']['log']['request'] === 'request') {
    commerce_iats_log_request($payment_method,
      'commerce_iats_cardonfile_credit_card_charge', $request);
  }

  $response = $iats->processCreditCardWithCustomerCode($request);

  return $response;
}

/**
 * @param $payment_method
 * @param $payment_data
 * @param $order
 * @param null $charge
 * @return null
 */
function commerce_iats_cardonfile_credit_card_create_and_charge($payment_method, $payment_data, $order, $charge = NULL) {
  // Format credit card data.
  $formatted_card_data = commerce_iats_format_credit_card_data($payment_data['credit_card']);

  // Create and populate the API request.
  $request = array(
    'customerIPAddress' => ip_address(),
    'invoiceNum' => t('@prefix@number', array(
      '@prefix' => $payment_method['settings']['invoice_prefix'],
      '@number' => $order->order_number,
    )),
    'ccNum' => $formatted_card_data['number'],
    'ccExp' => $formatted_card_data['expiry'],
    'cvv2' => $payment_data['credit_card']['code'],
    'mop' => $formatted_card_data['type'],
    'total' => commerce_currency_amount_to_decimal($charge['amount'], $charge['currency_code']),
    'currency' => $charge['currency_code'],
  );

  // Add the billing information.
  commerce_iats_address($request, $order, 'commerce_customer_billing');

  // Get the API service.
  $iats = commerce_iats_get_service('ProcessLink', $payment_method);
  if ($iats == NULL) {
    drupal_set_message(t('Unable to load iATS Payments library.'), 'error');
    return NULL;
  }

  // Log the request.
  if ($payment_method['settings']['log']['request'] === 'request') {
    commerce_iats_log_request($payment_method,
      'commerce_iats_cardonfile_credit_card_create_and_charge', $request);
  }

  // Create new customer code and process the payment.
  $response = $iats->createCustomerCodeAndProcessCreditCard($request);

  if ($response != NULL) {
    if (isset($response['CUSTOMERCODE'])) {
      $customer_code = trim($response['CUSTOMERCODE']);
      // Create a new card on file record for the new customer code.
      commerce_iats_cardonfile_create_credit_card($order, $payment_method,
        $payment_data['credit_card'], $customer_code, $formatted_card_data['orig_type']);
    }
    else {
      drupal_set_message(t('Your credit card could not be stored for later use.'), 'error');
    }
  }

  return $response;
}

/**
 * Updates credit card details stored with iATS Payments.
 *
 * @param $form
 * @param $form_state
 * @param $payment_method
 * @param $card_data
 */
function commerce_iats_cardonfile_credit_card_update($form, &$form_state, $payment_method, $card_data) {
  // TODO: Populate method.
}

/**
 * Deletes credit card details stored with iATS Payments.
 *
 * @param $form
 * @param $form_state
 * @param $payment_method
 * @param $card_data
 */
function commerce_iats_cardonfile_credit_card_delete($form, &$form_state, $payment_method, $card_data) {
  // TODO: Populate method.
}