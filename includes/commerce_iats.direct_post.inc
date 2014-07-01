<?php
/**
 * @file
 * Methods used in processing Direct Post Method transactions.
 */

function commerce_iats_direct_post_relay() {
  global $base_url;

  $markup = "<html>\n"
    . "<head>\n"
    . "<script type='text/javascript' charset='utf-8'>\n"
    . "window.location = '{$base_url}';\n"
    . "</script>\n"
    . "</head>\n"
    . "<body></body>\n"
    . "</html>\n";

  echo $markup;
}
