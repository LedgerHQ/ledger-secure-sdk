#pragma once

/*
 --8<-- [start:swap_lib_calls_intro]
This file is the shared API between the Exchange application and the coin applications started in
Library mode for Exchange.

On Coin application side, the main() of the lib_standard_app has the logic to recognize a library
start from a dashboard start and the logic to dispatch the library commands to the correct handler
function.
--8<-- [end:swap_lib_calls_intro]
*/

#include <stdbool.h>
#include <stdint.h>
#include "swap_caller_app.h"
#include "chain_config.h"

// The os_lib_calls commands that can be called by the Exchange application.
// They are handled by the lib_standard_app main() function.
#define RUN_APPLICATION      1
#define SIGN_TRANSACTION     2
#define CHECK_ADDRESS        3
#define GET_PRINTABLE_AMOUNT 4

/*
 * Amounts are stored as bytes, with a max size of 16 (see protobuf
 * specifications). Max 16B integer is 340282366920938463463374607431768211455
 * in decimal, which is a 32-long char string.
 * The printable amount also contains spaces, the ticker symbol (with variable
 * size, up to 12 in Ethereum for instance) and a terminating null byte, so 50
 * bytes total should be a fair maximum.
 */
#define MAX_PRINTABLE_AMOUNT_SIZE 50

// Structure parameter used by swap_handle_check_address
// --8<-- [start:check_address_parameters_t]
typedef struct check_address_parameters_s {
    // INPUTS //
    // Additional data when dealing with tokens
    // Content is coin application specific
    uint8_t *coin_configuration;
    uint8_t  coin_configuration_length;

    // serialized path, segwit, version prefix, hash used, dictionary etc.
    // fields and serialization format are coin application specific
    uint8_t *address_parameters;
    uint8_t  address_parameters_length;

    // The address to check
    char *address_to_check;

    // Extra content that may be relevant depending on context: memo, calldata, ...
    // Content is coin application specific
    char *extra_id_to_check;

    // OUTPUT //
    // Set to 1 if the address belongs to the device. 0 otherwise.
    int result;
} check_address_parameters_t;
// --8<-- [end:check_address_parameters_t]

// Structure parameter used by swap_handle_get_printable_amount
// --8<-- [start:get_printable_amount_parameters_t]
typedef struct get_printable_amount_parameters_s {
    // INPUTS //
    // Additional data when dealing with tokens
    // Content is coin application specific
    uint8_t *coin_configuration;
    uint8_t  coin_configuration_length;

    // Raw amount in big number format
    uint8_t *amount;
    uint8_t  amount_length;

    // Set to true if the amount to format is the fee of the swap.
    bool is_fee;

    // OUTPUT //
    // Set to the formatted string if the formatting succeeds. 0 otherwise.
    char printable_amount[MAX_PRINTABLE_AMOUNT_SIZE];
} get_printable_amount_parameters_t;
// --8<-- [end:get_printable_amount_parameters_t]

// Structure parameter used by swap_copy_transaction_parameters
// --8<-- [start:create_transaction_parameters_t]
typedef struct create_transaction_parameters_s {
    // INPUTS //
    // Additional data when dealing with tokens
    // Content is coin application specific
    uint8_t *coin_configuration;
    uint8_t  coin_configuration_length;

    // The amount validated on the screen by the user
    uint8_t *amount;
    uint8_t  amount_length;

    // The fees amount validated on the screen by the user
    uint8_t *fee_amount;
    uint8_t  fee_amount_length;

    // The partner address that will receive the funds
    char *destination_address;
    char *destination_address_extra_id;

    // OUTPUT //
    // /!\ This parameter is handled by the lib_standard_app, DO NOT interact
    // with it in the Coin application
    //
    // After reception and signature or refusal of the transaction, the Coin
    // application will return to Exchange. This boolean is used to inform the
    // Exchange application of the result.
    // Set to 1 if the transaction was successfully signed, 0 otherwise.
    uint8_t result;
} create_transaction_parameters_t;
// --8<-- [end:create_transaction_parameters_t]

// --8<-- [start:libargs_t]
// Parameter given through os_lib_call() to the Coin application by the Exchange application.
// They are handled by the lib_standard_app main() function.
typedef struct libargs_s {
    unsigned int         id;
    unsigned int         command;
    coin_chain_config_t *chain_config;
    union {
        check_address_parameters_t        *check_address;
        create_transaction_parameters_t   *create_transaction;
        get_printable_amount_parameters_t *get_printable_amount;
        swap_caller_app_t                 *swap_caller_app;
    };
} libargs_t;
// --8<-- [end:libargs_t]
