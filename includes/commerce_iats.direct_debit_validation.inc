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
  // Create and populate the API request.
  $request = array(
    'customerIPAddress' => ip_address(),
    'accountNum' => $payment_data['bank_account']['number'],
    'total' => commerce_currency_amount_to_decimal($charge['amount'], $charge['currency_code']),
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
    commerce_iats_log_request($payment_method, 'commerce_iats_validate_direct_debit_payment', $request);
  }

  // Validate the direct debit payment.
  $response = $iats->directDebitACHEFTPayerValidate($request);

  return $response;
}
