<?php
/**
* @file
* Implements iATS credit card customer code services for use in Drupal Commerce.
*/

/**
 * Payment method callback: settings form.
 */
function commerce_iats_credit_card_customer_code_settings_form($settings = NULL) {
  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');
  $settings += commerce_iats_default_settings();

  $form = commerce_iats_credit_card_settings_form_base($settings);

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
  $card_types = array_diff(array_values($payment_method['settings']['credit_card']['card_types']), array(0));
  if (count($card_types) == 0)
  {
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
  if (!empty($pane_values['credit_card']['number'])) {
    module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');
    $card_type = commerce_payment_validate_credit_card_type($pane_values['credit_card']['number'], array_keys(commerce_iats_credit_card_types()));
  }

  $expiry_string = sprintf('%02d/%02d', $pane_values['credit_card']['exp_month'], ($pane_values['credit_card']['exp_year'] % 100));

  $request = array(
    'customerIPAddress' => ip_address(),
    'mop' => commerce_iats_credit_card_mop($card_type),
    'creditCardNum' => str_replace(' ', '', $pane_values['credit_card']['number']),
    'creditCardExpiry' => $expiry_string,
    'cvv2' => $pane_values['credit_card']['code'],
    'recurring' => FALSE, // Only used for recurring payments.
    'beginDate' => 0, // Only used for recurring payments.
    'endDate' => 0, // Only used for recurring payments.
    'currency' => 'USD',
  );

  // Add the billing information.
  commerce_iats_address($request, $order, 'commerce_customer_billing');

  $iats = commerce_iats_get_customerlink_service($payment_method);
  if ($iats == NULL)
  {
    drupal_set_message(t('Unable to load iATS Payments library.'), 'error');
    return FALSE;
  }

  if ($payment_method['settings']['log']['request'] == 'request') {
    commerce_iats_log_request($payment_method, $request);
  }

  $response = $iats->createCreditCardCustomerCode($request);

  if ($response != NULL) {

    if ($payment_method['settings']['log']['response'] == 'response') {
      commerce_iats_log_response($payment_method, $response);
    }

    $auth_result = (isset($response['AUTHORIZATIONRESULT']))? trim($response['AUTHORIZATIONRESULT']) : NULL;
    $customer_code = (isset($response['CUSTOMERCODE']))? trim($response['CUSTOMERCODE']) : NULL;

    if ($auth_result == NULL) {
      $auth_result = $response;
    }

    if (strpos($auth_result, 'Error') !== FALSE) {

      drupal_set_message(t('There was an error processing this transaction: @error.',
          array('@error' => $auth_result))
        , 'error');
      return FALSE;
    }

    // Prepare a transaction object to log the API response.
    $transaction = commerce_payment_transaction_new('iats_credit_card_customer_code', $order->order_id);
    $transaction->instance_id = $payment_method['instance_id'];

    $transaction->remote_id = $customer_code;
    $transaction->amount = 0;
    $transaction->currency_code = 'USD';
    $transaction->status = COMMERCE_PAYMENT_STATUS_SUCCESS;

    // Build a meaningful response message.
    $message = array(
      '<b>' . t('Customer code created.') . '</b>');

    $transaction->message = implode('<br />', $message);

    // Save the transaction information.
    commerce_payment_transaction_save($transaction);

    // Save the customer code as a card on file.
    commerce_iats_credit_card_save_card_on_file($order, $payment_method, $pane_values, $customer_code, $card_type);
  }
}

/**
 * Gets an instance of the iATS Payments API CustomerLink service.
 *
 * @return CustomerLink
 */
function commerce_iats_get_customerlink_service($payment_method) {
  if (commerce_iats_load_library()) {
    $service = new iATS\CustomerLink($payment_method['settings']['agent_code'],
      $payment_method['settings']['agent_password'],
      $payment_method['settings']['server']);

    return $service;
  }

  return NULL;
}