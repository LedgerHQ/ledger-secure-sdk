#include <string.h>
#include "nbgl_driver.h"
#include "nbgl_types.h"
#include "nbgl_driver_masking.h"
#include "os_helpers.h"

#define NB_MAX_MASKS (2)  // Maximum number of managed masks

#define MASKING_AREA_RADIUS            (12)             // Fixed radius size for all masks
#define MASKING_AREA_INNER_COLOR_4BPP  WHITE_4BPP       // Fixed inner color for all masks
#define MASKING_AREA_BORDER_COLOR_4BPP LIGHT_GRAY_4BPP  // Fixed border color for all masks

static nbgl_area_t maskedArea[NB_MAX_MASKS];

// Arrays defining the inside and outside curves of a 12 px radius.
// These values have been defined with a designer.
static const uint8_t QUARTER_CIRCLE_12PX_OUTSIDE_CURVE[]
    = {12, 12, 12, 11, 11, 10, 10, 9, 8, 7, 5, 3};
static const uint8_t QUARTER_CIRCLE_12PX_INSIDE_CURVE[]
    = {12, 12, 12, 11, 11, 10, 10, 9, 8, 6, 4, 1};

/**
 * Indicates position of a point relative to a mask area.
 */
typedef enum PointPositionInMask_s {
    OUTSIDE = 0,
    STROKE  = 1,
    INSIDE  = 2
} PointPositionInMask_t;

/**
 * @brief Enable or disable a mask
 *
 * @param masked_index Index of mask to control (should be below NB_MAX_MASKS)
 * @param area_or_null If null, disable the mask. Else, points to the desired masked area
 * coordinate.
 */
void nbgl_driver_masking_controlMasking(uint8_t masked_index, nbgl_area_t *area_or_null)
{
    if (masked_index >= NB_MAX_MASKS) {
        return;
    }

    if (area_or_null == NULL) {
        // Disable masking
        memset(&maskedArea[masked_index], 0, sizeof(nbgl_area_t));
    }
    else {
        memcpy(&maskedArea[masked_index], area_or_null, sizeof(nbgl_area_t));
    }
}

/**
 * @brief Indicates if a least one masking area is active
 *
 * @return true: masking is active / false otherwise
 */

bool nbgl_driver_masking_isMaskingActive(void)
{
    for (uint8_t i = 0; i < NB_MAX_MASKS; i++) {
        nbgl_area_t *area = &maskedArea[i];
        if ((area->width != 0) && (area->height != 0)) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Calculate position of a point relative to a mask area
 * @param mask Mask area
 * @param x X coordinate of the point
 * @param y Y coordinate of the point
 *
 * @return OUTSIDE / INSIDE / STROKE
 */

static PointPositionInMask_t check_point_in_masked_area(nbgl_area_t *mask, uint16_t x, uint16_t y)
{
    uint16_t width  = mask->width;
    uint16_t height = mask->height;
    uint16_t xmin   = mask->x0;
    uint16_t ymin   = mask->y0;
    uint16_t xmax   = xmin + width - 1;
    uint16_t ymax   = ymin + height - 1;

    if ((width == 0) || (height == 0)) {
        return OUTSIDE;
    }

    // y outside main rectangle
    if ((y < ymin) || (y > ymax)) {
        return OUTSIDE;
    }

    // x outside main rectangle
    if ((x < xmin) || (x > xmax)) {
        return OUTSIDE;
    }

    uint16_t xmin_inner = xmin + MASKING_AREA_RADIUS;
    uint16_t xmax_inner = xmax - MASKING_AREA_RADIUS;
    uint16_t ymin_inner = ymin + MASKING_AREA_RADIUS;
    uint16_t ymax_inner = ymax - MASKING_AREA_RADIUS;

    if ((x > xmin_inner) && (x < xmax_inner) && (y > ymin_inner) && (y < ymax_inner)) {
        return INSIDE;
    }

    // Check if point on a curve
    if (x > xmax_inner) {
        uint16_t index = x - xmax_inner - 1;

        if ((y < ymin_inner)) {
            // Top right curve
            uint16_t threshold_y_out = ymin_inner - QUARTER_CIRCLE_12PX_OUTSIDE_CURVE[index];
            uint16_t threshold_y_in  = ymin_inner - QUARTER_CIRCLE_12PX_INSIDE_CURVE[index];

            if (y < threshold_y_out) {
                return OUTSIDE;
            }
            else if (y > threshold_y_in) {
                return INSIDE;
            }
            else {
                return STROKE;
            }
        }

        if (y > ymax_inner) {
            // Bottom right curve
            uint16_t threshold_y_out = QUARTER_CIRCLE_12PX_OUTSIDE_CURVE[index] + ymax_inner;
            uint16_t threshold_y_in  = QUARTER_CIRCLE_12PX_INSIDE_CURVE[index] + ymax_inner;

            if (y > threshold_y_out) {
                return OUTSIDE;
            }
            else if (y < threshold_y_in) {
                return INSIDE;
            }
            else {
                return STROKE;
            }
        }

        if (x == xmax) {
            return STROKE;
        }
    }
    else if (x < xmin_inner) {
        uint16_t index = xmin_inner - x - 1;

        if (y < ymin_inner) {
            // Top left curve
            uint16_t threshold_y_out = ymin_inner - QUARTER_CIRCLE_12PX_OUTSIDE_CURVE[index];
            uint16_t threshold_y_in  = ymin_inner - QUARTER_CIRCLE_12PX_INSIDE_CURVE[index];

            if (y < threshold_y_out) {
                return OUTSIDE;
            }
            else if (y > threshold_y_in) {
                return INSIDE;
            }
            else {
                return STROKE;
            }
        }

        if (y > ymax_inner) {
            // Bottom left curve
            uint16_t threshold_y_out = QUARTER_CIRCLE_12PX_OUTSIDE_CURVE[index] + ymax_inner;
            uint16_t threshold_y_in  = QUARTER_CIRCLE_12PX_INSIDE_CURVE[index] + ymax_inner;

            if (y > threshold_y_out) {
                return OUTSIDE;
            }
            else if (y < threshold_y_in) {
                return INSIDE;
            }
            else {
                return STROKE;
            }
        }

        if (x == xmin) {
            return STROKE;
        }
    }
    else if (y == ymin) {
        return STROKE;
    }
    else if (y == (ymax)) {
        return STROKE;
    }

    return INSIDE;
}

/**
 * @brief Apply masking on a point (4BPP pixel)
 * @param x X coordinate of the point
 * @param y Y coordinate of the point
 * @param pix_4bpp original color of the point. Returned if the point is not masked.
 *
 * @return 4BPP color to be applied to the point
 */

uint8_t nbgl_driver_masking_applyOn4bppPix(uint16_t x, uint16_t y, uint8_t pix_4bpp)
{
    for (uint8_t i = 0; i < NB_MAX_MASKS; i++) {
        nbgl_area_t *mask      = &maskedArea[i];
        uint8_t      point_loc = check_point_in_masked_area(mask, x, y);

        if (point_loc == INSIDE) {
            return MASKING_AREA_INNER_COLOR_4BPP;
        }
        else if (point_loc == STROKE) {
            return MASKING_AREA_BORDER_COLOR_4BPP;
        }
    }

    // Point is outside of the masking area,
    // return the original pixel value.
    return pix_4bpp;
}

/**
 * @brief Check if a vertical segment meets with a specific mask area
 *
 * @param drawing_area_limit_y Equals to drawing area base y coordinate + drawing area height
 * @param mask Pointer to mask to be checked
 * @param x0 X coordinate of the first point of the checked segment
 * @param y0 Y coordinate of the first point of the checked segment
 * @param segment_height Size in pixels of the checked segment
 * @param out_y_start_masking [out] Masking must be checked beyond this coordinate
 * @param out_y_stop_masking [out] Masking check can be bypassed beyond this coordinate
 *
 * @return true if segment meets a masking area / false otherwise
 *
 */

static bool check_vertical_segment_on_mask(uint16_t     drawing_area_limit_y,
                                           nbgl_area_t *mask,
                                           uint16_t     x0,
                                           uint16_t     y0,
                                           uint16_t     segment_height,
                                           uint16_t    *out_y_start_masking,
                                           uint16_t    *out_y_stop_masking)
{
    uint16_t mask_max_y      = mask->y0 + mask->height;
    uint16_t segment_limit_y = y0 + segment_height;

    // Limit segment height to height drawing area
    if (segment_limit_y > drawing_area_limit_y) {
        segment_limit_y = drawing_area_limit_y;
    }

    // Initialize out_y_start_masking
    if (out_y_start_masking) {
        *out_y_start_masking = segment_limit_y;
    }

    // Initialize out_y_stop_masking
    if (out_y_stop_masking) {
        *out_y_stop_masking = segment_limit_y;
    }

    // Check if mask is active
    if ((mask->width == 0) || (mask->height == 0)) {
        return false;
    }

    if ((x0 < mask->x0) || (x0 >= (mask->x0 + mask->width))) {
        // x out of segment
        return false;
    }

    if (y0 > mask_max_y) {
        // Segment starts below masking area
        return false;
    }

    if ((y0 < mask->y0) && (segment_limit_y < mask->y0)) {
        // Segment does not cross upper masking limit
        return false;
    }

    if ((y0 > mask_max_y) && (segment_limit_y > mask_max_y)) {
        // Segment does not cross lower masking limit
        return false;
    }

    // Segment meets with masked area
    if (out_y_start_masking) {
        if (y0 < mask->y0) {
            *out_y_start_masking = mask->y0;
        }
        else {
            *out_y_start_masking = y0;
        }
    }

    if (out_y_stop_masking) {
        if (mask_max_y < segment_limit_y) {
            *out_y_stop_masking = mask_max_y;
        }
        else {
            *out_y_stop_masking = segment_limit_y;
        }
    }
    return true;
}

/**
 * @brief Check if a vertical segment meets with at least one the configured masking area
 *
 * This function allows to reduce the number of calls to `nbgl_driver_masking_applyOn4bppPix`
 * by checking if the vertical segment needs masking check when being drawn, and by returning
 * the minimum and maximum coordinates where masking check is necessary.
 *
 *
 *                     0                        x0
 *                      ┌──────────────────────────────────────────► x
 *                      │                        .
 *                      │                        .
 *                      │                        .
 *                      │       drawing area     .
 *                      │           ┌───────────────────────────┐
 *                      │           │            .              │
 *                   y0 │...........│.............              │
 *                      │           │            │              │
 *                      │           │            │              │
 *  out_y_start_masking │...........│..┌─────────┼─────┐        │
 *                      │           │  │         │     │        │
 *                      │           │  │ Mask 0  │     │        │
 *                      │           │  │ area    │     │        │
 *                      │           │  │         │     │        │
 *                      │           │  └─────────┼─────┘        │
 *                      │           │            │              │
 *                      │           │        vertical segment   │
 *                      │           │            │              │
 *                      │           │        ┌───┼──────────┐   │
 *                      │           │        │   │  Mask 1  │   │
 *                      │           │        │   │  area    │   │
 *                      │           │        │   │          │   │
 *   out_y_stop_masking │...........│........└───┼──────────┘   │
 *                      │           │            │              │
 *                      │           │            │              │
 *   y0 + segment_height│...........│............│              │
 *                      │           │                           │
 *                      │           │                           │
 *                      │           │                           │
 * y_drawing_area_limit │...........└───────────────────────────┘
 *                      │
 *                      │
 *                      ▼
 *                      y
 *
 * @param drawing_area_limit_y Equals to drawing area base y coordinate + drawing area height
 * @param x0 X coordinate of the first point of the checked segment
 * @param y0 Y coordinate of the first point of the checked segment
 * @param segment_height Size in pixels of the checked segment
 * @param out_y_start_masking [out] Masking must be checked beyond this coordinate
 * @param out_y_stop_masking [out] Masking check can be bypassed beyond this coordinate
 *
 * @return true if segment meets a masking area / false otherwise
 */

bool nbgl_driver_masking_verticalSegmentCheck(uint16_t  drawing_area_limit_y,
                                              uint16_t  x0,
                                              uint16_t  y0,
                                              uint16_t  segment_height,
                                              uint16_t *out_y_start_masking,
                                              uint16_t *out_y_stop_masking)
{
    bool     result = false;
    uint16_t tmp_y_start_masking;
    uint16_t tmp_y_stop_masking;

    for (uint8_t i = 0; i < NB_MAX_MASKS; i++) {
        bool mask_check = check_vertical_segment_on_mask(drawing_area_limit_y,
                                                         &maskedArea[i],
                                                         x0,
                                                         y0,
                                                         segment_height,
                                                         &tmp_y_start_masking,
                                                         &tmp_y_stop_masking);
        if ((i == 0) || (!result && mask_check)) {
            // Init output values
            *out_y_start_masking = tmp_y_start_masking;
            *out_y_stop_masking  = tmp_y_stop_masking;
        }
        else if (mask_check) {
            // Update output values if needed
            if (tmp_y_start_masking < *out_y_start_masking) {
                *out_y_start_masking = tmp_y_start_masking;
            }

            if (tmp_y_stop_masking > *out_y_stop_masking) {
                *out_y_stop_masking = tmp_y_stop_masking;
            }
        }
        result |= mask_check;
    }

    return result;
}
