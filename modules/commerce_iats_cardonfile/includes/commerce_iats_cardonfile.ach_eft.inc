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
  if (isset($payment_method['settings']['direct_post']['enable_direct_post']) && $payment_method['settings']['direct_post']['enable_direct_post'] == 1) {
    module_load_include('inc', 'commerce_iats', 'includes/commerce_iats.direct_post.forms');
    return commerce_iats_direct_post_ach_eft_form($payment_method, $order);
  }
  else {
    module_load_include('inc', 'commerce_iats', 'includes/commerce_iats.ach_eft.forms');

    $fields = array(
      'number' => '',
    );

    $currency_code = $order->commerce_order_total[LANGUAGE_NONE][0]['currency_code'];

    switch ($currency_code) {
      case 'USD':
        $form = commerce_iats_ach_eft_payment_form_usd($fields);
        break;

      case 'CAD':
        $form = commerce_iats_ach_eft_payment_form_cad($fields);
        break;

      case 'GBP':
        $form = commerce_iats_ach_eft_payment_form_gbp($fields);
        break;

      default:
        $form = commerce_iats_ach_eft_payment_form($fields);
    }

    return $form;
  }
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
    $order->revision = TRUE;
    $order->log = 'Order charge amount is empty.';
    commerce_order_save($order);

    watchdog('commerce_iats', 'Empty charge amount: @charge for order @order', array(
      '@charge' => '<pre>' . print_r($charge, TRUE) . '</pre>',
      '@order' => '<pre>' . print_r($order, TRUE) . '</pre>',
    ), WATCHDOG_DEBUG);
    return FALSE;
  }

  $payment_data = $pane_values;

  // If the user has opted to create a recurring payment during the checkout
  // process, create an iATS customer code with the recurring schedule
  // before processing the checkout purchase.
  if (isset($order->data['recurring']) && !empty($order->data['recurring']['schedule_type'])) {
    module_load_include('inc', 'commerce_iats_cardonfile', 'includes/commerce_iats_cardonfile.ach_eft_customer_code');

    $cc_payment_methods = commerce_iats_get_all_payment_methods('iats_ach_eft_customer_code');

    $recurring_payment_method = NULL;
    foreach ($cc_payment_methods as $cc_payment_method) {
      if (isset($cc_payment_method['settings']['allow_recurring']) && ($cc_payment_method['settings']['allow_recurring'] == 1)) {
        $recurring_payment_method = $cc_payment_method;
      }
    }

    if ($recurring_payment_method != NULL) {
      if (!commerce_iats_process_payment($recurring_payment_method, $payment_data, $order,
        NULL, 'commerce_iats_cardonfile_ach_eft_create_customer_code')) {
        drupal_set_message(t('We were unable to process your recurring payment at this time.'), 'error');
        return FALSE;
      }
    }
    else {
      watchdog('commerce_iats', 'Cannot create recurring payment. No valid payment method.', array(), WATCHDOG_WARNING);
      drupal_set_message(t('We were unable to process your recurring payment at this time.'), 'error');
      return FALSE;
    }
  }

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
  $invoice_number = $order->order_number;
  drupal_alter('commerce_iats_invoice_number', $invoice_number, $order);

  $request = array(
    'customerIPAddress' => ip_address(),
    'customerCode' => $card_data->remote_id,
    'invoiceNum' => t('@prefix@number', array(
      '@prefix' => $payment_method['settings']['invoice_prefix'],
      '@number' => $invoice_number,
    )),
    'total' => commerce_currency_amount_to_decimal($charge['amount'], $charge['currency_code']),
    'comment' => COMMERCE_IATS_COMMENT_PREFIX,
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
  module_load_include('inc', 'commerce_iats', 'includes/commerce_iats.ach_eft');

  $account_num = commerce_iats_ach_eft_currency_specific_account_number($payment_data['bank_account'], $charge['currency_code']);

  // Create and populate the API request.
  $invoice_number = $order->order_number;
  drupal_alter('commerce_iats_invoice_number', $invoice_number, $order);

  $request = array(
    'customerIPAddress' => ip_address(),
    'invoiceNum' => t('@prefix@number', array(
      '@prefix' => $payment_method['settings']['invoice_prefix'],
      '@number' => $invoice_number,
    )),
    'accountNum' => $account_num,
    'accountType' => isset($payment_data['bank_account']['type']) ? strtoupper($payment_data['bank_account']['type']) : '',
    'total' => commerce_currency_amount_to_decimal($charge['amount'], $charge['currency_code']),
    'comment' => COMMERCE_IATS_COMMENT_PREFIX,
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

      $expiration = commerce_iats_cardonfile_ach_eft_get_expiration();
      $payment_data['bank_account']['exp_year'] = $expiration['exp_year'];
      $payment_data['bank_account']['exp_month'] = $expiration['exp_month'];

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
 * Creates an expiration date 10 years in the future.
 *
 * Bank accounts do not expire, but Commerce Card on File requires an
 * expiration date to allow a bank account to be used.
 */
function commerce_iats_cardonfile_ach_eft_get_expiration() {
  $expiration = array(
    'exp_year' => (int) date('Y') + 10,
    'exp_month' => (int) date('m'),
  );

  return $expiration;
}
