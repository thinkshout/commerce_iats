<?php
/**
 * @file
 * Implements iATS Payments ACH/EFT services for use in Drupal Commerce
 * with Commerce Card on File.
 */

/**
 * Payment method callback: settings form.
 */
function commerce_iats_cardonfile_ach_eft_settings_form($settings = NULL) {
  return commerce_iats_settings_form_base($settings);
}

/**
 * Payment method callback: checkout form.
 */
function commerce_iats_cardonfile_ach_eft_submit_form($payment_method, $pane_values, $checkout_pane, $order) {
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
function commerce_iats_cardonfile_ach_eft_submit_form_validate($payment_method, $pane_form, $pane_values, $order, $form_parents = array()) {
  if (!commerce_iats_validate_payment_method($payment_method)) {
    drupal_set_message(t('Unable to process payment. Please contact the site administrator.'), 'error');
    return FALSE;
  }

  // Skip normal validation if the customer specified a card on file.
  if (!empty($pane_values['cardonfile']) && ($pane_values['cardonfile'] !== 'new')) {
    return TRUE;
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
function commerce_iats_cardonfile_ach_eft_submit_form_submit($payment_method, $pane_form, $pane_values, $order, $charge) {
  // Validate the charge.
  if (empty($charge['amount'])) {
    watchdog('commerce_iats', 'Empty charge amount: @charge for order @order', array(
      '@charge' => '<pre>' . print_r($charge, TRUE) . '</pre>',
      '@order' => '<pre>' . print_r($order, TRUE) . '</pre>',
    ), WATCHDOG_DEBUG);
    return FALSE;
  }

  $payment_data = $pane_values;

  if (isset($pane_values['cardonfile'])) {
    if ($pane_values['cardonfile'] === 'new') {
      if ($pane_values['bank_account']['cardonfile_store']) {
        // Create a new card on file record.
        return commerce_iats_process_payment($payment_method, $payment_data, $order, $charge, 'commerce_iats_cardonfile_ach_eft_create_and_charge');
      }
    }
    else {
      // Get the card on file data.
      $card_data = commerce_cardonfile_load($pane_values['cardonfile']);

      // Fail now if it is no longer available or the card is inactive.
      if (empty($card_data) || $card_data->status == 0) {
        drupal_set_message(t('The requested card on file is no longer valid.'), 'error');
        return FALSE;
      }

      return commerce_iats_process_payment($payment_method, $card_data, $order, $charge, 'commerce_iats_cardonfile_ach_eft_charge');
    }
  }

  // Customer has opted not to use card on file / card on file is not available.
  module_load_include('inc', 'commerce_iats', 'includes/commerce_iats.ach_eft');
  return commerce_iats_process_payment($payment_method, $payment_data, $order, $charge, 'commerce_iats_process_ach_eft_payment');
}

/**
 * Card on File charge callback function.
 *
 * @param array $payment_method
 *   The payment method used during this transaction.
 * @param array $card_data
 *   Array of credit card data.
 * @param object $order
 *   The order processed during this transaction.
 * @param array $charge
 *   The transaction charge.
 *
 * @return Mixed
 *   API response array or boolean FALSE on failure.
 */
function commerce_iats_cardonfile_ach_eft_charge_callback($payment_method, $card_data, $order, $charge = NULL) {
  return commerce_iats_process_payment($payment_method, $card_data, $order, $charge, 'commerce_iats_cardonfile_ach_eft_charge');
}

/**
 * Processes an ACH/EFT payment using a stored card on file.
 *
 * @param array $payment_method
 *   The payment method used during this transaction.
 * @param array $card_data
 *   Array of bank account data.
 * @param object $order
 *   The order processed during this transaction.
 * @param array $charge
 *   The transaction charge.
 *
 * @return Mixed
 *   API response array or error string.
 */
function commerce_iats_cardonfile_ach_eft_charge($payment_method, $card_data, $order, $charge = NULL) {
  // Create and populate the API request.
  $request = array(
    'customerIPAddress' => ip_address(),
    'customerCode' => $card_data->remote_id,
    'invoiceNum' => t('@prefix@number', array(
      '@prefix' => $payment_method['settings']['invoice_prefix'],
      '@number' => $order->order_number,
    )),
    'total' => commerce_currency_amount_to_decimal($charge['amount'], $charge['currency_code']),
  );

  // Get the API service.
  $iats = commerce_iats_get_service('ProcessLink', $payment_method);

  // Log the request.
  if ($payment_method['settings']['log']['request'] === 'request') {
    commerce_iats_log_request($payment_method,
      'commerce_iats_cardonfile_ach_eft_charge', $request);
  }

  $response = $iats->processACHEFTWithCustomerCode($request);

  return $response;
}

/**
 * Processes an ACH/EFT charge and creates a card on file record.
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
function commerce_iats_cardonfile_ach_eft_create_and_charge($payment_method, $payment_data, $order, $charge = NULL) {
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

  // Log the request.
  if ($payment_method['settings']['log']['request'] === 'request') {
    commerce_iats_log_request($payment_method,
      'commerce_iats_cardonfile_ach_eft_create_and_charge', $request);
  }

  // Create new customer code and process the payment.
  $response = $iats->createCustomerCodeAndProcessACHEFT($request);

  if ($response != NULL) {
    $customer_code = trim($response['CUSTOMERCODE']);
    if (commerce_iats_cardonfile_can_create_cardonfile($customer_code)) {
      $payment_data['bank_account']['default'] = (isset($payment_data['cardonfile_instance_default'])) ?
        $payment_data['cardonfile_instance_default'] : FALSE;

      // Add fake expiration date to force card on file to accept
      // this as a valid card. This is a hack because bank accounts
      // do not have expiration dates.
      // TODO: Review this expiration date hack.
      $payment_data['bank_account']['exp_year'] = (int) date('Y') + 10;
      $payment_data['bank_account']['exp_month'] = (int) date('m');

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

/**
 * Payment method callback: card update form.
 *
 * Updates bank account details stored with iATS Payments.
 */
function commerce_iats_cardonfile_ach_eft_update($form, &$form_state, $payment_method, $card_data) {
  $name_parts = explode(' ', $card_data->card_name);
  $first_name = array_shift($name_parts);
  $last_name = implode(' ', $name_parts);

  // Create and populate the API request.
  $request = array(
    'customerIPAddress' => ip_address(),
    'customerCode' => $card_data->remote_id,
    'firstName' => $first_name,
    'lastName' => $last_name,
    'accountCustomerName' => $card_data->card_name,
    'accountType' => $card_data->type,
    'updateAccountNum' => 0,
    // Only used for recurring payments.
    'recurring' => 0,
    // Only used for recurring payments.
    'beginDate' => '',
    // Only used for recurring payments.
    'endDate' => '',
  );

  // Get the API service.
  $iats = commerce_iats_get_service('CustomerLink', $payment_method);

  // Log the request.
  if ($payment_method['settings']['log']['request'] === 'request') {
    commerce_iats_log_request($payment_method,
      'commerce_iats_cardonfile_ach_eft_update', $request);
  }

  // Process the ACH/EFT update.
  $response = $iats->updateACHEFTCustomerCode($request);

  $auth_result = (isset($response['AUTHORIZATIONRESULT'])) ?
    trim($response['AUTHORIZATIONRESULT']) : NULL;

  if ($auth_result == NULL) {
    $auth_result = $response;
  }

  if (substr($auth_result, 0, 2) != COMMERCE_IATS_TXN_OK) {
    watchdog('commerce_iats_cardonfile',
      'iATS error when deleting customer code: @error',
      array(
        '@error' => $auth_result,
      )
    );
    return FALSE;
  }

  return TRUE;
}

/**
 * Payment method callback: card delete form.
 *
 * Deletes customer code and associated bank account details
 * stored with iATS Payments.
 * Will not delete customer codes that have been used in a payment transaction.
 */
function commerce_iats_cardonfile_ach_eft_delete($form, &$form_state, $payment_method, $card_data) {
  // Create and populate the API request.
  $request = array(
    'customerIPAddress' => ip_address(),
    'customerCode' => $card_data->remote_id,
  );

  // Get the API service.
  $iats = commerce_iats_get_service('CustomerLink', $payment_method);

  // Log the request.
  if ($payment_method['settings']['log']['request'] === 'request') {
    commerce_iats_log_request($payment_method,
      'commerce_iats_cardonfile_ach_eft_delete', $request);
  }

  // Process the customer code deletion.
  $response = $iats->deleteCustomerCode($request);

  $auth_result = (isset($response['AUTHORIZATIONRESULT'])) ?
    trim($response['AUTHORIZATIONRESULT']) : NULL;

  if ($auth_result == NULL) {
    $auth_result = $response;
  }

  if (substr($auth_result, 0, 2) == COMMERCE_IATS_TXN_OK) {
    return TRUE;
  }
  // Account for iATS retaining customer codes once used in a transaction.
  elseif (strstr($auth_result, 'THE CUSTOMER CODE HAS BEEN USED')) {
    drupal_set_message(
      t('Account details successfully removed. iATS Payments maintains a record of all account used in payment transactions.'),
      'error');
    return TRUE;
  }
  // Account for other API errors.
  else {
    watchdog('commerce_iats_cardonfile',
      'iATS error when deleting customer code: @error',
      array(
        '@error' => $auth_result,
      )
    );

    drupal_set_message(
      t('We were unable to remove your account details at this time.'),
      'error');
  }

  return FALSE;
}
