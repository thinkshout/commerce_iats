<?php
/**
 * @file
 * Administrative forms for the Commerce iATS module.
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