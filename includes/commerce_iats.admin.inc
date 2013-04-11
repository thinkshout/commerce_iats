<?php

/**
 * @file
 * Administrative forms for the IATS module.
 */

/**
 * Form callback: allows the user to issue a refund on a prior transaction.
 */
function commerce_iats_refund_form($form, &$form_state, $order, $transaction) {
  $form_state['order'] = $order;
  $form_state['transaction'] = $transaction;

  // Load and store the payment method instance for this transaction.
  $payment_method = commerce_payment_method_instance_load($transaction->instance_id);
  $form_state['payment_method'] = $payment_method;

  $default_amount = number_format(commerce_currency_amount_to_decimal($transaction->amount, $transaction->currency_code), 2, '.', '');

  $form['amount'] = array(
    '#type' => 'textfield',
    '#title' => t('Refund amount'),
    '#description' => t('Enter the amount to be refunded back to the original card.'),
    '#default_value' => $default_amount,
    '#field_suffix' => check_plain($transaction->currency_code),
    '#size' => 16,
  );

  $form = confirm_form($form,
    t('What amount do you want to refund?'),
    'admin/commerce/orders/' . $order->order_id . '/payment',
    '',
    t('Refund'),
    t('Cancel'),
    'confirm'
  );

  return $form;
}

/**
 * Validate handler: check the refund amount before attempting refund request.
 */
function commerce_iats_refund_form_validate($form, &$form_state) {
  $transaction = $form_state['transaction'];
  $amount = $form_state['values']['amount'];

  // Ensure a positive numeric amount has been entered for refund.
  if (!is_numeric($amount) || $amount <= 0) {
    form_set_error('amount', t('You must specify a positive numeric amount to refund.'));
  }

  // Ensure the amount is less than or equal to the captured amount.
  if ($amount > commerce_currency_amount_to_decimal($transaction->amount, $transaction->currency_code)) {
    form_set_error('amount', t('You cannot refund more than you captured through IATS.'));
  }

  // If the transaction is older than 120 days, display an error message and redirect.
  if (time() - $transaction->created > 86400 * 120) {
    drupal_set_message(t('This capture has passed its 120 day limit for issuing refunds.'), 'error');
    drupal_goto('admin/commerce/orders/' . $form_state['order']->order_id . '/payment');
  }
}

/**
 * Submit handler: process a refund via IATS.
 */
function commerce_iats_refund_form_submit($form, &$form_state) {
  $transaction = $form_state['transaction'];
  $amount = number_format($form_state['values']['amount'], 2, '.', '');
  $order = $form_state['order'];
  $payment_method = $form_state['payment_method'];

  $payment_method['settings'] += commerce_iats_soap_process_default_settings();

  $iats_method = $payment_method['ProcessCreditCardRefundWithTransactionId'];

  // Create and populate the request object.
  $request = (object) array(
    'total' => $amount * -1,
    'customerIPAddress' => ip_address(),
    'transactionId' => $transaction->remote_id,
    'comment' => t('Refund on order') . ' ' . $order->order_number,
  );

  if ($response = commerce_iats_soap_api_request($payment_method, $request, $iats_method)) {
    $credit_amount = commerce_currency_decimal_to_amount($amount, $transaction->currency_code);
    $processresult = $response->PROCESSRESULT;
    $auth_result = trim(current($processresult->AUTHORIZATIONRESULT));
    // Prepare a transaction object to log the API response.
    $refund_transaction = commerce_payment_transaction_new('iats_soap_process', $order->order_id);
    $refund_transaction->instance_id = $payment_method['instance_id'];
    $refund_transaction->remote_id = trim(current($processresult->TRANSACTIONID));
    $refund_transaction->amount = $credit_amount;
    $refund_transaction->currency_code = $transaction->currency_code;
    // $transaction->payload[REQUEST_TIME] = $response;
    // If we didn't get an approval response code...
    // Note: do not use SUCCESS property, which just means IATS said "hello"
    if (substr($auth_result, 0, 2) != IATS_TXN_OK) {
      // Create a failed transaction with the error message.
      $refund_transaction->status = COMMERCE_PAYMENT_STATUS_FAILURE;
    }
    else {
      $refund_transaction->status = COMMERCE_PAYMENT_STATUS_SUCCESS;
    }

    // Build a meaningful response message.
    $message = array(
      '<b>' . t('Credit Card Refund') . '</b>',
      '<b>' . t('@decision:', array('@decision' => $auth_result)) . '</b>',
    );

    $refund_transaction->message = implode('<br />', $message);

    // Save the transaction information.
    commerce_payment_transaction_save($refund_transaction);

    // If the payment failed, display an error and rebuild the form.
    if (substr($auth_result, 0, 2) != IATS_TXN_OK) {
      drupal_set_message(t('We received the following error while attempting to process your card: @reason', array('@reason' => commerce_iats_reason_message($auth_result))), 'error');

      if ($auth_result == 'REJECT: 5') {
        drupal_set_message('You may have interrupted an authorization in progress - please contact us to process/complete your order.', 'error');
      }
      else {
        drupal_set_message('Please enter your information again or try a different card.', 'error');
      }
      return FALSE;
    }
  }

  $form_state['redirect'] = 'admin/commerce/orders/' . $order->order_id . '/payment';
}
