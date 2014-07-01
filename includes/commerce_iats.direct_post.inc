<?php
/**
 * @file
 * Methods used in processing Direct Post Method transactions.
 */

function commerce_iats_direct_post_relay() {
  global $base_url;

  $order_id = $_POST['IATS_Comment'];

  $success = commerce_iats_direct_post_process_response($_POST);

  $target_url = $base_url . '/checkout/' . $order_id;
  $target_url .= ($success) ? '/complete' : '/review';

  $markup = "<html>\n"
    . "<head>\n"
    . "<script type='text/javascript' charset='utf-8'>\n"
    . "window.location = '{$target_url}';\n"
    . "</script>\n"
    . "</head>\n"
    . "<body></body>\n"
    . "</html>\n";

  echo $markup;
}

/**
 * Handles Direct Post transaction response.
 *
 * @param array $response
 *
 * $response['IATS_AccountNumber'] = '2220';
 * $response['IATS_MOP'] = 'VISA';
 * $response['IATS_PaymentType'] = 'CreditCard';
 * $response['IATS_Result'] = 'OK: 678594:';
 * $response['IATS_TransID'] = 'A70FC33C';
 * $response['IATS_Address'] = '433 North West Fourth Ave';
 * $response['IATS_City'] = 'Portland';
 * $response['IATS_Comment'] = '';
 * $response['IATS_Country'] = 'US';
 * $response['IATS_EMail'] = '';
 * $response['IATS_FirstName'] = 'Test';
 * $response['IATS_LastName'] = 'User';
 * $response['IATS_Phone'] = '';
 * $response['IATS_Fax'] = '';
 * $response['IATS_Phone2'] = '';
 * $response['IATS_State'] = 'OR';
 * $response['IATS_ZipCode'] = '97209';
 * $response['IATS_Amount'] = '10.00';
 * $response['IATS_Payment_AgencyCodeHash'] = '984816FD329622876E14907634264E6F332E9FB3';
 * $response['IATS_ExpiryMonth'] = '04';
 * $response['IATS_ExpiryYear'] = '2016';
 * $response['IATS_Payment_SubCode'] = '88';
 *
 * @return boolean
 *   True if the transaction was successful.
 */
function commerce_iats_direct_post_process_response($response) {
  if (!empty($response['IATS_Result'])) {
    // Get the order ID from the comment.
    $order_id = $response['IATS_Comment'];

    $order = commerce_order_load($order_id);

    $payment_method_instance_id = $order->data['payment_method'];
    $currency_code = $order->commerce_order_total['und'][0]['currency_code'];

    $payment_method = commerce_payment_method_instance_load($payment_method_instance_id);

    if ($payment_method['settings']['log']['response'] === 'response') {
      commerce_iats_log_response($payment_method, $response);
    }

    $result = $response['IATS_Result'];
    $transaction_id = $response['IATS_TransID'];
    // TODO: Get customer code when available.
    $customer_code = NULL;

    // Build the remote ID from the transaction ID and customer code.
    $remote_id = $transaction_id;
    if ($customer_code != NULL) {
      $remote_id .= '|' . $customer_code;
    }

    $amount = commerce_currency_decimal_to_amount($response['IATS_Amount'], $currency_code);

    // Prepare a transaction object to log the API response.
    $transaction = commerce_payment_transaction_new($payment_method['method_id'], $order_id);
    $transaction->uid = $order->uid;
    $transaction->instance_id = $payment_method['instance_id'];
    $transaction->remote_id = $remote_id;
    $transaction->amount = $amount;
    $transaction->currency_code = $currency_code;

    if (substr($result, 0, 2) != COMMERCE_IATS_TXN_OK) {
      // Create a failed transaction with the error message.
      $transaction->status = COMMERCE_PAYMENT_STATUS_FAILURE;
      $message = array('Transaction failed.');
    }
    else {
      $transaction->status = COMMERCE_PAYMENT_STATUS_SUCCESS;
      $message = array('Transaction successful.');
      if (!is_null($customer_code)) {
        $message[] = 'Customer code created: ' . $customer_code;
      }
      if ($payment_method['method_id'] == 'iats_ach_eft') {
        $message[] = 'ACH/EFT payment pending.';
      }
    }

    $transaction->message = implode('<br />', $message);

    // Save the transaction information.
    commerce_payment_transaction_save($transaction);

    // If the payment failed, display an error and rebuild the form.
    if (substr($result, 0, 2) != COMMERCE_IATS_TXN_OK) {
      drupal_set_message(
        t('We received the following error while attempting to process your payment: @reason',
          array('@reason' => $result)),
        'error'
      );
      return FALSE;
    }

    return TRUE;
  }
  else {
    drupal_set_message(
      t('We encountered an error contacting our payment processor. Please try submitting your payment again.'),
      'error'
    );
    return FALSE;
  }
}
