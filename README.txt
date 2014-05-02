# Commerce iATS Module for Drupal Commerce

A module to implement the iATS Payments payment processing services
in Drupal Commerce.

http://iatspayments.com/

## Features

* Provides payment methods that utilize the iATS PHP API Wrapper
* Processes credit card, ACH/EFT and Direct Debit payments
* Sub-module Commerce iATS Card on File enables the use of iATS Customer Codes
  with the Commerce Card on File module

## Requirements

* iATS PHP API Wrapper - https://github.com/iATSPayments/PHP/
* SOAP enabled in your PHP installation

### Optional

* Commerce Card on File - https://drupal.org/project/commerce_cardonfile
* Commerce Checkout Pages - https://drupal.org/project/dc_co_pages
    Used to customize the checkout process

## Upgrading from Commerce iATS 1.x

There is no upgrade path from the 1.x release of iATS Commerce.

## Payment Methods

### iATS Webservice: Credit card payment
* Processes a credit card transaction
* May optionally be used with [Commerce Card on File](https://drupal.org/project/commerce_cardonfile) to allow reuse of the card

### iATS Webservice: Credit card customer code creation
*
* May optionally be used with [Commerce Card on File](https://drupal.org/project/commerce_cardonfile) to allow reuse of the card

### iATS Webservice: ACH/EFT payment
*
* May optionally be used with [Commerce Card on File](https://drupal.org/project/commerce_cardonfile) to allow reuse of the card

### iATS Webservice: ACH/EFT customer code creation
*
* May optionally be used with [Commerce Card on File](https://drupal.org/project/commerce_cardonfile) to allow reuse of the card

### iATS Webservice: Direct debit payment validation

### iATS Webservice: Direct debit customer code creation

## Recurring Payment Checkout

Commerce iATS provides a custom checkout pane for recurring payments. To
implement this, you will need to install the Commerce Checkout Pages module:
https://drupal.org/project/dc_co_pages

iATS Payments can provide recurring payments. To leverage this on your website:
1) Enable the Commerce iATS Card on File module.
2) Enable any of the compatible payment methods:
    * iATS Webservice: Credit card customer code creation
    * iATS Webservice: ACH/EFT customer code creation
3) In the payment method settings, check the box labeled:
   "Allow recurring payments through iATS Payments"
4) Under "Administration / Store / Configuration / Checkout settings /
   Checkout pages", add a new page named "Recurring."
   Move the page to the top of the pages list.
5) Under "Administration / Store / Configuration / Checkout settings /
   Checkout form," move the "Recurring Payment" pane into the
   "Recurring" page. This will make it the first page the user sees at checkout.

A cron task will update Drupal with recurring transactions after they are
processed by iATS Payments.

If you wish to implement user or admin notifications of successful or failed
transactions, you will need to implement the Commerce Payment API hook:
"hook_commerce_payment_transaction_presave"

Documentation on the Commerce Payment API can be found here:
http://api.drupalcommerce.org/api/Drupal%20Commerce/sites!all!modules!commerce!modules!payment!commerce_payment.api.php/DC

## Direct Debit Checkout

Both the iATS Webservice: Direct debit payment validation and
iATS Webservice: Direct debit customer code creation payment methods must
be enabled to use Direct Debit.

TODO: Document Direct Debit checkout process.

## Testing

The following credentials may be used in the payment method configuration to
perform test transactions using the credit card and ACH/EFT payment methods:

  * Agent Code: TEST88
  * Password: TEST88

To test Direct Debit transactions, use the following credentials in the
payment method configuration:

  * Agent Code: UDDD88
  * Password: UDDD88
