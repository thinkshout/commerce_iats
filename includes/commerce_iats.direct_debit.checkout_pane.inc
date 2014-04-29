<?php
/**
 * @file
 * Checkout pane callback functions for direct debit payments.
 */

/**
 * Checkout pane callback: Direct debit declaration page settings form.
 */
function commerce_iats_direct_debit_declaration_pane_settings_form($checkout_pane) {
  $form = array();

  $form['commerce_iats_direct_debit_declaration_pane_paper_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Paper Direct Debit Instruction URL'),
    '#description' => t('The URL a user will be given to print a paper Direct Debit Instruction, if needed.'),
    '#size' => 64,
    '#default_value' => variable_get('commerce_iats_direct_debit_declaration_pane_paper_url', ''),
  );

  $form['commerce_iats_direct_debit_declaration_pane_phone'] = array(
    '#type' => 'textfield',
    '#title' => t('Contact Phone Number'),
    '#size' => 32,
    '#default_value' => variable_get('commerce_iats_direct_debit_declaration_pane_phone', ''),
  );

  $form['commerce_iats_direct_debit_declaration_pane_email'] = array(
    '#type' => 'textfield',
    '#title' => t('Contact Email Address'),
    '#size' => 32,
    '#default_value' => variable_get('commerce_iats_direct_debit_declaration_pane_email', ''),
  );

  return $form;
}

/**
 * Checkout pane callback: Displays the direct debit declaration page.
 */
function commerce_iats_direct_debit_declaration_pane_checkout_form($form, &$form_state, $checkout_pane, $order) {
  $pane_form = array();

  $paper_url = variable_get('commerce_iats_direct_debit_declaration_pane_paper_url', '');

  $info = '<p><b>Note:</b> If you are not the account holder or your account'
    . ' requires more than one signature a paper Direct Debit Instruction will'
    . ' be required to be completed and posted to us.'
    . ' <a href="'. $paper_url . '">Click here</a> to print off a paper'
    . ' Direct Debit Instruction.</p>'
    . '<p>OR</p><p>Continue with the details below:</p>';

  $terms = '<p><b>Note:</b> All the normal Direct Debit safeguards and'
    . ' guarantees apply. No changes in the amount, date, frequency to be'
    . ' debited can be made witout notifying you at least five (5) working'
    . ' days in advance of your account being debited. In the event of any'
    . ' error, you are entitled to an immediate refund from your Bank or'
    . ' Building Society. You have the right to cancel a Direct Debit'
    . ' Instruction at any time simply by writing to your Bank or Building'
    . ' Society, with a copy to us.</p>';

  $pane_form['info'] = array(
    '#type' => 'item',
    '#markup' => $info,
  );

  $pane_form['declaration'] = array(
    '#type' => 'fieldset',
    '#title' => t('Declaration'),
  );

  $pane_form['declaration']['confirmation'] = array(
    '#type' => 'checkbox',
    '#title' => t('I wish to start a Direct Debit'),
    '#required' => TRUE,
  );

  $pane_form['terms'] = array(
    '#type' => 'fieldset'
  );

  $pane_form['terms']['text'] = array(
    '#type' => 'item',
    '#markup' => $terms,
  );

  $pane_form['terms']['phone'] = array(
    '#type' => 'item',
    '#markup' => t('Call us: @phone', array(
      '@phone' => variable_get('commerce_iats_direct_debit_declaration_pane_phone', ''),
    )),
  );

  $pane_form['terms']['email'] = array(
    '#type' => 'item',
    '#markup' => t('Email us: @email', array(
      '@email' => variable_get('commerce_iats_direct_debit_declaration_pane_email', ''),
    )),
  );

  return $pane_form;
}

/**
 * Checkout pane callback: Saves the direct debit declaration status.
 */
function commerce_iats_direct_debit_declaration_pane_checkout_form_submit($form, &$form_state, $checkout_pane, &$order) {
  $order->data['declaration'] = $form_state['values']['checkout_direct_debit_declaration'];
}

/**
 * Checkout pane callback: Direct debit account / payer settings form.
 */
function commerce_iats_direct_debit_account_payer_pane_settings_form($checkout_pane) {
  $form = array();

  $form['commerce_iats_direct_debit_account_payer_pane_phone'] = array(
    '#type' => 'textfield',
    '#title' => t('Contact Phone Number'),
    '#size' => 32,
    '#default_value' => variable_get('commerce_iats_direct_debit_account_payer_pane_phone', ''),
  );

  $form['commerce_iats_direct_debit_account_payer_pane_email'] = array(
    '#type' => 'textfield',
    '#title' => t('Contact Email Address'),
    '#size' => 32,
    '#default_value' => variable_get('commerce_iats_direct_debit_account_payer_pane_email', ''),
  );

  $form['commerce_iats_direct_debit_account_payer_first_collection_option_count'] = array(
    '#type' => 'textfield',
    '#title' => t('First Collection Option Count'),
    '#description' => t('The number of first collection date options to offer the user.'),
    '#size' => 4,
    '#default_value' => variable_get('commerce_iats_direct_debit_account_payer_first_collection_option_count', '5'),
  );

  return $form;
}

/**
 * Checkout pane callback: Displays the direct debit account / payer page.
 */
function commerce_iats_direct_debit_account_payer_pane_checkout_form($form, &$form_state, $checkout_pane, $order) {
  $pane_form = array();
  $default_values = isset($order->data['account_payer']) ?
    $order->data['account_payer'] : array();

  $pane_form['account'] = array(
    '#type' => 'fieldset',
    '#title' => t('Account Details'),
  );

  $pane_form['account']['account_holder'] = array(
    '#type' => 'textfield',
    '#title' => t('Account Holder Name'),
    '#default_value' => '',
    '#attributes' => array('autocomplete' => 'off'),
    '#required' => TRUE,
    '#maxlength' => 64,
    '#size' => 32,
  );

  $pane_form['account']['number'] = array(
    '#type' => 'textfield',
    '#title' => t('Account Number'),
    '#attributes' => array('autocomplete' => 'off'),
    '#required' => TRUE,
    '#maxlength' => 64,
    '#size' => 32,
  );

  $pane_form['account']['sort_code'] = array(
    '#type' => 'textfield',
    '#title' => t('Sort Code'),
    '#attributes' => array('autocomplete' => 'off'),
    '#required' => TRUE,
    '#maxlength' => 64,
    '#size' => 32,
  );

  $pane_form['account']['frequency_type'] = array(
    '#type' => 'item',
    '#title' => t('Frequency Type'),
    '#markup' => $order->data['recurring']['schedule_type'],
  );

  $pane_form['account']['amount'] = array(
    '#type' => 'item',
    '#title' => t('Amount'),
    '#markup' => $order->data['recurring']['amount'],
  );

  $pane_form['account']['first_collection_date'] = array(
    '#type' => 'select',
    '#title' => t('Date Of First Collection'),
    '#options' => commerce_iats_direct_debit_get_first_collection_options(
      $order->data['recurring']['schedule_type'],
      $order->data['recurring']['schedule_date']),
  );

  $pane_form['payer'] = array(
    '#type' => 'fieldset',
    '#title' => t('Payer Details'),
  );

  $pane_form['payer']['company_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Company Name'),
    '#maxlength' => 64,
    '#size' => 32,
    '#default_value' => isset($default_values['payer']['company_name']) ?
    $default_values['payer']['company_name'] : '',
  );

  $pane_form['payer']['first_name'] = array(
    '#type' => 'textfield',
    '#title' => t('First Name(s)'),
    '#required' => TRUE,
    '#maxlength' => 64,
    '#size' => 32,
    '#default_value' => isset($default_values['payer']['first_name']) ?
    $default_values['payer']['first_name'] : '',
  );

  $pane_form['payer']['last_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Last Name'),
    '#required' => TRUE,
    '#maxlength' => 64,
    '#size' => 32,
    '#default_value' => isset($default_values['payer']['last_name']) ?
    $default_values['payer']['last_name'] : '',
  );

  $pane_form['payer']['address_1'] = array(
    '#type' => 'textfield',
    '#title' => t('Address 1'),
    '#required' => TRUE,
    '#maxlength' => 64,
    '#size' => 32,
    '#default_value' => isset($default_values['payer']['address_1']) ?
    $default_values['payer']['address_1'] : '',
  );

  $pane_form['payer']['address_2'] = array(
    '#type' => 'textfield',
    '#title' => t('Address 2'),
    '#maxlength' => 64,
    '#size' => 32,
    '#default_value' => isset($default_values['payer']['address_2']) ?
    $default_values['payer']['address_2'] : '',
  );

  $pane_form['payer']['address_3'] = array(
    '#type' => 'textfield',
    '#title' => t('Address 3'),
    '#maxlength' => 64,
    '#size' => 32,
    '#default_value' => isset($default_values['payer']['address_3']) ?
    $default_values['payer']['address_3'] : '',
  );

  $pane_form['payer']['town'] = array(
    '#type' => 'textfield',
    '#title' => t('Town'),
    '#required' => TRUE,
    '#maxlength' => 64,
    '#size' => 32,
    '#default_value' => isset($default_values['payer']['town']) ?
    $default_values['payer']['town'] : '',
  );

  $pane_form['payer']['county'] = array(
    '#type' => 'textfield',
    '#title' => t('County'),
    '#maxlength' => 64,
    '#size' => 32,
    '#default_value' => isset($default_values['payer']['county']) ?
    $default_values['payer']['county'] : '',
  );

  $pane_form['payer']['postcode'] = array(
    '#type' => 'textfield',
    '#title' => t('Postcode'),
    '#required' => TRUE,
    '#maxlength' => 64,
    '#size' => 32,
    '#default_value' => isset($default_values['payer']['postcode']) ?
    $default_values['payer']['postcode'] : '',
  );

  $pane_form['payer']['email'] = array(
    '#type' => 'textfield',
    '#title' => t('Email Address'),
    '#required' => TRUE,
    '#maxlength' => 64,
    '#size' => 32,
    '#default_value' => isset($default_values['payer']['email']) ?
    $default_values['payer']['email'] : '',
  );

  $pane_form['payer']['confirm_email'] = array(
    '#type' => 'textfield',
    '#title' => t('Retype Email Address'),
    '#required' => TRUE,
    '#maxlength' => 64,
    '#size' => 32,
    '#default_value' => '',
    '#attributes' => array('autocomplete' => 'off'),
  );

  $pane_form['contact'] = array(
    '#type' => 'fieldset'
  );

  $pane_form['contact']['phone'] = array(
    '#type' => 'item',
    '#markup' => t('Call us: @phone', array(
      '@phone' => variable_get('commerce_iats_direct_debit_account_payer_pane_phone', ''),
    )),
  );

  $pane_form['contact']['email'] = array(
    '#type' => 'item',
    '#markup' => t('Email us: @email', array(
      '@email' => variable_get('commerce_iats_direct_debit_account_payer_pane_email', ''),
    )),
  );

  return $pane_form;
}

/**
 * Checkout pane callback: Validates the account / payer details.
 */
function commerce_iats_direct_debit_account_payer_pane_checkout_form_validate($form, &$form_state, $checkout_pane, &$order) {
  if ($form_state['values']['checkout_direct_debit_account_payer']['payer']['email'] !=
    $form_state['values']['checkout_direct_debit_account_payer']['payer']['confirm_email']) {
    return FALSE;
  }

  return TRUE;
}

/**
 * Checkout pane callback: Saves the account / payer details.
 */
function commerce_iats_direct_debit_account_payer_pane_checkout_form_submit($form, &$form_state, $checkout_pane, &$order) {
  $order->data['account_payer'] = $form_state['values']['checkout_direct_debit_account_payer'];

  // TODO: Process Direct Debit Validate payment method.

  // TODO: Account for custom rule name.
  $payment_method_name = 'direct_debit_validation|commerce_payment_direct_debit_validation';

  // Only process if there were payment methods available.
  $order->data['payment_method'] = $payment_method_name;

  // Delegate submit to the payment method callback.
  $payment_method = commerce_payment_method_instance_load($payment_method_name);

  if ($callback = commerce_payment_method_callback($payment_method, 'submit_form_submit')) {
    $payment_data = $form_state['values']['checkout_direct_debit_account_payer'];
    // No balance required for direct debit validation.
    $balance = 0;
    if ($callback($payment_method, $form, $payment_data, $order, $balance) === FALSE) {
      // If payment fails, rebuild the checkout form without progressing.
      $form_state['rebuild'] = TRUE;
    }
  }

  // TODO: Get bank details.

}

/**
 * Checkout pane callback: Direct debit validation settings form.
 */
function commerce_iats_direct_debit_validation_pane_settings_form($checkout_pane) {
  $form = array();

  $form['commerce_iats_direct_debit_validation_pane_company_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Company Name'),
    '#size' => 32,
    '#default_value' => variable_get('commerce_iats_direct_debit_validation_pane_company_name', ''),
  );

  $form['company_address'] = array(
    '#type' => 'fieldset',
    '#title' => t('Company Address'),
  );

  $form['company_address']['commerce_iats_direct_debit_validation_pane_address'] = array(
    '#type' => 'textfield',
    '#title' => t('Address'),
    '#size' => 32,
    '#default_value' => variable_get('commerce_iats_direct_debit_validation_pane_address', ''),
  );

  $form['company_address']['commerce_iats_direct_debit_validation_pane_city'] = array(
    '#type' => 'textfield',
    '#title' => t('City'),
    '#size' => 32,
    '#default_value' => variable_get('commerce_iats_direct_debit_validation_pane_city', ''),
  );

  $form['company_address']['commerce_iats_direct_debit_validation_pane_state'] = array(
    '#type' => 'textfield',
    '#title' => t('State'),
    '#size' => 32,
    '#default_value' => variable_get('commerce_iats_direct_debit_validation_pane_state', ''),
  );

  $form['company_address']['commerce_iats_direct_debit_validation_pane_country'] = array(
    '#type' => 'textfield',
    '#title' => t('Country'),
    '#size' => 32,
    '#default_value' => variable_get('commerce_iats_direct_debit_validation_pane_country', ''),
  );

  $form['company_address']['commerce_iats_direct_debit_validation_pane_postcode'] = array(
    '#type' => 'textfield',
    '#title' => t('ZIP / Post Code'),
    '#size' => 32,
    '#default_value' => variable_get('commerce_iats_direct_debit_validation_pane_postcode', ''),
  );

  return $form;
}

/**
 * Checkout pane callback: Displays the direct debit validation details page.
 */
function commerce_iats_direct_debit_validation_pane_checkout_form($form, &$form_state, $checkout_pane, $order) {
  $pane_form = array();

  $confirmation = '<p>These are the details that you have entered to set up'
    . ' Direct Debit Instruction.</p><p><b>Important:</b> In order to complete'
    . ' the Direct Debit set-up, please check the following details and click'
    . ' the "Continue" button lower down this page.</p>';

  $pane_form['confirmation'] = array(
    '#type' => 'fieldset',
    '#title' => t('Confirmation'),
  );

  $pane_form['confirmation']['text'] = array(
    '#type' => 'item',
    '#markup' => $confirmation,
  );

  $pane_form['title'] = array(
    '#type' => 'item',
    '#markup' => t('Instruction to your Bank or Building Society to pay by Direct Debit'),
  );

  $pane_form['company'] = array(
    '#type' => 'fieldset',
  );

  $pane_form['company']['name'] = array(
    '#type' => 'item',
    '#markup' => variable_get('commerce_iats_direct_debit_validation_pane_company_name', ''),
  );

  $pane_form['company']['address'] = array(
    '#type' => 'item',
    '#markup' => variable_get('commerce_iats_direct_debit_validation_pane_address', ''),
  );

  $pane_form['company']['city'] = array(
    '#type' => 'item',
    '#markup' => variable_get('commerce_iats_direct_debit_validation_pane_city', ''),
  );

  $pane_form['company']['state'] = array(
    '#type' => 'item',
    '#markup' => variable_get('commerce_iats_direct_debit_validation_pane_state', ''),
  );

  $pane_form['company']['country'] = array(
    '#type' => 'item',
    '#markup' => variable_get('commerce_iats_direct_debit_validation_pane_country', ''),
  );

  $pane_form['company']['postcode'] = array(
    '#type' => 'item',
    '#markup' => variable_get('commerce_iats_direct_debit_validation_pane_postcode', ''),
  );

  $pane_form['details'] = array(
    '#type' => 'fieldset',
  );

  $pane_form['details']['account_holder'] = array(
    '#type' => 'item',
    '#title' => t('Name of Account Holder'),
    '#markup' => $order->data['account_payer']['account']['account_holder'],
  );

  $pane_form['details']['account_number'] = array(
    '#type' => 'item',
    '#title' => t('Bank / Building Society Account Number'),
    '#markup' => $order->data['account_payer']['account']['number'],
  );

  $pane_form['details']['sort_code'] = array(
    '#type' => 'item',
    '#title' => t('Branch Sort Code'),
    '#markup' => $order->data['account_payer']['account']['sort_code'],
  );

  $pane_form['details']['name_address'] = array(
    '#type' => 'item',
    '#title' => t('Name and full postal address of your Bank or Building Society'),
    '#markup' => $order->data['account_payer']['account']['sort_code'],
  );

  $pane_form['details']['service_user_number'] = array(
    '#type' => 'item',
    '#title' => t('Service User Number'),
    // TODO: Get Service User Number.
    '#markup' => '',
  );

  $pane_form['details']['reference_number'] = array(
    '#type' => 'item',
    '#title' => t('Reference'),
    // TODO: Get Reference Number.
    '#markup' => '',
  );

  $pane_form['details']['instruction'] = array(
    '#type' => 'fieldset',
  );

  $instruction_info = '<p><b>Instructions to your Bank or Building Society:</b>'
    . ' Please pay @company_name from the account detailed in this instruction'
    . ' subject to the safeguards assured by the Direct Debit Guarantee.'
    . ' I understand that this instruction may remain with @company_name and,'
    . ' if so, details will be passed electronically to my Bank/Building Society.';

  $pane_form['details']['instruction']['info'] = array(
    '#type' => 'item',
    '#markup' => t($instruction_info,
    array(
      '@company_name' => variable_get('commerce_iats_direct_debit_validation_pane_company_name', ''),
    ))
  );

  $pane_form['details']['instruction']['value'] = array(
    '#type' => 'item',
    // TODO: Get First Collection Date.
    '#markup' => '',
  );

  return $pane_form;
}

/**
 * Checkout pane callback: Direct debit summary page settings form.
 */
function commerce_iats_direct_debit_summary_pane_settings_form($checkout_pane) {
  $form = array();

  $form['commerce_iats_direct_debit_summary_pane_company_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Company Name'),
    '#description' => t('The company name to use in the Direct Debit Instruction.'),
    '#size' => 64,
    '#default_value' => variable_get('commerce_iats_direct_debit_summary_pane_company_name', ''),
  );

  return $form;
}

/**
 * Checkout pane callback: Displays the direct debit summary page.
 */
function commerce_iats_direct_debit_summary_pane_checkout_form($form, &$form_state, $checkout_pane, $order) {
  $pane_form = array();
  $company_name = variable_get('commerce_iats_direct_debit_summary_pane_company_name', '');

  $info = '<p>A summary of the details of your submission are listed below.</p>'
    . '<p>Confirmation of the Direct Debit set up will be sent by email'
    . ' within 3 working days or no later than 10 working days before'
    . ' first collection.</p>';

  $pane_form['summary'] = array(
    '#type' => 'fieldset',
    '#title' => t('Summary Details Entered'),
  );

  $pane_form['summary']['text'] = array(
    '#type' => 'item',
    '#markup' => $info,
  );

  $pane_form['service_user'] = array(
    '#type' => 'fieldset',
    '#title' => t('Service User'),
  );

  $pane_form['service_user']['name'] = array(
    '#type' => 'item',
    '#title' => t('Name'),
    '#markup' => $company_name,
  );

  $pane_form['service_user']['address'] = array(
    '#type' => 'item',
    '#title' => t('Address'),
    // TODO: Get service user address.
    '#markup' => '',
  );

  $pane_form['service_user']['reference_number'] = array(
    '#type' => 'item',
    '#title' => t('Reference Number'),
    // TODO: Get user reference number.
    '#markup' => '',
  );

  $pane_form['service_user']['company_info'] = array(
    '#type' => 'item',
    '#markup' => t('The company name that will appear on your bank statement against the Direct Debit will be "@company_name"',
      array(
        '@company_name' => $company_name,
      )),
  );

  $pane_form['amount_details'] = array(
    '#type' => 'fieldset',
    '#title' => t('Amount Details'),
  );

  $pane_form['amount_details']['amount'] = array(
    '#type' => 'item',
    '#title' => t('Amount'),
    '#markup' => $order->data['recurring']['amount'],
  );

  $pane_form['amount_details']['frequency'] = array(
    '#type' => 'item',
    '#title' => t('Frequency'),
    '#markup' => $order->data['recurring']['schedule_type'],
  );

  $pane_form['amount_details']['first_collection_date'] = array(
    '#type' => 'item',
    '#title' => t('Date of first collection'),
    // TODO: Get first collection date.
    '#markup' => '',
  );

  $pane_form['payer_details'] = array(
    '#type' => 'fieldset',
    '#title' => t('Payer Details'),
  );

  $pane_form['payer_details']['first_name'] = array(
    '#type' => 'item',
    '#title' => t('First Name'),
    '#markup' => $order->data['account_payer']['payer']['first_name'],
  );

  $pane_form['payer_details']['last_name'] = array(
    '#type' => 'item',
    '#title' => t('Last Name'),
    '#markup' => $order->data['account_payer']['payer']['last_name'],
  );

  $address = $order->data['account_payer']['payer']['address_1'] . '<br />'
    . $order->data['account_payer']['payer']['address_2'] . '<br />'
    . $order->data['account_payer']['payer']['address_3'] . '<br />'
    . $order->data['account_payer']['payer']['town'] . '<br />'
    . $order->data['account_payer']['payer']['county'] . '<br />'
    . $order->data['account_payer']['payer']['postcode'];

  $pane_form['payer_details']['address'] = array(
    '#type' => 'item',
    '#title' => t('Address'),
    '#markup' => $address,
  );

  $pane_form['payer_details']['email_address'] = array(
    '#type' => 'item',
    '#title' => t('Email Address'),
    '#markup' => $order->data['account_payer']['payer']['email'],
  );

  $pane_form['account_details'] = array(
    '#type' => 'fieldset',
    '#title' => t('Account Details'),
  );

  $pane_form['account_details']['name'] = array(
    '#type' => 'item',
    '#title' => t('Account Name'),
    '#markup' => $order->data['account_payer']['account']['account_holder'],
  );

  $pane_form['account_details']['number'] = array(
    '#type' => 'item',
    '#title' => t('Account Number'),
    '#markup' => $order->data['account_payer']['account']['number'],
  );

  $pane_form['account_details']['sort_code'] = array(
    '#type' => 'item',
    '#title' => t('Account Sort Code'),
    '#markup' => $order->data['account_payer']['account']['sort_code'],
  );

  return $pane_form;
}

/**
 * Checkout pane callback: Saves the account / payer details.
 */
function commerce_iats_direct_debit_summary_pane_checkout_form_submit($form, &$form_state, $checkout_pane, &$order) {
  $order->data['summary'] = $form_state['values']['checkout_direct_debit_summary'];
}

/**
 * Checkout pane callback: Displays the direct debit set up complete page.
 */
function commerce_iats_direct_debit_complete_pane_checkout_form($form, &$form_state, $checkout_pane, $order) {
  $pane_form = array();

  $pane_form['complete'] = array(
    '#type' => 'fieldset',
    '#title' => t('Completed'),
  );

  $pane_form['complete']['text'] = array(
    '#type' => 'item',
    '#markup' => t('Your Direct Debit Reference Number is @reference_number',
      array(
        // TODO: Get reference number.
        '@reference_number' => '',
      )),
  );

  return $pane_form;
}

/**
 * Gets an array of first collection date options, indexed by UNIX timestamp.
 *
 * @param string $schedule_type
 *   The iATS Schedule Type.
 * @param string $schedule_date
 *   The iATS Schedule Date.
 *
 * @see commerce_iats_recurring_pane_schedule_types
 * @see commerce_iats_recurring_pane_schedule_dates
 *
 * @return array
 *   Array of first collection date options.
 */
function commerce_iats_direct_debit_get_first_collection_options($schedule_type, $schedule_date) {
  $option_count = (int) variable_get('commerce_iats_direct_debit_account_payer_first_collection_option_count', 5);
  $schedule_date = (int) $schedule_date;

  $dates = array();
  switch ($schedule_type) {
    case 'Weekly':
      // Start date options on next chosen day of week after lead time,
      // repeat weekly.
      $current_week_day = (int) date('N', mktime(0, 0, 0, date('n'), date('j'), date('y')));

      $day_offset = ($current_week_day != $schedule_date) ?
        ($schedule_date - $current_week_day) : 0;
      // Find the next weekday that matches the chosen day of week.
      $initial_time = mktime(0, 0, 0, date('n'), date('j') + $day_offset, date('y'));

      for ($i = 0; $i < $option_count; $i++) {
        $time = mktime(0, 0, 0, date('n', $initial_time),
          date('j', $initial_time) + ($i * 7), date('y', $initial_time));
        if (!commerce_iats_direct_debit_validate_lead_time($time)) {
          $option_count++;
          continue;
        }
        $dates[$time] = commerce_iats_direct_debit_get_formatted_date($time);
      }
      break;

    case 'Monthly':
      // Start date options on next chosen day of month after lead time,
      // repeat monthly.
      for ($i = 0; $i < $option_count; $i++) {
        $time = mktime(0, 0, 0, date('n') + $i, $schedule_date, date('y'));
        if (!commerce_iats_direct_debit_validate_lead_time($time)) {
          $option_count++;
          continue;
        }
        $dates[$time] = commerce_iats_direct_debit_get_formatted_date($time);
      }
      break;

    case 'Quarterly':
      // Start date options on next quarter after lead time, repeat quarterly.
      for ($i = 0; $i < $option_count; $i++) {
        $time = mktime(0, 0, 0, date('n') + ($i * 4),
          date('j') + COMMERCE_IATS_DIRECT_DEBIT_LEAD_DAYS, date('y'));
        if (!commerce_iats_direct_debit_validate_lead_time($time)) {
          $option_count++;
          continue;
        }
        $dates[$time] = commerce_iats_direct_debit_get_formatted_date($time);
      }
      break;

    case 'Annually':
      // Start date options after lead time, repeat annually.
      for ($i = 0; $i < $option_count; $i++) {
        $time = mktime(0, 0, 0, date('n'),
          date('j') + COMMERCE_IATS_DIRECT_DEBIT_LEAD_DAYS, date('y') + $i);
        if (!commerce_iats_direct_debit_validate_lead_time($time)) {
          $option_count++;
          continue;
        }
        $dates[$time] = commerce_iats_direct_debit_get_formatted_date($time);
      }
      break;
  }

  return $dates;
}

/**
 * Determines if a UNIX timestamp provides valid lead time for direct debit.
 *
 * @param int $time
 *   UNIX timestamp to test.
 *
 * @return bool
 *   TRUE if timestamp provides valid lead time.
 */
function commerce_iats_direct_debit_validate_lead_time($time) {
  $seconds_in_day = (60 * 60 * 24);
  $time_today = mktime(0, 0, 0, date('n'), date('j'), date('y'));

  return ($time >= ($time_today + ($seconds_in_day * COMMERCE_IATS_DIRECT_DEBIT_LEAD_DAYS)));
}

/**
 * Gets a formatted date string for a given UNIX timestamp.
 *
 * @param int $time
 *   The UNIX timestamp to convert to a date string.
 *
 * @return string
 *   The formatted date string.
 */
function commerce_iats_direct_debit_get_formatted_date($time) {
  return date('l, F jS, Y', $time);
}
