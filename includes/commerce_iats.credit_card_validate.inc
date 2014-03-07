<?php
/**
* @file
* Implements iATS credit card validation services for use in Drupal Commerce.
*/

/**
 * Payment method callback: checkout form.
 */
function commerce_iats_credit_card_validate_submit_form($payment_method, $pane_values, $checkout_pane, $order) {
  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');
  $fields = array();
  // Limit credit card types, since IATS has a limited selection.
  $card_types = array_diff(array_values($payment_method['settings']['credit_card']['card_types']), array(0));
  if (!empty($card_types)) {
    if ($card_types === array(0)) {
      $card_types = commerce_iats_credit_card_types();
    }
    $fields['type'] = $card_types;
  }

  $form = commerce_payment_credit_card_form($fields);
  return $form;
}

/**
 * Payment method callback: checkout form submission.
 */
function commerce_iats_soap_customerlink_submit_form_submit($payment_method, $pane_form, $pane_values, $order) {
  $request = array(
    'customerIPAddress' => ip_address(),
  );

  // Add the billing information.
  commerce_iats_address($request, 'billing', $order, 'commerce_customer_billing');

  // Add the credit card details.
  $expiry_string = sprintf('%02d/%02d', $pane_values['credit_card']['exp_month'], ($pane_values['credit_card']['exp_year'] % 100));

  $request['creditCardNum'] = str_replace(' ', '', $pane_values['credit_card']['number']);
  $request['creditCardExpiry'] = $expiry_string;

  // Add the CVV data, if included.
  if (isset($pane_values['credit_card']['code'])) {
    $request['cvv2'] = $pane_values['credit_card']['code'];
  }

  $request['mop'] = commerce_iats_credit_card_mop($pane_values['credit_card']['type']);

  // Instantiate the iATS ProcessLink class to process credit card payment.
  $iats = new CustomerLink($payment_method['settings']['agent_code'], $payment_method['settings']['agent_password']);

  $response = $iats->createCreditCardCustomerCode($request);

  if ($response != NULL) {
    $auth_result = $response['AUTHORIZATIONRESULT'];

    if (strpos($auth_result, 'Error') !== FALSE) {

      drupal_set_message(t('There was an error processing this transaction: @error.',
          array('@error' => $auth_result))
        , 'error');
      return FALSE;
    }

    // Prepare a transaction object to log the API response.
    $transaction = commerce_payment_transaction_new('iats_credit_card_validate', $order->order_id);
    $transaction->instance_id = $payment_method['instance_id'];

    $transaction->remote_id = $response['CUSTOMERCODE'];
    $transaction->amount = 0;
    $transaction->currency_code = 'USD';
    $transaction->status = COMMERCE_PAYMENT_STATUS_SUCCESS;

    // Build a meaningful response message.
    $message = array(
      '<b>' . t('Customer code created.') . '</b>');

    $transaction->message = implode('<br />', $message);

    // Save the transaction information.
    commerce_payment_transaction_save($transaction);
  }
}