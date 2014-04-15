<?php
/**
 * @file
 * Implements iATS Payments ACH/EFT services for use in Drupal Commerce.
 */

/**
 * Payment method callback: settings form.
 */
function commerce_iats_ach_eft_settings_form($settings = NULL) {
  $form = commerce_iats_settings_form_base($settings);

  return $form;
}

/**
 * Payment method callback: checkout form.
 */
function commerce_iats_ach_eft_submit_form($payment_method, $pane_values, $checkout_pane, $order) {
  module_load_include('inc', 'commerce_iats', 'includes/commerce_iats.ach_eft.forms');

  $fields = array(
    'type' => '',
    'number' => '',
  );

  return commerce_iats_ach_eft_payment_form($fields);
}

/**
 * Payment method callback: checkout form validation.
 */
function commerce_iats_ach_eft_submit_form_validate($payment_method, $pane_form, $pane_values, $order, $form_parents = array()) {
  module_load_include('inc', 'commerce_iats', 'includes/commerce_iats.ach_eft.forms');

  // Validate the bank account fields.
  $settings = array(
    'form_parents' => array_merge($form_parents, array('bank_account')),
  );

  if (!commerce_iats_ach_eft_payment_form_validate($pane_values['bank_account'], $settings)) {
    return FALSE;
  }
  return TRUE;
}

/**
 * Payment method callback: checkout form submission.
 */
function commerce_iats_ach_eft_submit_form_submit($payment_method, $pane_form, $pane_values, $order, $charge) {
  // Validate charge.
  if (empty($charge['amount'])) {
    watchdog('commerce_iats', 'Empty charge amount: @charge for order @order', array(
      '@charge' => '<pre>' . check_plain(print_r($charge, TRUE)) . '</pre>',
      '@order' => '<pre>' . check_plain(print_r($order, TRUE)) . '</pre>',
    ), WATCHDOG_DEBUG);
    return FALSE;
  }

  $payment_data = $pane_values;

  return commerce_iats_process_payment($payment_method, $payment_data, $order,
    $charge, 'commerce_iats_process_ach_eft_payment');
}

/**
 * Processes an ACH/EFT payment via the iATS Payments API.
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
function commerce_iats_process_ach_eft_payment($payment_method, $payment_data, $order, $charge) {
  // Create and populate the API request.
  $request = array(
    'customerIPAddress' => ip_address(),
    'invoiceNum' => t('@prefix@number', array(
      '@prefix' => $payment_method['settings']['invoice_prefix'],
      '@number' => $order->order_number,
    )),
    'accountNum' => $payment_data['bank_account']['number'],
    'accountType' => $payment_data['bank_account']['type'],
    'total' => commerce_currency_amount_to_decimal($charge['amount'], $charge['currency_code']),
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
    commerce_iats_log_request($payment_method, 'commerce_iats_process_ach_eft_payment', $request);
  }

  // Process the ACH/EFT payment.
  $response = $iats->processACHEFT($request);

  return $response;
}
