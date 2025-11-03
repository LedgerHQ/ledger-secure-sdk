#pragma once

// Default Status Word (ISO7816)
// refer to https://www.eftlab.com/knowledge-base/complete-list-of-apdu-responses

// Proprietary error codes
#define SWO_SEC_PIN_15 0x5515

// 61 -- Normal processing, lower byte indicates the amount of data to be retrieved
#define SWO_RESPONSE_BYTES_AVAILABLE         0x6100
// 62 -- Warning, the state of persistent memory is unchanged. The command succeeded, possibly with
// restrictions. Typically used to signal blocked applications
#define SWO_EOF_REACHED_BEFORE_LE            0x6282
#define SWO_SELECTED_FILE_INVALID            0x6284
#define SWO_NO_INPUT_DATA_AVAILABLE          0x6285
// 63 -- Warning, the state of persistent memory is changed. Typically used to indicate the number
// of attempts left on a PIN code after a failure
#define SWO_KEY_NUMBER_INVALID               0x6388
#define SWO_KEY_LENGTH_INCORRECT             0x6389
#define SWO_VERIFY_FAILED                    0x63c0  // lower 4-bits indicates the number of attempts left
#define SWO_MORE_DATA_EXPECTED               0x63f1
// 64 -- Execution error, the state of persistent memory is unchanged
#define SWO_EXECUTION_ERROR                  0x6400
#define SWO_COMMAND_TIMEOUT                  0x6401
// 65 -- Execution error, the state of persistent memory is changed
#define SWO_MEMORY_WRITE_ERROR               0x6501
#define SWO_MEMORY_FAILURE                   0x6581
// 66 -- Security-related issues
#define SWO_SECURITY_ISSUE                   0x6600
// 67 -- Transport error. The length is incorrect
#define SWO_WRONG_LENGTH                     0x6700
// 68 -- Functions in CLA not supported
#define SWO_NOT_SUPPORTED_ERROR_NO_INFO      0x6800
#define SWO_LOGICAL_CHANNEL_NOT_SUPPORTED    0x6881
#define SWO_SECURE_MESSAGING_NOT_SUPPORTED   0x6882
#define SWO_LAST_COMMAND_OF_CHAIN_EXPECTED   0x6883
#define SWO_COMMAND_CHAINING_NOT_SUPPORTED   0x6884
// 69 -- Command not allowed
#define SWO_COMMAND_ERROR_NO_INFO            0x6900
#define SWO_COMMAND_NOT_ACCEPTED             0x6901
#define SWO_COMMAND_NOT_ALLOWED              0x6980
#define SWO_SUBCOMMAND_NOT_ALLOWED           0x6981
#define SWO_SECURITY_CONDITION_NOT_SATISFIED 0x6982
#define SWO_AUTH_METHOD_BLOCKED              0x6983
#define SWO_REFERENCED_DATA_BLOCKED          0x6984
#define SWO_CONDITIONS_NOT_SATISFIED         0x6985
#define SWO_COMMAND_NOT_ALLOWED_EF           0x6986
#define SWO_EXPECTED_SECURE_MSG_OBJ_MISSING  0x6987
#define SWO_INCORRECT_SECURE_MSG_DATA_OBJ    0x6988
#define SWO_PERMISSION_DENIED                0x69f0
// 6A -- Wrong parameters (with details)
#define SWO_PARAMETER_ERROR_NO_INFO          0x6a00
#define SWO_INCORRECT_DATA                   0x6a80
#define SWO_FUNCTION_NOT_SUPPORTED           0x6a81
#define SWO_FILE_NOT_FOUND                   0x6a82
#define SWO_RECORD_NOT_FOUND                 0x6a83
#define SWO_INSUFFICIENT_MEMORY              0x6a84
#define SWO_INCONSISTENT_TLV_STRUCT          0x6a85
#define SWO_INCORRECT_P1_P2                  0x6a86
#define SWO_WRONG_DATA_LENGTH                0x6a87
#define SWO_REFERENCED_DATA_NOT_FOUND        0x6a88
#define SWO_FILE_ALREADY_EXISTS              0x6a89
#define SWO_DF_NAME_ALREADY_EXISTS           0x6a8a
#define SWO_WRONG_PARAMETER_VALUE            0x6af0
// 6B -- Wrong parameters P1-P2
#define SWO_WRONG_P1_P2                      0x6b00
// 6C -- Wrong Le field. lower byte indicates the appropriate length
#define SWO_INCORRECT_P3_LENGTH              0x6c00
// 6D -- The instruction code is not supported
#define SWO_INVALID_INS                      0x6d00
// 6E -- The instruction class is not supported
#define SWO_INVALID_CLA                      0x6e00
// 6F -- No precise diagnosis is given
#define SWO_UNKNOWN                          0x6f00
// 9- --
#define SWO_SUCCESS                          0x9000
#define SWO_PIN_NOT_SUCCESFULLY_VERIFIED     0x9004
#define SWO_RESULT_OK                        0x9100
#define SWO_STATES_STATUS_WRONG              0x9101
#define SWO_COMMAND_CODE_NOT_SUPPORTED       0x911c
#define SWO_WRONG_LENGTH_FOR_INS             0x917e
#define SWO_NO_EF_SELECTED                   0x9400
#define SWO_ADDRESS_RANGE_EXCEEDED           0x9402
#define SWO_FID_NOT_FOUND                    0x9404
#define SWO_PARSE_ERROR                      0x9405
#define SWO_NO_PIN_DEFINED                   0x9802
#define SWO_ACCESS_CONDITION_NOT_FULFILLED   0x9804

// Notes:
// - The SWO codes are defined based on the ISO/IEC 7816-4 standard for smart cards.
//
// - 61XX and 6CXX are different.
//   When a command returns 61XX, its process is normally completed,
//   and it indicates the number of available bytes;
//   it then expects a GET RESPONSE command with the appropriate length.
//   When a command returns 6CXX, its process has been aborted,
//   and it expects the command to be reissued.
//   As mentioned above, 61XX indicates a normal completion,
//   and 6CXX is considered as a transport error (defined in ISO7817-3).
//
// - Except for 63XX and 65XX, which warn that the persistent content has been changed,
//   other status word should be used when the persistent content of the application is unchanged.
//
// - Status words 67XX, 6BXX, 6DXX, 6EXX, and 6FXX, where XX is not 0, are proprietary status words,
//   as well as 9YYY, where YYY is not 000.
