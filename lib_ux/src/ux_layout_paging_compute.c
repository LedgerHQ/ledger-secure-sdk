#include "os_helpers.h"
#include "os_pic.h"
#include "ux.h"
#include "ux_layouts.h"
#include "bagl.h"
#include "string.h"
#include "os.h"

#ifdef HAVE_UX_FLOW
#ifdef HAVE_BAGL

static bool is_word_delim(unsigned char c)
{
    // return !((c >= 'a' && c <= 'z')
    //       || (c >= 'A' && c <= 'Z')
    //       || (c >= '0' && c <= '9'));
    return c == ' ' || c == '\n' || c == '-' || c == '_';
}

// return the number of pages to be displayed when current page to show is -1
unsigned int ux_layout_paging_compute(const char               *text_to_split,
                                      unsigned int              page_to_display,
                                      ux_layout_paging_state_t *paging_state,
                                      bagl_font_id_e            font)
{
#ifndef HAVE_FONTS
    UNUSED(font);
#endif

    // reset length and offset of lines
    memset(paging_state->offsets, 0, sizeof(paging_state->offsets));
    memset(paging_state->lengths, 0, sizeof(paging_state->lengths));

    // a page has been asked, but no page exists
    if (page_to_display >= paging_state->count && page_to_display != (unsigned int) -1) {
        return 0;
    }

    // compute offset/length of text of each line for the current page
    unsigned int page   = 0;
    unsigned int line   = 0;
    const char  *start  = (text_to_split ? STRPIC(text_to_split) : G_ux.externalText);
    const char  *start2 = start;
    const char  *end    = start + strlen(start);
    while (start < end) {
        unsigned int len             = 0;
        unsigned int linew           = 0;
        const char  *last_word_delim = start;
        // not reached end of content
        while (start + len < end
               // line is not full
               && linew <= PIXEL_PER_LINE
               // avoid display buffer overflow for each line
               // && len < sizeof(G_ux.string_buffer)-1
        ) {
            // compute new line length
#ifdef HAVE_FONTS
            linew = bagl_compute_line_width(font, 0, start, len + 1, BAGL_ENCODING_DEFAULT);
#else   // HAVE_FONTS
            linew = se_compute_line_width_light(start, len + 1, G_ux.layout_paging.format);
#endif  // HAVE_FONTS
        // if (start[len] )
            if (linew > PIXEL_PER_LINE) {
                // we got a full line
                break;
            }
            unsigned char c = start[len];
            if (is_word_delim(c)) {
                last_word_delim = &start[len];
            }
            len++;
            // new line, don't go further
            if (c == '\n') {
                break;
            }
        }

        // if not splitting line onto a word delimiter, then cut at the previous word_delim, adjust
        // len accordingly (and a wor delim has been found already)
        if (start + len < end && last_word_delim != start && len) {
            // if line split within a word
            if ((!is_word_delim(start[len - 1]) && !is_word_delim(start[len]))) {
                len = last_word_delim - start;
            }
        }

        // fill up the paging structure
        if (page_to_display != (unsigned int) -1 && page_to_display == page
            && page_to_display < paging_state->count) {
            paging_state->offsets[line] = start - start2;
            paging_state->lengths[line] = len;

            // won't compute all pages, we reached the one to display
#if UX_LAYOUT_PAGING_LINE_COUNT > 1
            if (line >= UX_LAYOUT_PAGING_LINE_COUNT - 1)
#endif  // UX_LAYOUT_PAGING_LINE_COUNT
            {
                // a page has been computed
                return 1;
            }
        }

        // prepare for next line
        start += len;

        // skip to next line/page
        line++;
        if (
#if UX_LAYOUT_PAGING_LINE_COUNT > 1
            line >= UX_LAYOUT_PAGING_LINE_COUNT &&
#endif  // UX_LAYOUT_PAGING_LINE_COUNT
            start < end) {
            page++;
            line = 0;
        }
    }

    // return total number of page detected
    return page + 1;
}

#endif  // HAVE_BAGL
#endif  // HAVE_UX_FLOW
