
#ifdef HAVE_SWAP

#include "swap_error_code_helpers.h"
#include "swap_utils.h"

__attribute__((noreturn)) void send_swap_error_simple(uint16_t status_word,
                                                      uint8_t  common_error_code,
                                                      uint8_t  application_specific_error_code)
{
    send_swap_error_with_buffers(
        status_word, common_error_code, application_specific_error_code, NULL, 0);
}

__attribute__((noreturn)) void send_swap_error_with_buffer(uint16_t status_word,
                                                           uint8_t  common_error_code,
                                                           uint8_t  application_specific_error_code,
                                                           const buffer_t buffer_data)
{
    send_swap_error_with_buffers(
        status_word, common_error_code, application_specific_error_code, &buffer_data, 1);
}

__attribute__((noreturn)) void send_swap_error_with_buffers(uint16_t status_word,
                                                            uint8_t  common_error_code,
                                                            uint8_t application_specific_error_code,
                                                            const buffer_t *buffer_data,
                                                            size_t          count)
{
    if (!G_called_from_swap) {
        PRINTF("Fatal error, send_swap_error_with_buffers called outside of swap context\n");
        // Don't try to recover, the caller logic has a huge issue
        os_sched_exit(0);
    }
    // Force G_swap_response_ready to true
    G_swap_response_ready = true;

    // Simply prepend a constructed buffer with the error code to the buffer list and use standard
    // io function to send
    uint8_t swap_error_code[2] = {common_error_code, application_specific_error_code};

    // Allocate enough space for the error code and buffers
    buffer_t response[1 + SWAP_ERROR_HELPER_MAX_BUFFER_COUNT] = {0};
    response[0].ptr                                           = (uint8_t *) &swap_error_code;
    response[0].size                                          = sizeof(swap_error_code);

    // Not really an error, let's just truncate without raising
    if (count > SWAP_ERROR_HELPER_MAX_BUFFER_COUNT) {
        PRINTF("send_swap_error_with_buffers truncated from %d to %d\n",
               count,
               SWAP_ERROR_HELPER_MAX_BUFFER_COUNT);
        count = SWAP_ERROR_HELPER_MAX_BUFFER_COUNT;
    }
    // We are only copying the buffer_t structure, not the content
    memcpy(&response[1], buffer_data, count * sizeof(buffer_t));

    // io_send_response_buffers will use the correct flag IO_RETURN_AFTER_TX
    io_send_response_buffers(response, count + 1, status_word);

    // unreachable
    os_sched_exit(0);
}

#endif  // HAVE_SWAP
