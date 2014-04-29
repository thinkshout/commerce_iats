<?php
/**
 * @file
 * Checkout pane callback functions for recurring payments.
 */

/**
 * Checkout pane callback: Recurring payments settings form.
 */
function commerce_iats_recurring_pane_settings_form($checkout_pane) {
  $form = array();

  return $form;
}

/**
 * Checkout pane callback: Displays the recurring payment page.
 */
function commerce_iats_recurring_pane_checkout_form($form, &$form_state, $checkout_pane, $order) {
  $pane_form = array();

  $schedule_type = '';
  if (isset($form_state['values']['checkout_recurring']['schedule_type'])) {
    $schedule_type = $form_state['values']['checkout_recurring']['schedule_type'];
  }

  $pane_form['amount'] = array(
    '#type' => 'textfield',
    '#title' => t('Amount'),
    '#required' => TRUE,
  );

  $pane_form['schedule_type'] = array(
    '#type' => 'select',
    '#title' => t('Payment schedule type'),
    '#required' => TRUE,
    '#options' => commerce_iats_recurring_pane_schedule_types(),
    '#default_value' => '',
    '#ajax' => array(
      'callback' => 'commerce_iats_recurring_pane_schedule_type_callback',
      'method' => 'replace',
      'wrapper' => 'payment-date-wrapper',
    ),
  );

  if (!empty($schedule_type)) {
    $payment_dates = commerce_iats_recurring_pane_schedule_dates($schedule_type);

    $pane_form['schedule_date'] = array(
      '#type' => 'select',
      '#title' => t('Payment date'),
      '#options' => $payment_dates,
      '#default_value' => 1,
    );
  }
  else {
    $pane_form['schedule_date'] = array();
  }

  $pane_form['schedule_date']['#prefix'] = '<div id="payment-date-wrapper">';
  $pane_form['schedule_date']['#suffix'] = '</div>';

  return $pane_form;
}

/**
 * Checkout pane callback: Saves the recurring payment state.
 */
function commerce_iats_recurring_pane_checkout_form_submit($form, &$form_state, $checkout_pane, &$order) {
  $order->data['recurring'] = $form_state['values']['checkout_recurring'];
}

/**
 * Form callback when selecting a schedule type.
 */
function commerce_iats_recurring_pane_schedule_type_callback($form, $form_state) {
  return $form['checkout_recurring']['schedule_date'];
}

/**
 * Returns an associative array of payment schedule types.
 *
 * @return array
 *   Array of recurring schedule types.
 */
function commerce_iats_recurring_pane_schedule_types() {
  $types = array(
    '' => 'None',
    'Weekly' => 'Weekly',
    'Monthly' => 'Monthly',
    'Quarterly' => 'Quarterly',
    'Annually' => 'Annually',
  );
  return $types;
}

/**
 * Returns an associative array of payment schedule dates.
 *
 * @param string $payment_type
 *   The schedule type to return schedule dates for.
 *
 * @return array
 *   Array of schedule dates.
 */
function commerce_iats_recurring_pane_schedule_dates($payment_type) {
  switch ($payment_type) {
    case 'Weekly':
      $payment_dates = array(
        1 => 'Monday',
        2 => 'Tuesday',
        3 => 'Wednesday',
        4 => 'Thursday',
        5 => 'Friday',
        6 => 'Saturday',
        7 => 'Sunday',
      );
      break;

    case 'Monthly':
      $payment_dates = array();
      for ($i = 1; $i <= 31; $i++) {
        $payment_dates[$i] = $i;
      }
      break;

    default:
      // Payment types that do not require schedule dates.
      $payment_dates = array(
        '' => $payment_type,
      );
  }
  return $payment_dates;
}
