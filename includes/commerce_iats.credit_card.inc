<?php
/**
 * @file
 * Implements iATS credit card payment services for use in Drupal Commerce.
 */

/**
 * Implements hook_commerce_payment_method_info().
 */
function commerce_iats_commerce_payment_method_info() {
  $payment_methods = array();

  $payment_methods['iats_credit_card'] = array(
    'base' => 'commerce_iats_credit_card',
    'title' => t('iATS Webservice: Credit Card'),
    'short_title' => t('iATS Credit Card'),
    'display_title' => t('Credit Card'),
    'description' => t('Integrates the iATS webservice for credit card payments.'),
    'cardonfile' => array(
      'charge callback' => 'commerce_iats_cardonfile_charge',
      'update callback' => 'commerce_iats_cardonfile_update',
      'delete callback' => 'commerce_iats_cardonfile_delete',
    ),
  );
}

/**
 * Payment method callback: settings form.
 */
function commerce_iats_credit_card_settings_form($settings = NULL) {

}

/**
 * Payment method callback: checkout form.
 */
function commerce_iats_credit_card_submit_form($payment_method, $pane_values, $checkout_pane, $order) {

}

/**
 * Payment method callback: checkout form validation.
 */
function commerce_iats_credit_card_submit_form_validate($payment_method, $pane_form, $pane_values, $order, $form_parents = array()) {

}

/**
 * Payment method callback: checkout form submission.
 */
function commerce_iats_credit_card_submit_form_submit($payment_method, $pane_form, $pane_values, $order, $charge) {

}



