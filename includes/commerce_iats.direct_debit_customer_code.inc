<?php
/**
 * @file
 * Implements iATS Payments direct debit customer code services for use
 * in Drupal Commerce.
 */

/**
 * Payment method callback: settings form.
 */
function commerce_iats_direct_debit_customer_code_settings_form($settings = NULL) {
  $form = commerce_iats_settings_form_base($settings);

  // No invoice in Direct Debit customer code creation.
  unset($form['invoice_prefix']);

  // Direct Debit limited to UK server.
  $form['server']['#options'] = array(
    COMMERCE_IATS_UK_SERVER_ID => t('UK'),
  );
  $form['server']['#default_value'] = COMMERCE_IATS_UK_SERVER_ID;

  return $form;
}

/**
 * Payment method callback: checkout form submission.
 */
function commerce_iats_direct_debit_customer_code_submit_form_submit($payment_method, $pane_form, $pane_values, $order, $charge) {
  $payment_data = $pane_values;

  return commerce_iats_process_payment($payment_method, $payment_data, $order,
    $charge, 'commerce_iats_direct_debit_create_customer_code');
}

/**
 * Creates a direct debit customer code via the iATS Payments API.
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
function commerce_iats_direct_debit_create_customer_code($payment_method, $payment_data, $order, $charge) {
  $combined_address = $payment_data['account_payer']['payer']['address_1'] . "\n"
    . $payment_data['account_payer']['payer']['address_2'] . "\n"
    . $payment_data['account_payer']['payer']['address_3'];

  // Create and populate the API request.
  $request = array(
    'customerIPAddress' => ip_address(),
    'ACHEFTReferenceNum' => $payment_data['validate']['reference_number'],
    'firstName' => $payment_data['account_payer']['payer']['first_name'],
    'lastName' => $payment_data['account_payer']['payer']['last_name'],
    'companyName' => $payment_data['account_payer']['payer']['company_name'],
    'address' => $combined_address,
    'city' => $payment_data['account_payer']['payer']['town'],
    'state' => $payment_data['account_payer']['payer']['county'],
    'zipCode' => $payment_data['account_payer']['payer']['postcode'],
    'phone' => $payment_data['account_payer']['payer']['phone'],
    'email' => $payment_data['account_payer']['payer']['email'],
    'recurring' => TRUE,
    'amount' => $payment_data['account_payer']['account']['amount'],
    'beginDate' => $payment_data['account_payer']['account']['first_collection_date'],
    'endDate' => $payment_data['recurring']['end_date'],
    'schduleType' => $payment_data['recurring']['schedule_type'],
    'scheduleDate' => $payment_data['recurring']['schedule_date'],
    'accountCustomerName' => $payment_data['account_payer']['account']['account_holder'],
    'accountNum' => $payment_data['account_payer']['account']['number'],
  );

  // Get the API service.
  $iats = commerce_iats_get_service('CustomerLink', $payment_method);
  if ($iats == NULL) {
    drupal_set_message(t('Unable to load iATS Payments library.'), 'error');
    return NULL;
  }

  // Log the request.
  if ($payment_method['settings']['log']['request'] === 'request') {
    commerce_iats_log_request($payment_method, 'commerce_iats_direct_debit_create_customer_code', $request);
  }

  // Validate the direct debit payment.
  $response = $iats->directDebitCreateACHEFTCustomerCode($request);

  return $response;
}
