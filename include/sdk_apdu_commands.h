#ifndef SDK_APDU_COMMANDS_H
#define SDK_APDU_COMMANDS_H

#if !defined(HAVE_BOLOS_NO_DEFAULT_APDU)
/** Instruction class */
#define DEFAULT_APDU_CLA 0xB0

/**
 * @brief Instruction code with CLA = 0xB0 to get the version.
 * @details If the OS is running, it returns the name "BOLOS"
 *          and the OS version. If an application is running,
 *          it returns the application name and its version.
 *
 * - Command APDU
 * |FIELD |LENGTH |VALUE |DESCRIPTION       |
 * |------|-------|------|------------------|
 * |CLA   |0x01   |0xB0  |Instruction class |
 * |INS   |0x01   |0x01  |Instruction code  |
 * |P1    |0x01   |0x00  |None              |
 * |P2    |0x01   |0x00  |None              |
 * |LC    |0x01   |0x00  |No data           |
 *
 * - Response APDU
 * |DATA        |LENGTH      |DESCRIPTION                           |
 * |------------|------------|--------------------------------------|
 * |NAME_LEN    |0x01        |Length of the running process name    |
 * |NAME        |NAME_LEN    |Running process (OS or app)           |
 * |VERSION_LEN |0x01        |Version length of the running process |
 * |VERSION     |VERSION_LEN |Version of the running process        |
 * |STATUS_WORD |0x02        |0x9000 on success                     |
 */
#define DEFAULT_APDU_INS_GET_VERSION 0x01

#if defined(HAVE_SEED_COOKIE)
/**
 * @brief Instruction code with CLA = 0xB0 to get a hash of
 *        a public key derived from the seed.
 * @details The hash is computed by applying SHA512
 *          on the public key derived from the seed
 *          through a specific path.
 *
 * - Command APDU
 * |FIELD |LENGTH |VALUE |DESCRIPTION       |
 * |------|-------|------|------------------|
 * |CLA   |0x01   |0xB0  |Instruction class |
 * |INS   |0x01   |0x02  |Instruction code  |
 * |P1    |0x01   |0x00  |None              |
 * |P2    |0x01   |0x00  |None              |
 * |LC    |0x01   |0x00  |No data           |
 *
 * - Response APDU
 * |DATA        |LENGTH | DESCRIPTION                    |
 * |------------|-------|--------------------------------|
 * |PK_HASH     |0x200  | Hash of the derived public key |
 * |STATUS_WORD |0x02   | 0x9000 on success              |
 */
#define DEFAULT_APDU_INS_GET_SEED_COOKIE 0x02
#endif

#if defined(DEBUG_OS_STACK_CONSUMPTION)
#define DEFAULT_APDU_INS_STACK_CONSUMPTION 0x57
#endif  // DEBUG_OS_STACK_CONSUMPTION

/**
 * @brief Instruction code with CLA = 0xB0 to exit
 *        the running application.
 *
 * - Command APDU
 * |FIELD |LENGTH |VALUE |DESCRIPTION       |
 * |------|-------|------|------------------|
 * |CLA   |0x01   |0xB0  |Instruction class |
 * |INS   |0x01   |0xA7  |Instruction code  |
 * |P1    |0x01   |0x00  |None              |
 * |P2    |0x01   |0x00  |None              |
 * |LC    |0x01   |0x00  |No data           |
 *
 * - Response APDU
 * |DATA        |LENGTH | DESCRIPTION       |
 * |------------|-------|-------------------|
 * |STATUS_WORD |0x02   | 0x9000 on success |
 */
#define DEFAULT_APDU_INS_APP_EXIT 0xA7

#if defined(HAVE_LEDGER_PKI)
/**
 * @brief Instruction code with CLA = 0xB0 to load a certificate.
 * @details
 * - Command APDU
 * |FIELD |LENGTH  |VALUE     |DESCRIPTION        |
 * |------|--------|----------|-------------------|
 * |CLA   |0x01    |0xB0      |Instruction class  |
 * |INS   |0x01    |0x06      |Instruction code   |
 * |P1    |0x01    |KEY_USAGE |Key usage          |
 * |P2    |0x01    |0x00      |None               |
 * |LC    |0x01    |DATA_LEN  |DATA length        |
 * |DATA  |0x01    |CERT_LEN  |Certificate length |
 * |DATA  |CERT_LEN|CERT      |Certificate        |
 *
 * - Response APDU
 * |DATA             |LENGTH           | DESCRIPTION       |
 * |-----------------|-----------------|-------------------|
 * |STATUS_WORD      |0x02             | 0x9000 on success |
 */
#define DEFAULT_APDU_INS_LOAD_CERTIFICATE 0x06
#endif  // HAVE_LEDGER_PKI

#endif  // !HAVE_BOLOS_NO_DEFAULT_APDU

#endif /* SDK_APDU_COMMANDS_H */
