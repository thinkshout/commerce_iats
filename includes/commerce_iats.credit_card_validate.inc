<?php
/**
* @file
* Implements iATS credit card validation services for use in Drupal Commerce.
*/

/**
 * Payment method callback: settings form.
 */
function commerce_iats_credit_card_validate_settings_form($settings = NULL) {
  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');
  $settings += commerce_iats_default_settings();

  $form = array();

  $form['agent_code'] = array(
    '#type' => 'textfield',
    '#title' => t('Agent Code'),
    '#description' => t('This is the agent code used to authenticate with the iATS website.'),
    '#default_value' => $settings['agent_code'],
    '#required' => TRUE,
  );
  $form['agent_password'] = array(
    '#type' => 'textfield',
    '#title' => t('Password'),
    '#description' => t('Enter the corresponding password for this agent code.'),
    '#default_value' => $settings['agent_password'],
    '#required' => TRUE,
  );
  $form['server'] = array(
    '#type' => 'radios',
    '#title' => t('Transaction Server'),
    '#description' => t('Select the transaction server to use for processing payments.'),
    '#options' => array(
      IATS_NA_SERVER_ID => t('US'),
      IATS_UK_SERVER_ID => t('UK'),
    ),
    '#default_value' => $settings['server'],
  );
  // TODO: Implement transaction mode.
  $form['transaction_mode'] = array(
    '#type' => 'radios',
    '#title' => t('Transaction mode - NOT IMPLEMENTED'),
    '#description' => t('Adjust to live transactions when you are ready to start processing real payments.'),
    '#options' => array(
      IATS_TXN_MODE_PRODUCTION => t('Live transactions in the production environment'),
      IATS_TXN_MODE_TEST => t('Test transactions in the test environment'),
    ),
    '#default_value' => $settings['transaction_mode'],
  );
  $form['log'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Log the following messages for debugging'),
    '#options' => array(
      'request' => t('API request messages'),
      'response' => t('API response messages'),
    ),
    '#default_value' => $settings['log'],
  );

  $form['credit_card'] = array(
    '#type' => 'fieldset',
    '#title' => t('Credit card service settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['credit_card']['card_types'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Limit accepted credit cards to the following types'),
    '#description' => t('If none are checked, any credit card type will be accepted.'),
    '#options' => commerce_iats_credit_card_types(),
    '#default_value' => isset($settings['credit_card']['card_types']) ? $settings['credit_card']['card_types'] : array(),
  );
  return $form;
}

/**
 * Payment method callback: checkout form.
 */
function commerce_iats_credit_card_validate_submit_form($payment_method, $pane_values, $checkout_pane, $order) {
  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');
  $fields = array();
  // Limit credit card types, since iATS has a limited selection.
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
function commerce_iats_credit_card_validate_submit_form_submit($payment_method, $pane_form, $pane_values, $order) {
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

  $iats = commerce_iats_get_customerlink_service($payment_method);
  if ($iats == NULL)
  {
    drupal_set_message(t('Unable to load iATS Payments library.'), 'error');
    return FALSE;
  }

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