#ifndef NBGL_DRIVER_MASKING_H_
#define NBGL_DRIVER_MASKING_H_

#include <stdbool.h>
#include <stdint.h>
#include "nbgl_types.h"

bool    nbgl_driver_masking_isMaskingActive(void);
uint8_t nbgl_driver_masking_applyOn4bppPix(uint16_t x, uint16_t y, uint8_t pix_4bpp);
void    nbgl_driver_masking_controlMasking(uint8_t masked_index, nbgl_area_t *area_or_null);
bool    nbgl_driver_masking_verticalSegmentCheck(uint16_t  drawing_area_limit_y,
                                                 uint16_t  x0,
                                                 uint16_t  y0,
                                                 uint16_t  segment_height,
                                                 uint16_t *out_y_start_masking,
                                                 uint16_t *out_y_stop_masking);
#endif  // NBGL_DRIVER_MASKING_H_
