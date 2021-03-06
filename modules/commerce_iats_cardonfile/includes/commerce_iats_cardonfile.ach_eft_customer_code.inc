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
  $form = commerce_iats_cardonfile_customer_code_settings_form_base($settings, 'iats_ach_eft');

  // No invoice when creating customer codes.
  unset($form['invoice_prefix']);

  // No credit card options in ACH/EFT.
  unset($form['credit_card']);

  return $form;
}

/**
 * Payment method callback: checkout form.
 */
function commerce_iats_cardonfile_ach_eft_customer_code_submit_form($payment_method, $pane_values, $checkout_pane, $order) {
  if (isset($payment_method['settings']['direct_post']['enable_direct_post']) && $payment_method['settings']['direct_post']['enable_direct_post'] == 1) {
    module_load_include('inc', 'commerce_iats', 'includes/commerce_iats.direct_post.forms');
    module_load_include('inc', 'commerce_iats_cardonfile', 'includes/commerce_iats_cardonfile.direct_post.forms');
    return commerce_iats_cardonfile_direct_post_ach_eft_customer_code_form($payment_method, $order);
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
function commerce_iats_cardonfile_ach_eft_customer_code_submit_form_submit($payment_method, $pane_form, $pane_values, $order, $charge) {
  $payment_data = $pane_values;

  return commerce_iats_process_payment($payment_method, $payment_data, $order,
    $charge, 'commerce_iats_cardonfile_ach_eft_create_customer_code');
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
  module_load_include('inc', 'commerce_iats', 'includes/commerce_iats.ach_eft');

  $account_num = commerce_iats_ach_eft_currency_specific_account_number($payment_data['bank_account'], $charge['currency_code']);

  // Create and populate the API request.
  $request = array(
    'customerIPAddress' => ip_address(),
    'accountNum' => $account_num,
    'accountType' => isset($payment_data['bank_account']['type']) ? strtoupper($payment_data['bank_account']['type']) : '',
    'comment' => COMMERCE_IATS_COMMENT_PREFIX,
  );

  if (isset($payment_method['settings']['allow_recurring'])
    && ($payment_method['settings']['allow_recurring'] == 1)
    && isset($order->data['recurring'])) {
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
    // If the user has opted to store the card or created a recurring payment.
    if ($payment_data['bank_account']['cardonfile_store'] || $request['recurring']) {
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

        $account_type = (isset($payment_data['bank_account']['type'])) ?
          $payment_data['bank_account']['type'] : 'account';

        // Create a new card on file record for the new customer code.
        commerce_iats_cardonfile_create_cardonfile($order, $payment_method,
          $payment_data['bank_account'], $customer_code, $account_type);
      }
      else {
        drupal_set_message(t('Your account details could not be stored for later use.'), 'error');
      }
    }
  }

  return $response;
}
