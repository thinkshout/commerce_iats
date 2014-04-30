<?php
/**
 * @file
 * iATS API ReportLink service used for testing.
 */

namespace iATS;

/**
 * @file
 * Test version of the iATS Payments ReportLink API.
 * Provides report data for test cases.
 */

class ReportLinkTest {

  /**
   * Gets credit card transaction data.
   *
   * @see iATS\ReportLink::getCreditCardJournal()
   */
  public function getCreditCardJournal() {
    return $this->getTestCreditCardTransactionData();
  }

  /**
   * Gets ACH/EFT transaction data.
   *
   * @see iATS\ReportLink::getACHEFTJournal()
   */
  public function getACHEFTJournal() {
    return $this->getTestACHEFTTransactionData();
  }

  /**
   * Gets an array of credit card transaction data for use in tests.
   *
   * @return array
   *   Test transaction data.
   *   array(
   *     $order_id => array(
   *       $customer_code_creation_remote_id => array(
   *         'success' => array(),
   *         'failure' => array(),
   *       ),
   *     )
   *   )
   */
  public function getTestCreditCardTransactionData() {
    $data = array(
      1 => array(
        COMMERCE_IATS_RECURRING_PAYMENT_TRANSACTION_ID . '|CCC000001' => array(
          'success' => array(
            array(
              'TNTYP' => 'CreditCard',
              'TNID' => 'CCT000001',
              'AGT' => 'TEST88',
              'CST' => array(
                'CSTC' => 'CCC000001',
                'FN' => 'Test',
                'LN' => 'User',
                'ADD' => '123 Any St.',
                'CTY' => 'Anytown',
                'ST' => 'AK',
                'CNT' => 'US',
                'ZC' => '11111',
              ),
              'CC' => array(
                'CCN' => '************2220',
                'MP' => 'VISA',
                'EXP' => '12/15',
              ),
              'AMT' => '10.00',
              'RST' => 'OK:678594:',
            ),
            array(
              'TNTYP' => 'CreditCard',
              'TNID' => 'CCT000002',
              'AGT' => 'TEST88',
              'CST' => array(
                'CSTC' => 'CCC000001',
                'FN' => 'Test',
                'LN' => 'User',
                'ADD' => '123 Any St.',
                'CTY' => 'Anytown',
                'ST' => 'AK',
                'CNT' => 'US',
                'ZC' => '11111',
              ),
              'CC' => array(
                'CCN' => '************2220',
                'MP' => 'VISA',
                'EXP' => '12/15',
              ),
              'AMT' => '10.00',
              'RST' => 'OK:678594:',
            ),
          ),
          'failure' => array(
            array(
              'TNTYP' => 'CreditCard',
              'TNID' => 'CCT000003',
              'AGT' => 'TEST88',
              'CST' => array(
                'CSTC' => 'CCC000001',
                'FN' => 'Test',
                'LN' => 'User',
                'ADD' => '123 Any St.',
                'CTY' => 'Anytown',
                'ST' => 'AK',
                'CNT' => 'US',
                'ZC' => '11111',
              ),
              'CC' => array(
                'CCN' => '************2220',
                'MP' => 'VISA',
                'EXP' => '12/15',
              ),
              'AMT' => '10.00',
              'RST' => 'TIMEOUT:Timeout',
            ),
          ),
        ),
      ),
    );

    return $this->convertTestTransactionDataToReportFormat($data);
  }

  /**
   * Gets an array of ACH/EFT transaction data for use in tests.
   *
   * @return array
   *   Test transaction data.
   *   array(
   *     $order_id => array(
   *       $customer_code_creation_remote_id => array(
   *         'success' => array(),
   *         'failure' => array(),
   *       ),
   *     )
   *   )
   */
  public function getTestACHEFTTransactionData() {
    $data = array(
      2 => array(
        COMMERCE_IATS_RECURRING_PAYMENT_TRANSACTION_ID . '|ACHC00001' => array(
          'success' => array(
            array(
              'TNTYP' => 'ACHEFT',
              'TNID' => 'ACHT00001',
              'AGT' => 'TEST88',
              'CST' => array(
                'CSTC' => 'ACHC00001',
                'FN' => 'Test',
                'LN' => 'User',
                'ADD' => '123 Any St.',
                'CTY' => 'Anytown',
                'ST' => 'AK',
                'CNT' => 'US',
                'ZC' => '11111',
              ),
              'ACH' => array(
                'ACN' => '************2220',
                'ACTYP' => 'Checking',
              ),
              'AMT' => '10.00',
              'RST' => 'OK:678594:',
            ),
            array(
              'TNTYP' => 'ACHEFT',
              'TNID' => 'ACHT00002',
              'AGT' => 'TEST88',
              'CST' => array(
                'CSTC' => 'ACHC00001',
                'FN' => 'Test',
                'LN' => 'User',
                'ADD' => '123 Any St.',
                'CTY' => 'Anytown',
                'ST' => 'AK',
                'CNT' => 'US',
                'ZC' => '11111',
              ),
              'ACH' => array(
                'ACN' => '************2220',
                'ACTYP' => 'Checking',
              ),
              'AMT' => '10.00',
              'RST' => 'OK:678594:',
            ),
          ),
          'failure' => array(
            array(
              'TNTYP' => 'ACHEFT',
              'TNID' => 'ACHT00003',
              'AGT' => 'TEST88',
              'CST' => array(
                'CSTC' => 'ACHC00001',
                'FN' => 'Test',
                'LN' => 'User',
                'ADD' => '123 Any St.',
                'CTY' => 'Anytown',
                'ST' => 'AK',
                'CNT' => 'US',
                'ZC' => '11111',
              ),
              'ACH' => array(
                'ACN' => '************2220',
                'ACTYP' => 'Checking',
              ),
              'AMT' => '10.00',
              'RST' => 'Error',
            ),
          ),
        ),
      ),
    );

    return $this->convertTestTransactionDataToReportFormat($data);
  }

  /**
   * Converts transaction test data to match the format returned by iATS API.
   *
   * @param array $data
   *   Transaction test data to convert.
   *
   * @see getTestCreditCardSuccessTransactionData()
   * @see getTestCreditCardRejectTransactionData()
   *
   * @return array
   *   Array of transactions.
   */
  protected function convertTestTransactionDataToReportFormat($data) {
    $report_data = array();

    foreach ($data as $order_data) {
      foreach ($order_data as $transactions_data) {
        foreach ($transactions_data['success'] as $success_transaction) {
          $report_data[] = $success_transaction;
        }
        foreach ($transactions_data['failure'] as $failure_transaction) {
          $report_data[] = $failure_transaction;
        }
      }
    }

    return $report_data;
  }
}
