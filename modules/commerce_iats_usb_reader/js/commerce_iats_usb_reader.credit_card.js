/**
 * @file
 * Commerce iATS USB card reader JavaScript functionality.
 */

(function ($) {

  Drupal.behaviors.commerce_iats_usb_reader_credit_card = {
    attach: function (context, settings) {
      var encrypted_element = $("textarea#edit-payment-details-encrypted-credit-card-encrypted-number", context);

      // When the card data field has focus, instuct the user to swipe the card.
      encrypted_element.focusin(function () {
        $("#edit-payment-details-encrypted-credit-card-status", context).html(Drupal.t('Swipe credit card now.'));
      });

      // When the card data field loses focus, instruct the user to regain focus.
      encrypted_element.focusout(function (e) {
        $("#edit-payment-details-encrypted-credit-card-status", context).html('<a href="#" onclick="Drupal.behaviors.commerce_iats_usb_reader_credit_card.card_focus(context); return false">Drupal.t(Click to swipe credit card)</a>');
      });

      encrypted_element.focus();
    },

    card_focus: function (context) {
      var encrypted_element = $("textarea#edit-payment-details-encrypted-credit-card-encrypted-number", context);
      encrypted_element.focus();

      // The focus event set in attach() is not caught when setting focus in JS.
      // Have to set status text manually here.
      $("#edit-payment-details-encrypted-credit-card-status", context).html(Drupal.t('Swipe credit card now.'));
    }

  }

})(jQuery);
