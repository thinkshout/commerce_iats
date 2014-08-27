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
    if (empty($pane_values['encrypted_credit_card']['encrypted_number'])) {
      form_set_error($prefix . 'encrypted_number', t('There was a problem processing the card via the USB card reader.'));
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
