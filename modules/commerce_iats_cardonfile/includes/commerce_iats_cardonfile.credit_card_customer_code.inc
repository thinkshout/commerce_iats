<?php
/**
 * @file
 * Implements iATS Payments credit card customer code services for use in Drupal
 * Commerce.
 */

/**
 * Payment method callback: settings form.
 */
function commerce_iats_cardonfile_credit_card_customer_code_settings_form($settings = NULL) {
  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');

  $form = commerce_iats_cardonfile_customer_code_settings_form_base($settings);

  // No invoice when creating customer codes.
  unset($form['invoice_prefix']);

  return $form;
}

/**
 * Payment method callback: checkout form.
 */
function commerce_iats_cardonfile_credit_card_customer_code_submit_form($payment_method, $pane_values, $checkout_pane, $order) {
  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');

  // Prepare the fields to include on the credit card form.
  $fields = array(
    'code' => '',
    'type' => commerce_iats_enabled_credit_card_types($payment_method),
  );

  return commerce_payment_credit_card_form($fields);
}

/**
 * Payment method callback: checkout form validation.
 */
function commerce_iats_cardonfile_credit_card_customer_code_submit_form_validate($payment_method, $pane_form, $pane_values, $order, $form_parents = array()) {
  // Skip normal validation if the customer specified a card on file.
  if (!empty($pane_values['cardonfile']) && ($pane_values['cardonfile'] !== 'new')) {
    return TRUE;
  }

  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');

  // Validate the credit card fields.
  $settings = array(
    'form_parents' => array_merge($form_parents, array('credit_card')),
  );

  return commerce_payment_credit_card_validate($pane_values['credit_card'], $settings);

  // TODO: Validate recurring payment fields.
}

/**
 * Payment method callback: checkout form submission.
 */
function commerce_iats_cardonfile_credit_card_customer_code_submit_form_submit($payment_method, $pane_form, $pane_values, $order) {
  $payment_data = $pane_values;

  return commerce_iats_process_payment($payment_method, $payment_data, $order,
    NULL, 'commerce_iats_cardonfile_credit_card_create_customer_code');
}

/**
 * Creates a customer code and associated card on file record.
 *
 * @param array $payment_method
 *   The payment method used during this transaction.
 * @param array $payment_data
 *   Payment data retrieved from the submit form.
 * @param object $order
 *   The order processed during this transaction.
 * @param array $charge
 *   The transaction charge. Unused in this method but required
 *   as part of callback signature.
 *
 * @return Mixed
 *   API response array or error string.
 */
function commerce_iats_cardonfile_credit_card_create_customer_code($payment_method, $payment_data, $order, $charge) {
  // Format credit card data.
  $formatted_card_data = commerce_iats_format_credit_card_data($payment_data['credit_card']);

  // Create and populate the API request.
  $request = array(
    'customerIPAddress' => ip_address(),
    'mop' => $formatted_card_data['type'],
    'creditCardNum' => $formatted_card_data['number'],
    'creditCardExpiry' => $formatted_card_data['expiry'],
    'cvv2' => $payment_data['credit_card']['code'],
    'currency' => 'USD',
  );

  if (isset($payment_method['settings']['allow_recurring'])
    && $payment_method['settings']['allow_recurring'] == 1) {
    $request['recurring'] = TRUE;
    // TODO: Handle recurring begin and end dates.
    $request['beginDate'] = 0;
    $request['endDate'] = 0;
    $request['amount'] = $payment_data['recurring']['amount'];
    $request['scheduleType'] = $payment_data['recurring']['schedule_type'];
    $request['scheduleDate'] = $payment_data['recurring']['schedule_date'];
  }
  else {
    $request['recurring'] = FALSE;
    $request['beginDate'] = 0;
    $request['endDate'] = 0;
  }

  // Add the billing information.
  commerce_iats_address($request, $order, 'commerce_customer_billing');

  // Get the API service.
  $iats = commerce_iats_get_service('CustomerLink', $payment_method);

  // Log the request.
  if ($payment_method['settings']['log']['request'] === 'request') {
    commerce_iats_log_request($payment_method, 'commerce_iats_cardonfile_credit_card_create_customer_code', $request);
  }

  $response = $iats->createCreditCardCustomerCode($request);

  if ($response != NULL) {
    if (isset($response['CUSTOMERCODE'])) {
      $customer_code = trim($response['CUSTOMERCODE']);

      $recurring_data = (isset($payment_data['recurring'])) ?
        $payment_data['recurring'] : NULL;

      $payment_data['credit_card']['default'] = (isset($payment_data['cardonfile_instance_default'])) ?
        $payment_data['cardonfile_instance_default'] : FALSE;

      // Create a new card on file record for the new customer code.
      commerce_iats_cardonfile_create_cardonfile($order, $payment_method,
        $payment_data['credit_card'], $recurring_data, $customer_code,
        $formatted_card_data['orig_type']);
    }
    else {
      drupal_set_message(t('Your credit card could not be stored for later use.'), 'error');
    }
  }

  return $response;
}
