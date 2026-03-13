/*****************************************************************************
 *   (c) 2026 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "os_apdu.h"
#include "address_book.h"
#include "status_words.h"
#include "external_address.h"
#include "ledger_account.h"

#if defined(HAVE_ADDRESS_BOOK)

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/
#define P1_ADD_EXTERNAL_ADDRESS    0x00
#define P1_EDIT_EXTERNAL_ADDRESS   0x01
#define P1_EDIT_CONTACT            0x02
#define P1_REGISTER_LEDGER_ACCOUNT 0x03
#define P1_RENAME_LEDGER_ACCOUNT   0x04

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
bolos_err_t addr_book_handle_apdu(uint8_t *buffer, size_t buffer_len, uint8_t p1)
{
    bolos_err_t err = SWO_CONDITIONS_NOT_SATISFIED;

    switch (p1) {
        case P1_ADD_EXTERNAL_ADDRESS:
            err = add_external_address(buffer, buffer_len);
            break;

        case P1_EDIT_EXTERNAL_ADDRESS:
            break;

        case P1_EDIT_CONTACT:
            err = edit_contact(buffer, buffer_len);
            break;

        case P1_REGISTER_LEDGER_ACCOUNT:
            err = add_ledger_account(buffer, buffer_len);
            break;

        case P1_RENAME_LEDGER_ACCOUNT:
            break;

        default:
            break;
    }

    return err;
}
#endif  // HAVE_ADDRESS_BOOK
