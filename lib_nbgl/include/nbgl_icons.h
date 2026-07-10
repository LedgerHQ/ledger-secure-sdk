/**
 * @file nbgl_icons.h
 * @brief Semantic icon defines for NBGL
 *
 * This file maps device-size-independent names to the concrete glyph symbols
 * generated from the per-size PNG/BMP source files.  Include it directly when
 * you need icon names outside of nbgl_obj.h.
 */

#ifndef NBGL_ICONS_H
#define NBGL_ICONS_H

#ifdef SCREEN_SIZE_WALLET

// -------------------------------------------------------------------------
// Small icons  (SMALL_ICON_SIZE selects the actual glyph symbol)
// -------------------------------------------------------------------------
#if SMALL_ICON_SIZE == 32
#define SPACE_ICON               C_Space_32px
#define BACKSPACE_ICON           C_Erase_32px
#define SHIFT_ICON               C_Maj_32px
#define SHIFT_LOCKED_ICON        C_Maj_Lock_32px
#define VALIDATE_ICON            C_Check_32px
#define RADIO_OFF_ICON           C_radio_inactive_32px
#define RADIO_ON_ICON            C_radio_active_32px
#define PUSH_ICON                C_Chevron_32px
#define LEFT_ARROW_ICON          C_Back_32px
#define RIGHT_ARROW_ICON         C_Next_32px
#define CHEVRON_BACK_ICON        C_Chevron_Back_32px
#define CHEVRON_NEXT_ICON        C_Chevron_Next_32px
#define CLOSE_ICON               C_Close_32px
#define WHEEL_ICON               C_Settings_32px
#define INFO_I_ICON              C_Info_32px
#define QRCODE_ICON              C_QRCode_32px
#define MINI_PUSH_ICON           C_Mini_Push_32px
#define WARNING_ICON             C_Warning_32px
#define ROUND_WARN_ICON          C_Important_Circle_32px
#define PRIVACY_ICON             C_Privacy_32px
#define EXCLAMATION_ICON         C_Exclamation_32px
#define QUESTION_ICON            C_Question_32px
#define DIGIT_ICON               C_round_24px
#define QUESTION_CIRCLE_ICON     C_Question_Mark_Circle_32px
#define SEARCH_ICON              C_Search_32px
#define SWITCH_ICON              C_switch_60_40
#define ADDRESS_BOOK_ICON        C_Address_Book_32px
#define LEDGER_SERVICES_ICON     C_Ledger_Services_32px
#define GRID_ICON                C_Grid_32px
#define LEDGER_ICON              C_Ledger_32px
#define LIST_ICON                C_List_32px
#define PLUS_ICON                C_Plus_32px
#define SAFE_ICON                C_Safe_32px
#define SECURITY_SHIELD_ICON     C_SecurityShield_32px
#define SHIELD_BACKUP_ICON       C_Shield_Backup_32px
#define SHIELD_CHECK_ICON        C_Shield_Check_32px
#define SMALL_CHECK_CIRCLE_ICON  C_Check_Circle_32px
#define SMALL_DENIED_CIRCLE_ICON C_Denied_Circle_32px
#define SMALL_INFO_CIRCLE_ICON   C_Info_Circle_32px
#define SMALL_REVIEW_ICON        C_Review_32px
#define SMALL_TRASH_ICON         C_Trash_32px
#elif SMALL_ICON_SIZE == 40
#define SPACE_ICON               C_Space_40px
#define BACKSPACE_ICON           C_Erase_40px
#define SHIFT_ICON               C_Maj_40px
#define SHIFT_LOCKED_ICON        C_Maj_Lock_40px
#define VALIDATE_ICON            C_Check_40px
#define RADIO_OFF_ICON           C_radio_inactive_40px
#define RADIO_ON_ICON            C_radio_active_40px
#define PUSH_ICON                C_Chevron_40px
#define LEFT_ARROW_ICON          C_Back_40px
#define RIGHT_ARROW_ICON         C_Next_40px
#define CHEVRON_BACK_ICON        C_Chevron_Back_40px
#define CHEVRON_NEXT_ICON        C_Chevron_Next_40px
#define CLOSE_ICON               C_Close_40px
#define WHEEL_ICON               C_Settings_40px
#define INFO_I_ICON              C_Info_40px
#define QRCODE_ICON              C_QRCode_40px
#define MINI_PUSH_ICON           C_Mini_Push_40px
#define WARNING_ICON             C_Warning_40px
#define ROUND_WARN_ICON          C_Important_Circle_40px
#define PRIVACY_ICON             C_Privacy_40px
#define EXCLAMATION_ICON         C_Exclamation_40px
#define QUESTION_ICON            C_Question_40px
#define DIGIT_ICON               C_pin_24
#define QUESTION_CIRCLE_ICON     C_Question_Mark_Circle_40px
#define SEARCH_ICON              C_Search_40px
#define SWITCH_ICON              C_switch_60_40
#define ADDRESS_BOOK_ICON        C_Address_Book_40px
#define LEDGER_SERVICES_ICON     C_Ledger_Services_40px
#define GRID_ICON                C_Grid_40px
#define LEDGER_ICON              C_Ledger_40px
#define LIST_ICON                C_List_40px
#define PLUS_ICON                C_Plus_40px
#define SAFE_ICON                C_Safe_40px
#define SECURITY_SHIELD_ICON     C_SecurityShield_40px
#define SHIELD_BACKUP_ICON       C_Shield_Backup_40px
#define SHIELD_CHECK_ICON        C_Shield_Check_40px
#define SMALL_CHECK_CIRCLE_ICON  C_Check_Circle_40px
#define SMALL_DENIED_CIRCLE_ICON C_Denied_Circle_40px
#define SMALL_INFO_CIRCLE_ICON   C_Info_Circle_40px
#define SMALL_REVIEW_ICON        C_Review_40px
#define SMALL_TRASH_ICON         C_Trash_40px
#elif SMALL_ICON_SIZE == 24
#define SPACE_ICON               C_Space_24px
#define BACKSPACE_ICON           C_Erase_24px
#define SHIFT_ICON               C_Maj_24px
#define SHIFT_LOCKED_ICON        C_Maj_Lock_24px
#define VALIDATE_ICON            C_Check_24px
#define RADIO_OFF_ICON           C_radio_inactive_24px
#define RADIO_ON_ICON            C_radio_active_24px
#define PUSH_ICON                C_Chevron_24px
#define LEFT_ARROW_ICON          C_Back_24px
#define RIGHT_ARROW_ICON         C_Next_24px
#define CHEVRON_BACK_ICON        C_Chevron_Back_24px
#define CHEVRON_NEXT_ICON        C_Chevron_Next_24px
#define CLOSE_ICON               C_Close_Tiny_24px
#define WHEEL_ICON               C_Settings_24px
#define INFO_I_ICON              C_Info_24px
#define QRCODE_ICON              C_QRCode_24px
#define MINI_PUSH_ICON           C_Mini_Push_24px
#define WARNING_ICON             C_Warning_24px
#define ROUND_WARN_ICON          C_Important_Circle_24px
#define PRIVACY_ICON             C_Privacy_24px
#define EXCLAMATION_ICON         C_Exclamation_24px
#define QUESTION_ICON            C_Question_24px
#define DIGIT_ICON               C_Dot_16px
#define QUESTION_CIRCLE_ICON     C_Question_Mark_Circle_24px
#define SEARCH_ICON              C_Search_24px
#define SWITCH_ICON              C_switch_on_24px
#define ADDRESS_BOOK_ICON        C_Address_Book_24px
#define LEDGER_SERVICES_ICON     C_Ledger_Services_24px
#define GRID_ICON                C_Grid_24px
#define LEDGER_ICON              C_Ledger_24px
#define LIST_ICON                C_List_24px
#define PLUS_ICON                C_Plus_24px
#define SAFE_ICON                C_Safe_24px
#define SECURITY_SHIELD_ICON     C_SecurityShield_24px
#define SHIELD_BACKUP_ICON       C_Shield_Backup_24px
#define SHIELD_CHECK_ICON        C_Shield_Check_24px
#define SMALL_CHECK_CIRCLE_ICON  C_Check_Circle_24px
#define SMALL_DENIED_CIRCLE_ICON C_Denied_Circle_24px
#define SMALL_INFO_CIRCLE_ICON   C_Info_Circle_24px
#define SMALL_REVIEW_ICON        C_Review_24px
#define SMALL_TRASH_ICON         C_Trash_24px
#else  // SMALL_ICON_SIZE
#error Undefined SMALL_ICON_SIZE
#endif  // SMALL_ICON_SIZE

// -------------------------------------------------------------------------
// Large icons  (LARGE_ICON_SIZE selects the actual glyph symbol)
// -------------------------------------------------------------------------
#if LARGE_ICON_SIZE == 64
#define CHECK_CIRCLE_ICON          C_Check_Circle_64px
#define DENIED_CIRCLE_ICON         C_Denied_Circle_64px
#define IMPORTANT_CIRCLE_ICON      C_Important_Circle_64px
#define LARGE_WARNING_ICON         C_Warning_64px
#define INFO_CIRCLE_ICON           C_Info_Circle_64px
#define LARGE_REVIEW_ICON          C_Review_64px
#define LARGE_LOGIN_ICON           C_Login_64px
#define LARGE_TRASH_ICON           C_Trash_64px
#define LARGE_ADDRESS_BOOK_ICON    C_Address_Book_64px
#define LARGE_LEDGER_SERVICES_ICON C_Ledger_Services_64px
#define LARGE_CODE_ICON            C_Code_64px
#define LARGE_LEDGER_ICON          C_Ledger_64px
#define LARGE_LIST_ICON            C_List_64px
#define LARGE_MULTISIG_ICON        C_Multisig_64px
#define LARGE_REFRESH_ICON         C_Refresh_64px
#define LARGE_REFRESH_SLASH_ICON   C_RefreshSlash_64px
#define LARGE_SAFE_ICON            C_Safe_64px
#define LARGE_SECURITY_SHIELD_ICON C_SecurityShield_64px
#define LARGE_SHIELD_BACKUP_ICON   C_Shield_Backup_64px
#elif LARGE_ICON_SIZE == 48
#define CHECK_CIRCLE_ICON          C_Check_Circle_48px
#define DENIED_CIRCLE_ICON         C_Denied_Circle_48px
#define IMPORTANT_CIRCLE_ICON      C_Important_Circle_48px
#define LARGE_WARNING_ICON         C_Warning_48px
#define INFO_CIRCLE_ICON           C_Info_Circle_48px
#define LARGE_REVIEW_ICON          C_Review_48px
#define LARGE_LOGIN_ICON           C_Login_48px
#define LARGE_TRASH_ICON           C_Trash_48px
#define LARGE_ADDRESS_BOOK_ICON    C_Address_Book_48px
#define LARGE_LEDGER_SERVICES_ICON C_Ledger_Services_48px
#define LARGE_CODE_ICON            C_Code_48px
#define LARGE_LEDGER_ICON          C_Ledger_48px
#define LARGE_LIST_ICON            C_List_48px
#define LARGE_MULTISIG_ICON        C_Multisig_48px
#define LARGE_REFRESH_ICON         C_Refresh_48px
#define LARGE_REFRESH_SLASH_ICON   C_RefreshSlash_48px
#define LARGE_SAFE_ICON            C_Safe_48px
#define LARGE_SECURITY_SHIELD_ICON C_SecurityShield_48px
#define LARGE_SHIELD_BACKUP_ICON   C_Shield_Backup_48px
#else  // LARGE_ICON_SIZE
#error Undefined LARGE_ICON_SIZE
#endif  // LARGE_ICON_SIZE

// -------------------------------------------------------------------------
// Backward-compatibility aliases (deprecated — will be removed)
// -------------------------------------------------------------------------
#define C_warning64px        _Pragma("GCC warning \"Deprecated constant!\"") C_Warning_64px
#define C_round_warning_64px _Pragma("GCC warning \"Deprecated constant!\"") C_Important_Circle_64px

#else  // SCREEN_SIZE_WALLET

// -------------------------------------------------------------------------
// Nano / non-wallet icons
// -------------------------------------------------------------------------
#define WARNING_ICON               C_icon_warning
#define REVIEW_ICON                C_icon_certificate
#define LOGIN_ICON                 C_Login_14px
#define TRASH_ICON                 C_Trash_14px
#define ADDRESS_BOOK_ICON          C_Address_Book_14px
#define LEDGER_ICON                C_Ledger_14px
#define INFO_CIRCLE_ICON           C_Information_circle_14px
#define SECURITY_SHIELD_ICON       C_SecurityShield_14px
#define VALIDATE_ICON              C_icon_validate_14px
#define ROUND_WARN_ICON            C_Alert_circle_14px
#define PLUS_ICON                  C_Plus_14px
#define EYE_ICON                   C_icon_eye
#define LARGE_MULTISIG_ICON        C_Multisig_14px
#define LARGE_CODE_ICON            C_Code_14px
// Aliases: let code using wallet icon names compile on Nano
#define LARGE_WARNING_ICON         WARNING_ICON
#define LARGE_REVIEW_ICON          REVIEW_ICON
#define LARGE_LOGIN_ICON           LOGIN_ICON
#define LARGE_TRASH_ICON           TRASH_ICON
#define LARGE_ADDRESS_BOOK_ICON    ADDRESS_BOOK_ICON
#define LARGE_LEDGER_ICON          LEDGER_ICON
#define SMALL_INFO_CIRCLE_ICON     INFO_CIRCLE_ICON
#define LARGE_SECURITY_SHIELD_ICON SECURITY_SHIELD_ICON
#define SMALL_CHECK_CIRCLE_ICON    VALIDATE_ICON
#define CHECK_CIRCLE_ICON          VALIDATE_ICON
#define IMPORTANT_CIRCLE_ICON      ROUND_WARN_ICON

#endif  // SCREEN_SIZE_WALLET

#endif  // NBGL_ICONS_H
