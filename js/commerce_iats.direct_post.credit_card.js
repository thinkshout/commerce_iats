/**
 * @file
 * Commerce iATS Direct Post credit card JavaScript functionality.
 */

(function ($) {

  Drupal.behaviors.commerce_iats_direct_post_credit_card = {
    attach: function(context, settings) {
      $('input#edit-continue', context).unbind('click').bind('click', function() {
        var expiry_month = $("#edit-commerce-payment-payment-details-iats-dpm-exp-month").val();
        var expiry_year = $("#edit-commerce-payment-payment-details-iats-dpm-exp-year").val();

        var expiry_string = expiry_month + '/' + expiry_year[2] + expiry_year[3];

        $("input[name=IATS_DPM_ExpiryDate]").val(expiry_string);

        return true;
      });
    }
  }

})(jQuery);
