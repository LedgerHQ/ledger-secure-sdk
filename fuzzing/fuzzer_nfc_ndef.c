#include "nfc_ndef.h"
#include <stddef.h>
#include <string.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char out_string[NFC_NDEF_MAX_SIZE];
    /*
    if(size == sizeof(ndef_struct_t)){
        ndef_struct_t ndef_message;
        memcpy(&ndef_message, data, sizeof(ndef_struct_t));
        ndef_message.ndef_type = ndef_message.ndef_type % 2 + 1;
        ndef_message.uri_id = ndef_message.uri_id % 37;
        ndef_message.text[NFC_TEXT_MAX_LEN] = '\0';
        ndef_message.info[NFC_INFO_MAX_LEN] = '\0';
        os_ndef_to_string(&ndef_message, out_string);
    }
    
    */
    if(size == sizeof(ndef_struct_t)){
        ndef_struct_t *ndef_message = (ndef_struct_t*)data;
        os_ndef_to_string2(ndef_message, out_string);
    }
    //os_get_uri_header(uri_id, out_string);
    return 0;
}
