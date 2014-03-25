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

  $form['invoice_prefix'] = array(
    '#type' => 'textfield',
    '#title' => t('Invoice Prefix'),
    '#description' => t('Invoice numbers consist of this value followed by the order number. ie. INVOICE-1, INVOICE-2'),
    '#default_value' => (isset($settings['invoice_prefix']))? $settings['invoice_prefix'] : 'INVOICE-',
    '#required' => TRUE,
  );

  return $form;
}

/**
 * Payment method callback: checkout form.
 */
function commerce_iats_cardonfile_credit_card_submit_form($payment_method, $pane_values, $checkout_pane, $order) {
  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');

  xdebug_print_function_stack();

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
function commerce_iats_cardonfile_credit_card_submit_form_validate($payment_method, $pane_form, $pane_values, $order, $form_parents = array()) {
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
 * @param $payment_method
 * @param $pane_form
 * @param $pane_values
 * @param $order
 * @param $charge
 * @return bool
 */
function commerce_iats_cardonfile_credit_card_submit_form_submit($payment_method, $pane_form, $pane_values, $order, $charge) {
  // Determine if the customer is using an exsting card on file.
  if ($payment_method['settings']['iats_customer_code']
    && !empty($pane_values['cardonfile'])
  ) {
    if ($pane_values['cardonfile'] === 'new') {
      // Create a new card on file record.
      $payment_data = $pane_values;

      commerce_iats_process_payment($payment_method, $payment_data, $order, $charge, 'commerce_iats_cardonfile_credit_card_create_and_charge');
    } else {
      // Get the card on file data.
      $card_data = commerce_cardonfile_load($pane_values['cardonfile']);

      // Fail now if it is no longer available or the card is inactive.
      if (empty($card_data) || $card_data->status == 0) {
        drupal_set_message(t('The requested card on file is no longer valid.'), 'error');
        return FALSE;
      }

      $card_data->code = $pane_values['credit_card']['code'];

      commerce_iats_process_payment($payment_method, $card_data, $order, $charge, 'commerce_iats_cardonfile_credit_card_charge');
    }
  }
}

/**
 * @param $payment_method
 * @param $card_data
 * @param $order
 * @param null $charge
 * @return null
 */
function commerce_iats_cardonfile_credit_card_charge($payment_method, $card_data, $order, $charge = NULL) {
  // Create and populate the request with standard properties.
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
    commerce_iats_log_request($payment_method, 'commerce_iats_cardonfile_credit_card_charge', $request);
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
    'creditCardNum' => $formatted_card_data['number'],
    'creditCardExpiry' => $formatted_card_data['expiry'],
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
    commerce_iats_log_request($payment_method, 'commerce_iats_cardonfile_credit_card_create_and_charge', $request);
  }

  $response = $iats->createCustomerCodeAndProcessCreditCard($request);

  return $response;
}

/**
 * @param $form
 * @param $form_state
 * @param $payment_method
 * @param $card_data
 */
function commerce_iats_cardonfile_credit_card_update($form, &$form_state, $payment_method, $card_data) {

}

/**
 * @param $form
 * @param $form_state
 * @param $payment_method
 * @param $card_data
 */
function commerce_iats_cardonfile_credit_card_delete($form, &$form_state, $payment_method, $card_data) {

}