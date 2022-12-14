/**
 * @file nbgl_fonts.h
 * Fonts types of the new BOLOS Graphical Library
 *
 */

#ifndef NBGL_FONTS_H
#define NBGL_FONTS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "nbgl_types.h"

/*********************
 *      DEFINES
 *********************/
#define PIC_CHAR(x) ((const nbgl_font_character_t *)PIC(x))
#define PIC_BMP(x) ((uint8_t const *)PIC(x))

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief structure defining an ASCII character (except the bitmap)
 *
 */
typedef struct {
  uint8_t char_width;        ///< width of character in pixels
  uint8_t bitmap_byte_count; ///< number of bytes used in chars buffer for this character
  uint16_t bitmap_offset;    ///< offset of this character in chars buffer
} nbgl_font_character_t;

/**
 * @brief structure defining an ASCII font
 *
 */
typedef struct {
  uint8_t font_id;          ///< ID of the font, from @ref nbgl_font_id_e
  uint8_t bpp;              ///< number of bits per pixels
  uint8_t char_height;      ///< height of all characters in pixels
  uint8_t baseline_height;  ///< baseline of all characters in pixels
  uint8_t line_height;      ///< height of a line for all characters in pixels
  uint8_t char_kerning;     ///< kerning for the font
  uint8_t first_char;       ///< ASCII code of the first character in \b bitmap and in \b characters fields
  uint8_t last_char;        ///< ASCII code of the last character in \b bitmap and in \b characters fields
  const nbgl_font_character_t *const characters; ///< array containing definitions of all characters
  uint8_t const *bitmap;    ///< array containing bitmaps of all characters
} nbgl_font_t;

#define BAGL_ENCODING_LATIN1  0
#define BAGL_ENCODING_UTF8    1
#define BAGL_ENCODING_DEFAULT BAGL_ENCODING_UTF8

/**
 * @brief structure defining a unicode character (except the bitmap)
 *
 */
typedef struct {
  uint32_t char_unicode;    ///< unicode = plane value from 0 to 16 then 16-bit code.
  uint8_t char_width;        ///< width of character in pixels
  uint8_t bitmap_byte_count; ///< number of bytes used in chars buffer for this character
  uint16_t bitmap_offset;    ///< offset of this character in chars buffer
} nbgl_font_unicode_character_t;

/**
 * @brief structure defining a unicode font
 *
 */
typedef struct {
  uint8_t font_id;          ///< ID of the font, from @ref nbgl_font_id_e
  uint8_t bpp;              ///< number of bits per pixels
  uint8_t char_height;      ///< height of all characters in pixels
  uint8_t baseline_height;  ///< baseline of all characters in pixels
  uint8_t line_height;      ///< height of a line for all characters in pixels
  uint8_t char_kerning;     ///< kerning for the font
  uint32_t first_unicode_char;      ///< unicode of the first character in \b bitmap and in \b characters fields
  uint32_t last_unicode_char;       ///< unicode of the first character in \b bitmap and in \b characters fields
#if !defined(HAVE_LANGUAGE_PACK)
  // When using language packs, those 2 pointers does not exists
  const nbgl_font_unicode_character_t * const characters; ///< array containing definitions of all characters
  uint8_t const * bitmap;    ///< array containing bitmaps of all characters
#endif //!defined(HAVE_LANGUAGE_PACK)
} nbgl_font_unicode_t;

typedef enum {
  BAGL_FONT_INTER_REGULAR_24px,
  BAGL_FONT_INTER_SEMIBOLD_24px,
  BAGL_FONT_INTER_REGULAR_32px,
  BAGL_FONT_HM_ALPHA_MONO_MEDIUM_32px,
  BAGL_FONT_LAST // MUST ALWAYS BE THE LAST, FOR AUTOMATED INVALID VALUE CHECKS
} nbgl_font_id_e;


/**********************
 * GLOBAL PROTOTYPES
 **********************/
const nbgl_font_t* nbgl_font_getFont(unsigned int fontId);
const nbgl_font_t *nbgl_getFont(nbgl_font_id_e fontId);
uint16_t nbgl_getSingleLineTextWidth(nbgl_font_id_e fontId, const char* text);
uint16_t nbgl_getSingleLineTextWidthInLen(nbgl_font_id_e fontId, const char* text, uint16_t maxLen);
uint16_t nbgl_getTextWidth(nbgl_font_id_e fontId, const char* text);
uint16_t nbgl_getCharWidth(nbgl_font_id_e fontId, const char *text);
uint8_t nbgl_getFontHeight(nbgl_font_id_e fontId);
uint8_t nbgl_getFontLineHeight(nbgl_font_id_e fontId);
uint16_t nbgl_getTextNbLines(const char *text);
uint16_t nbgl_getTextHeight(nbgl_font_id_e fontId, const char*text);
uint16_t nbgl_getTextLength(const char*text);
void nbgl_getTextMaxLenAndWidth(nbgl_font_id_e fontId, const char* text, uint16_t maxWidth, uint16_t *len, uint16_t *width, bool wrapping);
uint16_t nbgl_getTextNbLinesInWidth(nbgl_font_id_e fontId, const char* text, uint16_t maxWidth, bool wrapping);
uint16_t nbgl_getTextHeightInWidth(nbgl_font_id_e fontId, const char*text, uint16_t maxWidth, bool wrapping);
bool nbgl_getTextMaxLenAndWidthFromEnd(nbgl_font_id_e fontId, const char* text, uint16_t maxWidth, uint16_t *len, uint16_t *width);
bool nbgl_getTextMaxLenInNbLines(nbgl_font_id_e fontId, const char* text, uint16_t maxWidth, uint16_t maxNbLines, uint16_t *len);
void nbgl_textWrapOnNbLines(nbgl_font_id_e fontId, const char* text, uint16_t maxWidth, uint8_t nbLines);

uint32_t nbgl_popUnicodeChar(uint8_t **text, uint16_t *text_length, bool *is_unicode);
const nbgl_font_unicode_t* nbgl_getUnicodeFont(nbgl_font_id_e font_id,
                                                 nbgl_font_unicode_character_t **unicode_characters,
                                                 uint8_t **unicode_bitmap);
const nbgl_font_unicode_character_t *nbgl_getUnicodeFontCharacter(uint32_t unicode,
                                                           nbgl_font_unicode_character_t *unicode_characters);

/**********************
 *      MACROS
 **********************/
#define IS_UNICODE(__value) ((__value) > 0xF0)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NBGL_FONTS_H */
