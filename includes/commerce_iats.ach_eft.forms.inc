<?php
/**
 * @file
 * Forms for use in ACH/EFT payment processing.
 */

/**
 * Returns a set of bank account form elements.
 *
 * @param array $fields
 *   An array specifying the bank account fields that should be included on the form.
 * @param array $default
 *   An array of default values for the available bank account fields.
 */
function commerce_iats_ach_eft_payment_form($fields = array(), $default = array()) {
  $form = array(
    'bank_account' => array(
      '#tree' => TRUE,
    ),
  );

  $form['bank_account']['number'] = array(
    '#type' => 'textfield',
    '#title' => t('Account Number'),
    '#attributes' => array('autocomplete' => 'off'),
    '#required' => TRUE,
    '#maxlength' => 64,
    '#size' => 32,
  );

  $form['bank_account']['type'] = array(
    '#type' => 'select',
    '#title' => t('Account Type'),
    '#options' => commerce_iats_ach_eft_account_types(),
    '#required' => TRUE,
  );

  return $form;
}

/**
 * Validates a set of bank account details entered via the ACH/EFT form.
 *
 * @param array $details
 *   An array of bank account details as retrieved from the account array in
 *   the form values of a form containing the ACH/EFT form.
 * @param array $settings
 *   Settings used for calling validation functions and setting form errors:
 *   - form_parents: an array of parent elements identifying where the ACH/EFT
 *     card form was situated in the form array
 *
 * @return bool
 *   TRUE or FALSE indicating the validity of all the data.
 *
 * @see commerce_iats_ach_eft_payment_form()
 */
function commerce_iats_ach_eft_payment_form_validate($details, $settings) {
  // TODO Validation logic.
  return TRUE;
}

/**
 * Returns an associative array of valid bank account types for ACH/EFT payments.
 *
 * @return array
 *  Array of valid bank account types.
 */
function commerce_iats_ach_eft_account_types() {
  $types = array(
    'CHECKING' => 'Checking',
    'SAVING' => 'Saving',
  );
  return $types;
}