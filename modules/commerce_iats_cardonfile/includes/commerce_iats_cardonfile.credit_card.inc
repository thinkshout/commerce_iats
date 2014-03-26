<?php
/**
 * @file
 * Implements iATS Payments credit card services for use in Drupal Commerce
 * with Commerce Card on File.
 */

/**
 * Payment method callback: settings form.
 */
function commerce_iats_cardonfile_credit_card_settings_form($settings = NULL) {
  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');

  $form = commerce_iats_credit_card_settings_form_base($settings);

  return $form;
}

/**
 * Payment method callback: checkout form.
 */
function commerce_iats_cardonfile_credit_card_submit_form($payment_method, $pane_values, $checkout_pane, $order) {
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
function commerce_iats_cardonfile_credit_card_submit_form_validate($payment_method, $pane_form, $pane_values, $order, $form_parents = array()) {
  // Skip normal validation if the customer specified a card on file.
  if (!empty($pane_values['cardonfile']) && ($pane_values['cardonfile'] !== 'new')) {
    return TRUE;
  }

  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');

  // Validate the credit card fields.
  $settings = array(
    'form_parents' => array_merge($form_parents, array('credit_card')),
  );

  if (!commerce_payment_credit_card_validate($pane_values['credit_card'], $settings)) {
    return FALSE;
  }
  return TRUE;
}

/**
 * Payment method callback: checkout form submission.
 */
function commerce_iats_cardonfile_credit_card_submit_form_submit($payment_method, $pane_form, $pane_values, $order, $charge) {
  // Validate credit card charge.
  if (empty($charge['amount'])) {
    watchdog('commerce_iats', 'Empty charge amount: @charge for order @order', array(
      '@charge' => '<pre>' . check_plain(print_r($charge, TRUE)) . '</pre>',
      '@order' => '<pre>' . check_plain(print_r($order, TRUE)) . '</pre>',
    ), WATCHDOG_DEBUG);
    return FALSE;
  }

  $payment_data = $pane_values;

  if (isset($pane_values['cardonfile'])) {
    if ($pane_values['cardonfile'] === 'new') {
      if ($pane_values['credit_card']['cardonfile_store']) {
        // Create a new card on file record.
        return commerce_iats_process_payment($payment_method, $payment_data, $order, $charge, 'commerce_iats_cardonfile_credit_card_create_and_charge');
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

      $card_data->code = $pane_values['credit_card']['code'];

      return commerce_iats_process_payment($payment_method, $card_data, $order, $charge, 'commerce_iats_cardonfile_credit_card_charge');
    }
  }

  // Customer has opted not to use card on file or card on file is not available.
  module_load_include('inc', 'commerce_iats', 'includes/commerce_iats.credit_card');
  return commerce_iats_process_payment($payment_method, $payment_data, $order, $charge, 'commerce_iats_process_credit_card_payment');
}

/**
 * Processes a credit card payment using a stored card on file via the
 * iATS Payments API.
 *
 * @param array $payment_method
 *  The payment method used during this transaction.
 * @param array $card_data
 *  Array of credit card data.
 * @param object $order
 *  The order processed during this transaction.
 * @param array $charge
 *  The transaction charge.
 *
 * @return Mixed
 *  API response array or error string.
 */
function commerce_iats_cardonfile_credit_card_charge($payment_method, $card_data, $order, $charge = NULL) {
  // Create and populate the API request.
  $request = array(
    'customerIPAddress' => ip_address(),
    'customerCode' => $card_data->remote_id,
    'invoiceNum' => t('@prefix@number', array(
      '@prefix' => $payment_method['settings']['invoice_prefix'],
      '@number' => $order->order_number,
    )),
    'mop' => commerce_iats_credit_card_mop($card_data->card_type),
    'total' => commerce_currency_amount_to_decimal($charge['amount'], $charge['currency_code']),
    'currency' => $charge['currency_code'],
  );

  // Get the API service.
  $iats = commerce_iats_get_service('ProcessLink', $payment_method);
  if ($iats == NULL) {
    drupal_set_message(t('Unable to load iATS Payments library.'), 'error');
    return NULL;
  }

  // Log the request.
  if ($payment_method['settings']['log']['request'] === 'request') {
    commerce_iats_log_request($payment_method,
      'commerce_iats_cardonfile_credit_card_charge', $request);
  }

  $response = $iats->processCreditCardWithCustomerCode($request);

  return $response;
}

/**
 * Processes a credit card charge and creates a card on file record
 * via the iATS Payments API.
 *
 * @param array $payment_method
 *  The payment method used during this transaction.
 * @param array $payment_data
 *  Payment data retrieved from the submit form.
 * @param object $order
 *  The order processed during this transaction.
 * @param array $charge
 *  The transaction charge.
 *
 * @return Mixed
 *  API response array or error string.
 */
function commerce_iats_cardonfile_credit_card_create_and_charge($payment_method, $payment_data, $order, $charge = NULL) {
  // Format credit card data.
  $formatted_card_data = commerce_iats_format_credit_card_data($payment_data['credit_card']);

  // Create and populate the API request.
  $request = array(
    'customerIPAddress' => ip_address(),
    'invoiceNum' => t('@prefix@number', array(
      '@prefix' => $payment_method['settings']['invoice_prefix'],
      '@number' => $order->order_number,
    )),
    'ccNum' => $formatted_card_data['number'],
    'ccExp' => $formatted_card_data['expiry'],
    'cvv2' => $payment_data['credit_card']['code'],
    'mop' => $formatted_card_data['type'],
    'total' => commerce_currency_amount_to_decimal($charge['amount'], $charge['currency_code']),
    'currency' => $charge['currency_code'],
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
    commerce_iats_log_request($payment_method,
      'commerce_iats_cardonfile_credit_card_create_and_charge', $request);
  }

  // Create new customer code and process the payment.
  $response = $iats->createCustomerCodeAndProcessCreditCard($request);

  if ($response != NULL) {
    if (isset($response['CUSTOMERCODE'])) {
      $customer_code = trim($response['CUSTOMERCODE']);
      // Create a new card on file record for the new customer code.
      commerce_iats_cardonfile_create_cardonfile($order, $payment_method,
        $payment_data['credit_card'], $customer_code, $formatted_card_data['orig_type']);
    }
    else {
      drupal_set_message(t('Your credit card could not be stored for later use.'), 'error');
    }
  }

  return $response;
}

/**
 * Payment method callback: card update form.
 *
 * Updates credit card details stored with iATS Payments.
 */
function commerce_iats_cardonfile_credit_card_update($form, &$form_state, $payment_method, $card_data) {
  $card = array(
    'number' => '',
    'exp_month' => $card_data->card_exp_month,
    'exp_year' => $card_data->card_exp_year,
  );

  $formatted_card_data = commerce_iats_format_credit_card_data($card);

  $name_parts = explode(' ', $card_data->card_name);
  $first_name = array_shift($name_parts);
  $last_name = implode(' ', $name_parts);

  // Create and populate the API request.
  $request = array(
    'customerIPAddress' => ip_address(),
    'customerCode' => $card_data->remote_id,
    'firstName' => $first_name,
    'lastName' => $last_name,
    'creditCardExpiry' => $formatted_card_data['expiry'],
    'mop' => commerce_iats_credit_card_mop($card_data->card_type),
    'updateCreditCardNum' => FALSE,
    'recurring' => FALSE, // Only used for recurring payments.
    'beginDate' => 0, // Only used for recurring payments.
    'endDate' => 0, // Only used for recurring payments.
  );

  // Get the API service.
  $iats = commerce_iats_get_service('CustomerLink', $payment_method);
  if ($iats == NULL) {
    drupal_set_message(t('Unable to load iATS Payments library.'), 'error');
    return FALSE;
  }

  // Log the request.
  if ($payment_method['settings']['log']['request'] === 'request') {
    commerce_iats_log_request($payment_method,
      'commerce_iats_cardonfile_credit_card_update', $request);
  }

  // Process the credit card update.
  $response = $iats->updateCreditCardCustomerCode($request);

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
 * Deletes customer code and associated credit card details
 * stored with iATS Payments.
 * Will not delete customer codes that have been used in a payment transaction.
 */
function commerce_iats_cardonfile_credit_card_delete($form, &$form_state, $payment_method, $card_data) {
  // Create and populate the API request.
  $request = array(
    'customerIPAddress' => ip_address(),
    'customerCode' => $card_data->remote_id,
  );

  // Get the API service.
  $iats = commerce_iats_get_service('CustomerLink', $payment_method);
  if ($iats == NULL) {
    drupal_set_message(t('Unable to load iATS Payments library.'), 'error');
    return FALSE;
  }

  // Log the request.
  if ($payment_method['settings']['log']['request'] === 'request') {
    commerce_iats_log_request($payment_method,
      'commerce_iats_cardonfile_credit_card_delete', $request);
  }

  // Process the credit card deletion.
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
      t('Your card has been deleted, but could not be removed from iATS Payments as it was used in a transaction.'),
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
      t('An error occurred when trying to delete your card through the iATS API.'),
      'error');
  }

  return FALSE;
}