/**
 * @file
 * Commerce iATS USB card reader JavaScript functionality.
 */

(function ($) {

  Drupal.behaviors.commerce_iats_usb_reader_credit_card = {
    attach: function(context, settings) {
      var encrypted_element = $("textarea#edit-payment-details-encrypted-credit-card-encrypted-number");

      encrypted_element.focus();
    }
  }

})(jQuery);
