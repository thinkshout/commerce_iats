<?php
/**
 * @file
 * User page callbacks and forms for payment processing with Card on File.
 * Callback methods wrap payment method-specific callback methods to provide
 * the correct logic for the current payment method.
 */

/**
 * @see commerce_cardonfile_card_title()
 */
function commerce_iats_cardonfile_card_title($card_data) {
  if ($card_data->payment_method == 'iats_ach_eft') {
    module_load_include('inc', 'commerce_iats_cardonfile', 'includes/commerce_iats_cardonfile.ach_eft.pages');
    return commerce_iats_cardonfile_ach_eft_card_title($card_data);
  }
  else {
    module_load_include('inc', 'commerce_cardonfile', 'includes/commerce_cardonfile.pages');
    return commerce_cardonfile_card_title($card_data);
  }
}

/**
 * @see commerce_cardonfile_card_view()
 */
function commerce_iats_cardonfile_card_view($card_data, $view_mode) {
  if ($card_data->payment_method == 'iats_ach_eft') {
    module_load_include('inc', 'commerce_iats_cardonfile', 'includes/commerce_iats_cardonfile.ach_eft.pages');
    return commerce_iats_cardonfile_ach_eft_card_view($card_data, $view_mode);
  }
  else {
    module_load_include('inc', 'commerce_cardonfile', 'includes/commerce_cardonfile.pages');
    return commerce_cardonfile_card_view($card_data, $view_mode);
  }
}

/**
 * @see commerce_cardonfile_card_form_page()
 */
function commerce_iats_cardonfile_card_form_page($op, $card_data, $account) {
  if ($card_data->payment_method == 'iats_ach_eft') {
    module_load_include('inc', 'commerce_iats_cardonfile', 'includes/commerce_iats_cardonfile.ach_eft.pages');
    return commerce_iats_cardonfile_ach_eft_card_form_page($op, $card_data, $account);
  }
  else {
    module_load_include('inc', 'commerce_cardonfile', 'includes/commerce_cardonfile.pages');
    return commerce_cardonfile_card_form_page($op, $card_data, $account);
  }
}

/**
 * @see commerce_cardonfile_card_form()
 */
function commerce_iats_cardonfile_card_form($form, &$form_state, $op, $card_data) {
  if ($card_data->payment_method == 'iats_ach_eft') {
    module_load_include('inc', 'commerce_iats_cardonfile', 'includes/commerce_iats_cardonfile.ach_eft.pages');
    return commerce_iats_cardonfile_ach_eft_card_form($form, $form_state, $op, $card_data);
  }
  else {
    module_load_include('inc', 'commerce_cardonfile', 'includes/commerce_cardonfile.pages');
    return commerce_cardonfile_card_form($form, $form_state, $op, $card_data);
  }
}

/**
 * @see commerce_cardonfile_card_form_validate()
 */
function commerce_iats_cardonfile_card_form_validate($form, &$form_state) {
  if ($form_state['card_data']->payment_method == 'iats_ach_eft') {
    module_load_include('inc', 'commerce_iats_cardonfile', 'includes/commerce_iats_cardonfile.ach_eft.pages');
    return commerce_iats_cardonfile_ach_eft_card_form_validate($form, $form_state);
  }
  else {
    module_load_include('inc', 'commerce_cardonfile', 'includes/commerce_cardonfile.pages');
    return commerce_cardonfile_card_form_validate($form, $form_state);
  }
}

/**
 * @see commerce_cardonfile_card_form_submit()
 */
function commerce_iats_cardonfile_card_form_submit($form, &$form_state) {
  if ($form_state['card_data']->payment_method == 'iats_ach_eft') {
    module_load_include('inc', 'commerce_iats_cardonfile', 'includes/commerce_iats_cardonfile.ach_eft.pages');
    return commerce_iats_cardonfile_ach_eft_card_form_submit($form, $form_state);
  }
  else {
    module_load_include('inc', 'commerce_cardonfile', 'includes/commerce_cardonfile.pages');
    return commerce_cardonfile_card_form_submit($form, $form_state);
  }
}

/**
 * @see commerce_cardonfile_delete_form()
 */
function commerce_iats_cardonfile_delete_form($form, &$form_state, $card_data) {
  if ($card_data->payment_method == 'iats_ach_eft') {
    module_load_include('inc', 'commerce_iats_cardonfile', 'includes/commerce_iats_cardonfile.ach_eft.pages');
    return commerce_iats_cardonfile_ach_eft_delete_form($form, $form_state, $card_data);
  }
  else {
    module_load_include('inc', 'commerce_cardonfile', 'includes/commerce_cardonfile.pages');
    return commerce_cardonfile_delete_form($form, $form_state, $card_data);
  }
}

/**
 * @see commerce_cardonfile_delete_form_submit()
 */
function commerce_iats_cardonfile_delete_form_submit($form, &$form_state) {
  if ($form_state['card_data']->payment_method == 'iats_ach_eft') {
    module_load_include('inc', 'commerce_iats_cardonfile', 'includes/commerce_iats_cardonfile.ach_eft.pages');
    return commerce_iats_cardonfile_ach_eft_delete_form_submit($form, $form_state);
  }
  else {
    module_load_include('inc', 'commerce_cardonfile', 'includes/commerce_cardonfile.pages');
    return commerce_cardonfile_delete_form_submit($form, $form_state);
  }
}