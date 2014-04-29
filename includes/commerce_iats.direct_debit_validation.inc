<?php
/**
 * @file
 * Implements iATS Payments direct debit payment validation for use in Drupal Commerce.
 */

/**
 * Payment method callback: settings form.
 */
function commerce_iats_direct_debit_validation_settings_form($settings = NULL) {
  $form = commerce_iats_settings_form_base($settings);

  return $form;
}

/**
 * Payment method callback: checkout form.
 */
function commerce_iats_direct_debit_validation_submit_form($payment_method, $pane_values, $checkout_pane, $order) {
  module_load_include('inc', 'commerce_iats', 'includes/commerce_iats.direct_debit.forms');

  $fields = array(
    'type' => '',
    'number' => '',
  );

  return commerce_iats_direct_debit_validation_form($fields);
}

/**
 * Payment method callback: checkout form validation.
 */
function commerce_iats_direct_debit_validation_submit_form_validate($payment_method, $pane_form, $pane_values, $order, $form_parents = array()) {
  module_load_include('inc', 'commerce_iats', 'includes/commerce_iats.direct_debit.forms');

  // Validate the bank account fields.
  $settings = array(
    'form_parents' => array_merge($form_parents, array('bank_account')),
  );

  if (!commerce_iats_direct_debit_validation_payment_form_validate($pane_values['bank_account'], $settings)) {
    return FALSE;
  }
  return TRUE;
}

/**
 * Payment method callback: checkout form submission.
 */
function commerce_iats_direct_debit_validation_submit_form_submit($payment_method, $pane_form, $pane_values, $order, $charge) {
  $payment_data = $pane_values;

  return commerce_iats_process_payment($payment_method, $payment_data, $order,
    $charge, 'commerce_iats_validate_direct_debit_payment');
}

/**
 * Validates a direct debit payment via the iATS Payments API.
 *
 * @param array $payment_method
 *   The payment method used during this transaction.
 * @param array $payment_data
 *   Payment data retrieved from the submit form.
 * @param object $order
 *   The order processed during this transaction.
 * @param array $charge
 *   The transaction charge.
 *
 * @return Mixed
 *   API response array or error string.
 */
function commerce_iats_validate_direct_debit_payment($payment_method, $payment_data, $order, $charge) {
  $combined_address = $payment_data['payer']['address_1'] . "\n"
    . $payment_data['payer']['address_2'] . "\n"
    . $payment_data['payer']['address_3'];

  // Create and populate the API request.
  $request = array(
    'customerIPAddress' => ip_address(),
    'ACHEFTReferenceNum' => '',
    'beginDate' => $payment_data['account']['first_collection_date'],
    // TODO: Review this: End a year after start date.
    'endDate' => ($payment_data['account']['first_collection_date'] + (86400 * 365)),
    'accountCustomerName' => $payment_data['account']['account_holder'],
    'accountNum' => $payment_data['account']['number'],
    'companyName' => $payment_data['payer']['company_name'],
    'firstName' => $payment_data['payer']['first_name'],
    'lastName' => $payment_data['payer']['last_name'],
    'address' => $combined_address,
    'city' => $payment_data['payer']['town'],
    'state' => $payment_data['payer']['county'],
    // TODO: Implement country select, default to UK for now.
    'country' => 'UK',
    'email' => $payment_data['payer']['email'],
    'zipCode' => $payment_data['payer']['postcode'],
  );

  // Get the API service.
  $iats = commerce_iats_get_service('CustomerLink', $payment_method);
  if ($iats == NULL) {
    drupal_set_message(t('Unable to load iATS Payments library.'), 'error');
    return NULL;
  }

  // Log the request.
  if ($payment_method['settings']['log']['request'] === 'request') {
    commerce_iats_log_request($payment_method, 'commerce_iats_validate_direct_debit_payment', $request);
  }

  // Validate the direct debit payment.
  $response = $iats->directDebitACHEFTPayerValidate($request);

  return $response;
}
