<?php

/**
 * @file
 * Enables use of a USB card reader with the iATS credit card payment method.
 */

/**
 * Payment method callback: settings form.
 */
function commerce_iats_usb_reader_credit_card_settings_form($settings = NULL) {
  module_load_include('inc', 'commerce_iats_usb_reader', 'includes/commerce_iats_usb_reader.forms');
  $form = commerce_iats_usb_reader_credit_card_settings_form_base($settings);

  // Unset fields reserved for customer code creation.
  unset($form['cardonfile_payment_method']);
  unset($form['allow_recurring']);

  return $form;
}

/**
 * Payment method callback: checkout form.
 */
function commerce_iats_usb_reader_credit_card_submit_form($payment_method, $pane_values, $checkout_pane, $order) {
  if (isset($payment_method['settings']['direct_post']['enable_direct_post']) && $payment_method['settings']['direct_post']['enable_direct_post'] == 1) {
    module_load_include('inc', 'commerce_iats', 'includes/commerce_iats.direct_post.forms');
    return commerce_iats_direct_post_credit_card_form($payment_method, $order);
  }
  else {
    module_load_include('inc', 'commerce_iats_usb_reader', 'includes/commerce_iats_usb_reader.forms');
    return commerce_iats_usb_reader_credit_card_form_base($payment_method, $pane_values, $checkout_pane, $order);
  }
}

/**
 * Payment method callback: checkout form validation.
 */
function commerce_iats_usb_reader_credit_card_submit_form_validate($payment_method, $pane_form, $pane_values, $order, $form_parents = array()) {
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

    $card_data = commerce_iats_usb_reader_parse_card_data_from_encrypted_string($encrypted_string, $pane_values['credit_card']['valid_types']);

    if ($card_data['type'] === FALSE) {
      form_set_error($prefix . 'encrypted_number', t('The credit card does not appear to be a valid card type.'));
      $valid = FALSE;
    }
  }
  else {
    module_load_include('inc', 'commerce_iats', 'includes/commerce_iats.credit_card');
    if (!commerce_iats_credit_card_submit_form_validate($payment_method, $pane_form, $pane_values, $order, $form_parents)) {
      $valid = FALSE;
    }
  }

  return $valid;
}

/**
 * Payment method callback: checkout form submission.
 */
function commerce_iats_usb_reader_credit_card_submit_form_submit($payment_method, $pane_form, $pane_values, $order, $charge) {
  // Validate credit card charge.
  if (empty($charge['amount'])) {
    watchdog('commerce_iats', 'Empty charge amount: @charge for order @order', array(
      '@charge' => '<pre>' . check_plain(print_r($charge, TRUE)) . '</pre>',
      '@order' => '<pre>' . check_plain(print_r($order, TRUE)) . '</pre>',
    ), WATCHDOG_DEBUG);
    return FALSE;
  }

  if ($pane_values['usb_reader']) {
    return commerce_iats_process_payment($payment_method, $pane_values, $order,
      $charge, 'commerce_iats_usb_reader_process_credit_card_payment');
  }
  else {
    module_load_include('inc', 'commerce_iats', 'includes/commerce_iats.credit_card');
    return commerce_iats_process_payment($payment_method, $pane_values, $order,
      $charge, 'commerce_iats_process_credit_card_payment');
  }
}

/**
 * Processes a credit card payment via the iATS Payments API.
 *
 * Using an encrypted USB card reader.
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
function commerce_iats_usb_reader_process_credit_card_payment($payment_method, $payment_data, $order, $charge) {
  // Format the encrypted card data in the way iATS expects to receive it.
  // {Swiper type}|@|{Encrypted card data}
  $formatted_card_data = $payment_method['settings']['usb_reader']['usb_reader_type']
    . COMMERCE_IATS_USB_READER_DELIMITER
    . $payment_data['encrypted_credit_card']['encrypted_number'];

  $card_data = commerce_iats_usb_reader_parse_card_data_from_encrypted_string(
    $payment_data['encrypted_credit_card']['encrypted_number'], $payment_data['credit_card']['valid_types']);

  // Create and populate the API request.
  $invoice_number = $order->order_number;
  drupal_alter('commerce_iats_invoice_number', $invoice_number, $order);

  $request = array(
    'customerIPAddress' => ip_address(),
    'invoiceNum' => t('@prefix@number', array(
      '@prefix' => $payment_method['settings']['invoice_prefix'],
      '@number' => $invoice_number,
    )),
    'creditCardNum' => $formatted_card_data,
    'total' => commerce_currency_amount_to_decimal($charge['amount'], $charge['currency_code']),
    'currency' => $charge['currency_code'],
    'mop' => strtoupper($card_data['type']),
    'comment' => COMMERCE_IATS_COMMENT_PREFIX,
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
    commerce_iats_log_request($payment_method, 'commerce_iats_usb_reader_process_credit_card_payment', $request);
  }

  // Process the credit card payment.
  $response = $iats->processCreditCard($request);

  return $response;
}
