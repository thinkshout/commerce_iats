<?php
/**
 * @file
 * Implements iATS Payments ACH/EFT customer code services for use in Drupal
 * Commerce.
 */

/**
 * Payment method callback: settings form.
 */
function commerce_iats_cardonfile_ach_eft_customer_code_settings_form($settings = NULL) {
  $form = commerce_iats_cardonfile_customer_code_settings_form_base($settings);

  // No invoice when creating customer codes.
  unset($form['invoice_prefix']);

  return $form;
}

/**
 * Payment method callback: checkout form.
 */
function commerce_iats_cardonfile_ach_eft_customer_code_submit_form($payment_method, $pane_values, $checkout_pane, $order) {
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
function commerce_iats_cardonfile_ach_eft_customer_code_submit_form_validate($payment_method, $pane_form, $pane_values, $order, $form_parents = array()) {
  if (!commerce_iats_validate_payment_method($payment_method)) {
    drupal_set_message(t('Unable to process payment. Please contact the site administrator.'), 'error');
    return FALSE;
  }

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
function commerce_iats_cardonfile_ach_eft_customer_code_submit_form_submit($payment_method, $pane_form, $pane_values, $order) {
  $payment_data = $pane_values;

  return commerce_iats_process_payment($payment_method, $payment_data, $order,
    NULL, 'commerce_iats_cardonfile_ach_eft_create_customer_code');
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
function commerce_iats_cardonfile_ach_eft_create_customer_code($payment_method, $payment_data, $order, $charge) {
  $request = array(
    'customerIPAddress' => ip_address(),
    'accountNum' => $payment_data['bank_account']['number'],
    'accountType' => $payment_data['bank_account']['type'],
  );

  if (isset($payment_method['settings']['allow_recurring'])
    && $payment_method['settings']['allow_recurring'] == 1) {
    $request['recurring'] = 1;
    $request['beginDate'] = $order->data['recurring']['begin_date'];
    $request['endDate'] = $order->data['recurring']['end_date'];
    $request['amount'] = $order->data['recurring']['amount'];
    $request['scheduleType'] = $order->data['recurring']['schedule_type'];
    $request['scheduleDate'] = $order->data['recurring']['schedule_date'];
  }
  else {
    $request['recurring'] = 0;
    $request['beginDate'] = commerce_iats_get_formatted_date(time());
    $request['endDate'] = commerce_iats_get_formatted_date(time());
  }

  // Add the billing information.
  commerce_iats_address($request, $order, 'commerce_customer_billing');

  // Get the API service.
  $iats = commerce_iats_get_service('CustomerLink', $payment_method);

  // Log the request.
  if ($payment_method['settings']['log']['request'] === 'request') {
    commerce_iats_log_request($payment_method, 'commerce_iats_cardonfile_ach_eft_create_customer_code', $request);
  }

  $response = $iats->createACHEFTCustomerCode($request);

  if ($response != NULL) {
    $customer_code = trim($response['CUSTOMERCODE']);
    if (commerce_iats_cardonfile_can_create_cardonfile($customer_code)) {
      $payment_data['bank_account']['default'] = (isset($payment_data['cardonfile_instance_default'])) ?
        $payment_data['cardonfile_instance_default'] : FALSE;

      // Create a new card on file record for the new customer code.
      commerce_iats_cardonfile_create_cardonfile($order, $payment_method,
        $payment_data['bank_account'], $customer_code,
        $payment_data['bank_account']['type']);
    }
    else {
      drupal_set_message(t('Your account details could not be stored for later use.'), 'error');
    }
  }

  return $response;
}
