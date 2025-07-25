#pragma once

// the number of parameters of a syscall is stored in the syscall id
#define SYSCALL_NUMBER_OF_PARAMETERS(id) (((id) >> 24) & 0xf)

#define SYSCALL_get_api_level_ID                 0x00000001
#define SYSCALL_halt_ID                          0x00000002
#define SYSCALL_nvm_write_ID                     0x03000003
#define SYSCALL_nvm_erase_ID                     0x02000121
#define SYSCALL_cx_aes_set_key_hw_ID             0x020000b2
#define SYSCALL_cx_aes_reset_hw_ID               0x000000b3
#define SYSCALL_cx_aes_block_hw_ID               0x020000b4
#define SYSCALL_cx_bn_lock_ID                    0x02000112
#define SYSCALL_cx_bn_unlock_ID                  0x000000b6
#define SYSCALL_cx_bn_is_locked_ID               0x000000b7
#define SYSCALL_cx_bn_alloc_ID                   0x02000113
#define SYSCALL_cx_bn_alloc_init_ID              0x04000114
#define SYSCALL_cx_bn_destroy_ID                 0x010000bc
#define SYSCALL_cx_bn_nbytes_ID                  0x0200010d
#define SYSCALL_cx_bn_init_ID                    0x03000115
#define SYSCALL_cx_bn_rand_ID                    0x010000ea
#define SYSCALL_cx_bn_copy_ID                    0x020000c0
#define SYSCALL_cx_bn_set_u32_ID                 0x020000c1
#define SYSCALL_cx_bn_get_u32_ID                 0x020000eb
#define SYSCALL_cx_bn_export_ID                  0x030000c3
#define SYSCALL_cx_bn_cmp_ID                     0x030000c4
#define SYSCALL_cx_bn_cmp_u32_ID                 0x030000c5
#define SYSCALL_cx_bn_is_odd_ID                  0x02000118
#define SYSCALL_cx_bn_xor_ID                     0x030000c8
#define SYSCALL_cx_bn_or_ID                      0x030000c9
#define SYSCALL_cx_bn_and_ID                     0x030000ca
#define SYSCALL_cx_bn_tst_bit_ID                 0x030000cb
#define SYSCALL_cx_bn_set_bit_ID                 0x020000cc
#define SYSCALL_cx_bn_clr_bit_ID                 0x020000cd
#define SYSCALL_cx_bn_shr_ID                     0x020000ce
#define SYSCALL_cx_bn_shl_ID                     0x0200011c
#define SYSCALL_cx_bn_cnt_bits_ID                0x020000ec
#define SYSCALL_cx_bn_add_ID                     0x03000119
#define SYSCALL_cx_bn_sub_ID                     0x0300011a
#define SYSCALL_cx_bn_mul_ID                     0x030000d2
#define SYSCALL_cx_bn_mod_add_ID                 0x040000d3
#define SYSCALL_cx_bn_mod_sub_ID                 0x040000d4
#define SYSCALL_cx_bn_mod_mul_ID                 0x040000d5
#define SYSCALL_cx_bn_reduce_ID                  0x030000d6
#define SYSCALL_cx_bn_mod_sqrt_ID                0x0400011d
#define SYSCALL_cx_bn_mod_pow_bn_ID              0x040000d7
#define SYSCALL_cx_bn_mod_pow_ID                 0x050000ed
#define SYSCALL_cx_bn_mod_pow2_ID                0x050000ee
#define SYSCALL_cx_bn_mod_invert_nprime_ID       0x030000da
#define SYSCALL_cx_bn_mod_u32_invert_ID          0x03000116
#define SYSCALL_cx_bn_is_prime_ID                0x020000ef
#define SYSCALL_cx_bn_next_prime_ID              0x010000f0
#define SYSCALL_cx_bn_rng_ID                     0x020001dd
#define SYSCALL_cx_bn_gf2_n_mul_ID               0x05000046
#define SYSCALL_cx_mont_alloc_ID                 0x020000dc
#define SYSCALL_cx_mont_init_ID                  0x020000dd
#define SYSCALL_cx_mont_init2_ID                 0x030000de
#define SYSCALL_cx_mont_to_montgomery_ID         0x030000df
#define SYSCALL_cx_mont_from_montgomery_ID       0x030000e0
#define SYSCALL_cx_mont_mul_ID                   0x040000e1
#define SYSCALL_cx_mont_pow_ID                   0x050000e2
#define SYSCALL_cx_mont_pow_bn_ID                0x040000e3
#define SYSCALL_cx_mont_invert_nprime_ID         0x030000e4
#define SYSCALL_cx_ecdomain_size_ID              0x0200012e
#define SYSCALL_cx_ecdomain_parameters_length_ID 0x0200012f
#define SYSCALL_cx_ecdomain_parameter_ID         0x04000130
#define SYSCALL_cx_ecdomain_parameter_bn_ID      0x03000131
#define SYSCALL_cx_ecdomain_generator_ID         0x04000132
#define SYSCALL_cx_ecdomain_generator_bn_ID      0x02000133
#define SYSCALL_cx_ecpoint_alloc_ID              0x020000f1
#define SYSCALL_cx_ecpoint_destroy_ID            0x010000f2
#define SYSCALL_cx_ecpoint_init_ID               0x050000f3
#define SYSCALL_cx_ecpoint_init_bn_ID            0x030000f4
#define SYSCALL_cx_ecpoint_export_ID             0x050000f5
#define SYSCALL_cx_ecpoint_export_bn_ID          0x030000f6
#define SYSCALL_cx_ecpoint_compress_ID           0x0400012c
#define SYSCALL_cx_ecpoint_decompress_ID         0x0400012d
#define SYSCALL_cx_ecpoint_add_ID                0x0300010e
#define SYSCALL_cx_ecpoint_neg_ID                0x0100010f
#define SYSCALL_cx_ecpoint_scalarmul_ID          0x03000110
#define SYSCALL_cx_ecpoint_scalarmul_bn_ID       0x02000111
#define SYSCALL_cx_ecpoint_rnd_scalarmul_ID      0x03000127
#define SYSCALL_cx_ecpoint_rnd_scalarmul_bn_ID   0x02000128

#ifdef HAVE_FIXED_SCALAR_LENGTH
#define SYSCALL_cx_ecpoint_rnd_fixed_scalarmul_ID 0x03000129
#endif  // HAVE_FIXED_SCALAR_LENGTH

#define SYSCALL_cx_ecpoint_double_scalarmul_ID    0x07000148
#define SYSCALL_cx_ecpoint_double_scalarmul_bn_ID 0x0500014a
#define SYSCALL_cx_ecpoint_cmp_ID                 0x030000fb
#define SYSCALL_cx_ecpoint_is_on_curve_ID         0x020000fc
#define SYSCALL_cx_ecpoint_is_at_infinity_ID      0x0200014b

#ifdef HAVE_X25519
#define SYSCALL_cx_ecpoint_x25519_ID 0x0300001b
#endif  // HAVE_X25519

#ifdef HAVE_X448
#define SYSCALL_cx_ecpoint_x448_ID 0x03000060
#endif  // HAVE_X448

#ifdef HAVE_VSS
#define SYSCALL_cx_vss_generate_shares_ID 0x0a000001
#define SYSCALL_cx_vss_combine_shares_ID  0x04000002
#define SYSCALL_cx_vss_verify_commits_ID  0x05000003
#endif  // HAVE_VSS

#define SYSCALL_cx_crc_hw_ID 0x04000102

#ifdef HAVE_BLS
#define SYSCALL_ox_bls12381_sign_ID      0x05000103
#define SYSCALL_cx_hash_to_field_ID      0x06000104
#define SYSCALL_cx_bls12381_aggregate_ID 0x05000105
#define SYSCALL_cx_bls12381_key_gen_ID   0x03000108
#endif  // HAVE_BLS

#define SYSCALL_cx_get_random_bytes_ID     0x02000107
#define SYSCALL_cx_trng_get_random_data_ID 0x02000106

#define SYSCALL_os_perso_erase_all_ID                 0x0000004b
#define SYSCALL_os_perso_set_seed_ID                  0x0400004e
#define SYSCALL_os_perso_derive_and_set_seed_ID       0x0700004f
#define SYSCALL_os_perso_set_words_ID                 0x02000050
#define SYSCALL_os_perso_finalize_ID                  0x01000051
#define SYSCALL_os_perso_is_pin_set_ID                0x0000009e
#define SYSCALL_os_perso_isonboarded_ID               0x0000009f
#define SYSCALL_os_perso_setonboardingstatus_ID       0x03000094
#define SYSCALL_os_perso_derive_node_bip32_ID         0x05000053
#define SYSCALL_os_perso_derive_node_with_seed_key_ID 0x080000a6
#define SYSCALL_os_perso_derive_eip2333_ID            0x040000a7

#define SYSCALL_os_perso_master_seed_ID 0x02000052
#ifdef HAVE_RECOVER
#define SYSCALL_os_perso_recover_state_ID 0x02000054
#endif  // HAVE_RECOVER

#if defined(HAVE_SEED_COOKIE)
#define SYSCALL_os_perso_seed_cookie_ID 0x010000a8
#endif  // HAVE_SEED_COOKIE

#define SYSCALL_ENDORSEMENT_get_code_hash_ID               0x01000055
#define SYSCALL_ENDORSEMENT_get_public_key_ID              0x03000056
#define SYSCALL_ENDORSEMENT_get_public_key_certificate_ID  0x03000057
#define SYSCALL_ENDORSEMENT_key1_get_app_secret_ID         0x01000058
#define SYSCALL_ENDORSEMENT_key1_sign_data_ID              0x04000059
#define SYSCALL_ENDORSEMENT_key2_derive_and_sign_data_ID   0x0400005a
#define SYSCALL_ENDORSEMENT_key1_sign_without_code_hash_ID 0x0400005b
#define SYSCALL_os_perso_set_pin_ID                        0x0400004c
#define SYSCALL_os_perso_set_current_identity_pin_ID       0x0200004d
#define SYSCALL_os_global_pin_is_validated_ID              0x000000a0
#define SYSCALL_os_global_pin_check_ID                     0x020000a1
#define SYSCALL_os_global_pin_invalidate_ID                0x0000005d
#define SYSCALL_os_global_pin_retries_ID                   0x0000005e
#define SYSCALL_os_registry_count_ID                       0x0000005f
#define SYSCALL_os_registry_get_ID                         0x02000122
#define SYSCALL_os_ux_ID                                   0x01000064
#define SYSCALL_os_lib_call_ID                             0x01000067
#define SYSCALL_os_lib_end_ID                              0x00000068
#define SYSCALL_os_flags_ID                                0x0000006a
#define SYSCALL_os_version_ID                              0x0200006b
#define SYSCALL_os_serial_ID                               0x0200006c
#define SYSCALL_os_seph_features_ID                        0x0000006e
#define SYSCALL_os_seph_version_ID                         0x0200006f
#define SYSCALL_os_bootloader_version_ID                   0x02000073
#define SYSCALL_os_factory_setting_get_ID                  0x0300014c
#define SYSCALL_os_setting_get_ID                          0x03000070
#define SYSCALL_os_setting_set_ID                          0x03000071
#define SYSCALL_os_get_memory_info_ID                      0x01000072
#define SYSCALL_os_registry_get_tag_ID                     0x06000123
#define SYSCALL_os_registry_get_current_app_tag_ID         0x03000074
#define SYSCALL_os_registry_delete_app_and_dependees_ID    0x01000124
#define SYSCALL_os_registry_delete_all_apps_ID             0x00000125
#define SYSCALL_os_sched_exec_ID                           0x01000126
#define SYSCALL_os_sched_exit_ID                           0x0100009a
#define SYSCALL_os_sched_is_running_ID                     0x0100009b
#define SYSCALL_os_sched_create_ID                         0x0700011b
#define SYSCALL_os_sched_kill_ID                           0x01000078
#define SYSCALL_os_io_seph_tx_ID                           0x03000082
#define SYSCALL_os_io_seph_se_rx_event_ID                  0x05000083
#define SYSCALL_os_io_init_ID                              0x01000084
#define SYSCALL_os_io_start_ID                             0x01000085
#define SYSCALL_os_io_stop_ID                              0x01000086
#define SYSCALL_os_io_tx_cmd_ID                            0x04000088
#define SYSCALL_os_io_rx_evt_ID                            0x03000089
#define SYSCALL_nvm_write_page_ID                          0x0100010a
#define SYSCALL_nvm_erase_page_ID                          0x01000136
#define SYSCALL_try_context_get_ID                         0x00000087
#define SYSCALL_try_context_set_ID                         0x0100010b
#define SYSCALL_os_sched_last_status_ID                    0x0100009c
#define SYSCALL_os_sched_yield_ID                          0x0100009d
#define SYSCALL_os_sched_switch_ID                         0x0200009e
#define SYSCALL_os_sched_current_task_ID                   0x0000008b
#define SYSCALL_os_allow_protected_flash_ID                0x0000008e
#define SYSCALL_os_deny_protected_flash_ID                 0x00000091
#define SYSCALL_os_allow_protected_ram_ID                  0x00000092
#define SYSCALL_os_deny_protected_ram_ID                   0x00000093
#define SYSCALL_os_set_ux_time_ms_ID                       0x010000a2
#if defined(HAVE_LEDGER_PKI)
#define SYSCALL_os_pki_load_certificate_ID 0x060000aa
#define SYSCALL_os_pki_verify_ID           0x040000ab
#define SYSCALL_os_pki_get_info_ID         0x040000ac
#endif  // HAVE_LEDGER_PKI

#define SYSCALL_os_dashboard_mbx_ID       0x02000150
#define SYSCALL_os_ux_set_global_ID       0x03000151
#define SYSCALL_os_rk_set_backup_state_ID 0x01000152
#define SYSCALL_os_rk_set_update_state_ID 0x01000155

#ifdef HAVE_CUSTOM_CA_DETAILS_IN_SETTINGS
#define SYSCALL_CERT_get_ID   0x01000CA0
#define SYSCALL_CERT_erase_ID 0x00000CA1
#endif  // HAVE_CUSTOM_CA_DETAILS_IN_SETTINGS

#define SYSCALL_ENDORSEMENT_revoke_slot_ID 0x010001ED

#define SYSCALL_ENDORSEMENT_get_metadata_ID 0x02000138

#if defined(HAVE_VAULT_RECOVERY_ALGO)
#define SYSCALL_os_perso_derive_and_prepare_seed_ID 0x02000137
#define SYSCALL_os_perso_derive_and_xor_seed_ID     0x00000149
#define SYSCALL_os_perso_get_seed_algorithm_ID      0x00000152
#endif  // HAVE_VAULT_RECOVERY_ALGO

#ifdef HAVE_SE_SCREEN
#define SYSCALL_screen_clear_ID       0x00000079
#define SYSCALL_screen_update_ID      0x0000007a
#define SYSCALL_screen_set_keepout_ID 0x0400007b
#ifdef HAVE_BRIGHTNESS_SYSCALL
#define SYSCALL_screen_set_brightness_ID 0x0100008c
#endif  // HAVE_BRIGHTNESS_SYSCALL
#define SYSCALL_bagl_hal_draw_bitmap_within_rect_ID 0x0900007c
#define SYSCALL_bagl_hal_draw_rect_ID               0x0500007d
#endif  // HAVE_SE_SCREEN

#ifdef HAVE_BLE
#define SYSCALL_os_ux_set_status_ID_IN 0x02000134
#define SYSCALL_os_ux_get_status_ID_IN 0x01000135
#endif

#ifdef HAVE_SE_BUTTON
#define SYSCALL_io_button_read_ID 0x0000008f
#endif  // HAVE_SE_BUTTON

#define SYSCALL_os_standby_ID 0x0000d0d0

#define SYSCALL_os_seph_serial_ID 0x0200006d

#if defined(HAVE_LANGUAGE_PACK)
#define SYSCALL_list_language_packs_ID 0x01000153
#define SYSCALL_get_language_pack_ID   0x01000154
#endif  // defined(HAVE_LANGUAGE_PACK)

#ifdef DEBUG_OS_STACK_CONSUMPTION
#define SYSCALL_os_stack_operations_ID 0x01000199
#endif  // DEBUG_OS_STACK_CONSUMPTION

#ifdef BOLOS_DEBUG
#define SYSCALL_log_debug_ID        0x0100011e
#define SYSCALL_log_debug_nw_ID     0x0100011f
#define SYSCALL_log_debug_int_ID    0x02000080
#define SYSCALL_log_debug_int_nw_ID 0x02000081
#define SYSCALL_log_mem_ID          0x02000120
#endif  // BOLOS_DEBUG

// Stax syscalls
#ifdef HAVE_NBGL
#define SYSCALL_nbgl_front_draw_rect_ID            0x01fa0000
#define SYSCALL_nbgl_front_draw_line_ID            0x03fa0001
#define SYSCALL_nbgl_front_draw_img_ID             0x04fa0002
#define SYSCALL_nbgl_front_refresh_area_ID         0x03fa0003
#define SYSCALL_nbgl_front_draw_img_file_ID        0x05fa0004
#define SYSCALL_nbgl_side_draw_rect_ID             0x01fa0005
#define SYSCALL_nbgl_side_draw_horizontal_line_ID  0x03fa0006
#define SYSCALL_nbgl_side_draw_img_ID              0x04fa0007
#define SYSCALL_nbgl_side_refresh_area_ID          0x02fa0008
#define SYSCALL_nbgl_screen_reinit_ID              0x00fa000d
#define SYSCALL_nbgl_front_draw_img_rle_ID         0x05fa0010
#define SYSCALL_nbgl_front_control_area_masking_ID 0x03fa0012

#ifdef HAVE_SE_EINK_DISPLAY
#define SYSCALL_nbgl_wait_pipeline_ID 0x00fa0011
#endif

#ifdef HAVE_STAX_DISPLAY_FAST_MODE
#define SYSCALL_nbgl_screen_update_temperature_ID 0x01fa0011
#endif  // HAVE_STAX_DISPLAY_FAST_MODE

#ifdef HAVE_STAX_CONFIG_DISPLAY_FAST_MODE
#define SYSCALL_nbgl_screen_config_fast_mode_ID 0x00fa0012
#endif  // HAVE_STAX_CONFIG_DISPLAY_FAST_MODE
#endif  // HAVE_NBGL

#ifdef HAVE_BACKGROUND_IMG
#define SYSCALL_fetch_background_img  0x01fa0009
#define SYSCALL_delete_background_img 0x01fa000a
#endif

#ifdef HAVE_SE_TOUCH
#define SYSCALL_touch_get_last_info_ID   0x01fa000b
#define SYSCALL_touch_exclude_borders_ID 0x01fa000d
#define SYSCALL_touch_set_state_ID       0x01fa000e
#ifdef HAVE_TOUCH_READ_DEBUG_DATA_SYSCALL
#define SYSCALL_touch_debug_ID 0x03fa000f
#endif

#endif  // HAVE_SE_TOUCH
