<?php
/**
 * @file
 * Common forms for use with USB card reader transactions.
 */

/**
 * Gets base settings form fields for credit card based payment methods
 * using a USB card reader.
 *
 * @param array $settings
 *   Form field default values array.
 *
 * @return array
 *   Base fields for settings form.
 */
function commerce_iats_usb_reader_credit_card_settings_form_base(array $settings = NULL) {
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
    '#default_value' => isset($settings['usb_reader_type']) ? $settings['usb_reader_type'] : NULL,
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
 * Gets base form fields for credit card transactions using a USB card reader.
 */
function commerce_iats_usb_reader_credit_card_form_base($payment_method, $pane_values, $checkout_pane, $order) {
  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');

  // Prepare the fields to include on the credit card form.
  $fields = array(
    'code' => '',
    'type' => commerce_iats_enabled_credit_card_types($payment_method),
  );

  $form = commerce_payment_credit_card_form($fields);

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
      $form['credit_card']['code']['#required'] = FALSE;

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