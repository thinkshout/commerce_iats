<?php
/**
 * @file
 * Implements iATS Payments credit card customer code services for use in Drupal Commerce.
 */

/**
 * Payment method callback: settings form.
 */
function commerce_iats_credit_card_customer_code_settings_form($settings = NULL) {
  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');

  $form = commerce_iats_credit_card_settings_form_base($settings);

  // No invoice when creating customer codes.
  unset($form['invoice_prefix']);

  return $form;
}

/**
 * Payment method callback: checkout form.
 */
function commerce_iats_credit_card_customer_code_submit_form($payment_method, $pane_values, $checkout_pane, $order) {
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

  $form = commerce_payment_credit_card_form($fields);
  return $form;
}

/**
 * Payment method callback: checkout form submission.
 */
function commerce_iats_credit_card_customer_code_submit_form_submit($payment_method, $pane_form, $pane_values, $order) {
  $payment_data = $pane_values;

  return commerce_iats_process_payment($payment_method, $payment_data, $order,
    NULL, 'commerce_iats_create_customer_code');
}

/**
 * Creates a customer code and associated card on file record via the
 * iATS Payments API.
 *
 * @param $payment_method
 * @param $payment_data
 * @param $order
 * @param $charge
 */
function commerce_iats_create_customer_code($payment_method, $payment_data, $order, $charge) {
  // Format credit card data.
  $formatted_card_data = commerce_iats_format_credit_card_data($payment_data['credit_card']);

  // Create and populate the API request.
  $request = array(
    'customerIPAddress' => ip_address(),
    'mop' => $formatted_card_data['type'],
    'creditCardNum' => $formatted_card_data['number'],
    'creditCardExpiry' => $formatted_card_data['expiry'],
    'cvv2' => $payment_data['credit_card']['code'],
    'recurring' => FALSE, // Only used for recurring payments.
    'beginDate' => 0, // Only used for recurring payments.
    'endDate' => 0, // Only used for recurring payments.
    'currency' => 'USD',
  );

  // Add the billing information.
  commerce_iats_address($request, $order, 'commerce_customer_billing');

  // Get the API service.
  $iats = commerce_iats_get_service('CustomerLink', $payment_method);
  if ($iats == NULL) {
    drupal_set_message(t('Unable to load iATS Payments library.'), 'error');
    return NULL;
  }

  // Log the request.
  if ($payment_method['settings']['log']['request'] === 'request') {
    commerce_iats_log_request($payment_method, 'commerce_iats_create_customer_code', $request);
  }

  $response = $iats->createCreditCardCustomerCode($request);

  if ($response != NULL) {
    if (isset($response['CUSTOMERCODE'])) {
      $customer_code = trim($response['CUSTOMERCODE']);
      // Create a new card on file record for the new customer code.
      commerce_iats_cardonfile_create_credit_card($order, $payment_method,
        $payment_data['credit_card'], $customer_code, $formatted_card_data['type']);
    }
    else {
      drupal_set_message(t('Your credit card could not be stored for later use.'), 'error');
    }
  }

  return $response;
}