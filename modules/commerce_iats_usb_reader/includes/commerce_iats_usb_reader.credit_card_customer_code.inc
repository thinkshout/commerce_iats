<?php
/**
 * @file
 * Enables use of an encrypted USB card reader with the
 * iATS credit card customer code payment method.
 */

/**
 * Payment method callback: settings form.
 */
function commerce_iats_usb_reader_credit_card_customer_code_settings_form($settings = NULL) {
  module_load_include('inc', 'commerce_iats_usb_reader', 'includes/commerce_iats_usb_reader.forms');
  $form = commerce_iats_usb_reader_credit_card_settings_form_base($settings);
  return $form;
}

/**
 * Payment method callback: checkout form.
 */
function commerce_iats_usb_reader_credit_card_customer_code_submit_form($payment_method, $pane_values, $checkout_pane, $order) {
  module_load_include('inc', 'commerce_iats_usb_reader', 'includes/commerce_iats_usb_reader.forms');
  $form = commerce_iats_usb_reader_credit_card_form_base($payment_method, $pane_values, $checkout_pane, $order);
  return $form;
}

/**
 * Payment method callback: checkout form validation.
 */
function commerce_iats_usb_reader_credit_card_customer_code_submit_form_validate($payment_method, $pane_form, $pane_values, $order, $form_parents = array()) {
  if (!commerce_iats_validate_payment_method($payment_method)) {
    drupal_set_message(t('Unable to process payment. Please contact the site administrator.'), 'error');
    return FALSE;
  }

  // Validate the credit card fields.
  $settings = array(
    'form_parents' => array_merge($form_parents, array('credit_card')),
  );

  $prefix = implode('][', $settings['form_parents']) . '][';
  $valid = TRUE;

  if ($pane_values['usb_reader']) {
    // Validate the USB card reader input.
    $encrypted_string = $pane_values['encrypted_credit_card']['encrypted_number'];

    if (empty($encrypted_string)) {
      form_set_error($prefix . 'encrypted_number', t('There was a problem processing the card via the USB card reader.'));
      $valid = FALSE;
    }

    $type = commerce_iats_usb_reader_parse_card_type_from_encrypted_string($encrypted_string, $pane_values['credit_card']['valid_types']);

    if ($type === FALSE) {
      form_set_error($prefix . 'encrypted_number', t('The credit card does not appear to be a valid card type.'));
      $valid = FALSE;
    }
  }
  else {
    if (!commerce_iats_credit_card_submit_form_validate($payment_method, $pane_form, $pane_values, $order, $form_parents)) {
      $valid = FALSE;
    }
  }

  return $valid;
}

/**
 * Payment method callback: checkout form submission.
 */
function commerce_iats_usb_reader_credit_card_customer_code_submit_form_submit($payment_method, $pane_form, $pane_values, $order, $charge) {
  // Validate credit card charge.
  if (empty($charge['amount'])) {
    watchdog('commerce_iats', 'Empty charge amount: @charge for order @order', array(
      '@charge' => '<pre>' . check_plain(print_r($charge, TRUE)) . '</pre>',
      '@order' => '<pre>' . check_plain(print_r($order, TRUE)) . '</pre>',
    ), WATCHDOG_DEBUG);
    return FALSE;
  }

  return commerce_iats_process_payment($payment_method, $pane_values, $order,
    $charge, 'commerce_iats_usb_reader_process_credit_card_customer_code_payment');
}

/**
 * Creates a customer code via the iATS Payments API using
 * an encrypted USB card reader.
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
function commerce_iats_usb_reader_process_credit_card_customer_code_payment($payment_method, $payment_data, $order, $charge) {
  // Format the encrypted card data in the way iATS expects to receive it.
  // {Swiper type}|@|{Encrypted card data}
  $formatted_card_data = $payment_method['settings']['usb_reader_type']
    . COMMERCE_IATS_CARD_READER_DELIMITER
    . $payment_data['encrypted_credit_card']['encrypted_number'];

  // Create and populate the API request.
  $request = array(
    'customerIPAddress' => ip_address(),
    'creditCardNum' => $formatted_card_data,
    'total' => commerce_currency_amount_to_decimal($charge['amount'], $charge['currency_code']),
    'currency' => $charge['currency_code'],
    'mop' => strtoupper(commerce_iats_usb_reader_parse_card_type_from_encrypted_string(
      $payment_data['encrypted_credit_card']['encrypted_number'], $payment_data['credit_card']['valid_types'])),
    'recurring' => 0,
    'beginDate' => commerce_iats_get_formatted_date(time()),
    'endDate' => commerce_iats_get_formatted_date(time()),
    'comment' => '',
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
    commerce_iats_log_request($payment_method, 'commerce_iats_process_credit_card_payment', $request);
  }

  // Process the credit card payment.
  $response = $iats->createCreditCardCustomerCode($request);

  return $response;
}