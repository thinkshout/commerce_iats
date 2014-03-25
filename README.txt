# Commerce iATS Module for Drupal Commerce

A module to implement the [iATS Payments](http://iatspayments.com/) payment processing services in Drupal Commerce.

## Features
* Provides payment methods that utilize the iATS PHP API Wrapper
* Sub-module Commerce iATS Card on File enables the use of iATS Customer Codes with the [Commerce Card on File](https://drupal.org/project/commerce_cardonfile) module

## Requirements

* [iATS PHP API Wrapper](https://github.com/iATSPayments/PHP/)
* SOAP enabled in your PHP installation

## Payment Methods

### Commerce iATS

#### iATS Webservice: Credit card payment
* Processes a credit card transaction
* May optionally be used with [Commerce Card on File](https://drupal.org/project/commerce_cardonfile) to store a reference to credit card details

### Commerce iATS Card on File

#### iATS Webservice: Credit card customer code creation
* Creates a Customer Code for a user, storing their credit card details through iATS
* Creates a [Commerce Card on File](https://drupal.org/project/commerce_cardonfile) using the iATS Customer Code

## Testing

The following credentials may be used in the payment method configuration to perform test transactions:
  * Agent Code: TEST88
  * Password: TEST88