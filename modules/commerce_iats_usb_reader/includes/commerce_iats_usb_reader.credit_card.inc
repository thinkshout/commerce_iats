<?php
/**
 * @file
 * Enables use of an encrypted USB card reader with the
 * iATS credit card payment method.
 */

/**
 * Payment method callback: settings form.
 */
function commerce_iats_usb_reader_credit_card_settings_form($settings = NULL) {
  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');

  $form = commerce_iats_credit_card_settings_form_base($settings);

  $form['usb_reader'] = array(
    '#type' => 'checkbox',
    '#title' => 'Enable encrypted USB card reader',
    '#description' => 'Enable to optionally submit card data using an encrypted USB card reader (admin only.)',
    '#default_value' => isset($settings['usb_reader']) ? $settings['usb_reader'] : FALSE,
    '#weight' => 3,
  );

  $form['usb_reader_type'] = array(
    '#type' => 'select',
    '#title' => t('USB card reader type'),
    '#options' => commerce_iats_usb_reader_types(),
    '#default_value' => isset($default['usb_reader_type']) ? $default['usb_reader_type'] : NULL,
    '#weight' => 4,
    '#states' => array(
      'visible' => array(
        ':input[name="parameter[payment_method][settings][payment_method][settings][usb_reader]"]' => array('checked' => TRUE),
      ),
    ),
  );

  $form['direct_post']['#weight'] = 4;
  $form['process_key']['#weight'] = 5;

  return $form;
}

/**
 * Payment method callback: checkout form.
 */
function commerce_iats_usb_reader_credit_card_submit_form($payment_method, $pane_values, $checkout_pane, $order) {
  module_load_include('inc', 'commerce_iats', 'includes/commerce_iats.credit_card');
  $form = commerce_iats_credit_card_submit_form($payment_method, $pane_values, $checkout_pane, $order);

  if (strstr($_SERVER['HTTP_REFERER'], '/admin/')) {
    if ($payment_method['settings']['usb_reader']) {
      $form['usb_reader'] = array(
        '#type' => 'checkbox',
        '#title' => 'Use encrypted USB card reader',
        '#default_value' => TRUE,
        '#weight' => 0,
      );

      $form['credit_card']['#type'] = 'fieldset';
      $form['credit_card']['#weight'] = 1;

      $form['credit_card']['number']['#required'] = FALSE;
      $form['credit_card']['exp_month']['#required'] = FALSE;

      $form['credit_card']['#states'] = array(
        'visible' => array(
          ':input[name="payment_details[usb_reader]"]' => array('checked' => FALSE),
        ),
      );

      $form['encrypted_credit_card'] = array(
        '#type' => 'fieldset',
        '#states' => array(
          'visible' => array(
            ':input[name="payment_details[usb_reader]"]' => array('checked' => TRUE),
          ),
        ),
      );

      $form['encrypted_credit_card']['status'] = array(
        '#type' => 'item',
        '#markup' => 'Swipe credit card now.',
      );

      $form['encrypted_credit_card']['encrypted_number'] = array(
        '#type' => 'textarea',
      );

      $form['#attached']['js'] = array(
        drupal_get_path('module', 'commerce_iats_usb_reader') . '/js/commerce_iats_usb_reader.credit_card.js',
      );
    }
  }

  return $form;
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
function commerce_iats_usb_reader_credit_card_submit_form_submit($payment_method, $pane_form, $pane_values, $order, $charge) {
  // Validate credit card charge.
  if (empty($charge['amount'])) {
    watchdog('commerce_iats', 'Empty charge amount: @charge for order @order', array(
      '@charge' => '<pre>' . check_plain(print_r($charge, TRUE)) . '</pre>',
      '@order' => '<pre>' . check_plain(print_r($order, TRUE)) . '</pre>',
    ), WATCHDOG_DEBUG);
    return FALSE;
  }

  $payment_data = $pane_values;

  return commerce_iats_process_payment($payment_method, $payment_data, $order,
    $charge, 'commerce_iats_usb_reader_process_credit_card_payment');
}

/**
 * Processes a credit card payment via the iATS Payments API using
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
function commerce_iats_usb_reader_process_credit_card_payment($payment_method, $payment_data, $order, $charge) {
  // Format the encrypted card data in the way iATS expects to receive it.
  // {Swiper type}|@|{Encrypted card data}
  $formatted_card_data = $payment_method['settings']['usb_reader_type']
    . COMMERCE_IATS_CARD_READER_DELIMITER
    . $payment_data['encrypted_credit_card']['encrypted_number'];

  // Create and populate the API request.
  $request = array(
    'customerIPAddress' => ip_address(),
    'invoiceNum' => t('@prefix@number', array(
      '@prefix' => $payment_method['settings']['invoice_prefix'],
      '@number' => $order->order_number,
    )),
    'creditCardNum' => $formatted_card_data,
    'total' => commerce_currency_amount_to_decimal($charge['amount'], $charge['currency_code']),
    'currency' => $charge['currency_code'],
    'mop' => strtoupper(commerce_iats_usb_reader_parse_card_type_from_encrypted_string(
      $payment_data['encrypted_credit_card']['encrypted_number'], $payment_data['credit_card']['valid_types'])),
    'comment' => '',
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
    commerce_iats_log_request($payment_method, 'commerce_iats_process_credit_card_payment', $request);
  }

  // Process the credit card payment.
  $response = $iats->processCreditCard($request);

  return $response;
}

/**
 * Parses a card type from the obfuscated card number contained in
 * an encrypted string.
 *
 * @param string $encrypted_string
 *   The encrypted string from a USB card reader.
 * @param array $valid_types
 *   An array of valid credit card types.
 *
 * @return Mixed
 *   The credit card type as a string or boolean FALSE.
 */
function commerce_iats_usb_reader_parse_card_type_from_encrypted_string($encrypted_string, $valid_types)
{
  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');

  // Match pattern:
  // ;4941********5889
  $matches = NULL;
  preg_match("/;\d{4}\*+\d{4}\=/", $encrypted_string, $matches);

  if (empty($matches))
  {
    preg_match("/;\d{16}\=/", $encrypted_string, $matches);
  }

  $card_number = substr(substr($matches[0], 1), 0, -1);

  return commerce_payment_validate_credit_card_type($card_number, $valid_types);
}
