/* Absolution dispatcher for nbgl_page content construction (stateless).
 *
 * Feeds fuzzed content to three nbgl_page entry points:
 *   - nbgl_pageDrawGenericContent() with 6 content types
 *     (TAG_VALUE_LIST / SWITCHES_LIST / INFOS_LIST / CHOICES_LIST /
 *      BARS_LIST / CENTERED_INFO)
 *   - nbgl_pageDrawInfo()
 *   - nbgl_pageDrawConfirmation()
 *
 * These exercise the page + layout object-tree builders on attacker-influenced
 * content (the shape a transaction-review / settings screen renders). The
 * display/HAL layer is mocked (fuzzing/mock/nbgl) and the font/raster engine is
 * stubbed, so this catches OOB / logic bugs in page construction (pagination,
 * item counting, string handling) rather than pixel output. All strings are
 * carved from the fuzz tail and NUL-terminated in a bounded pool; page structs
 * only hold pointers into it.
 */

#include "mocks.h"
#include "parser.h"
#include "scenario_layout.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "nbgl_content.h"
#include "nbgl_page.h"

#define FUZZ_PREFIX_SIZE_FALLBACK 0
#define FUZZ_CTRL_OFF             SCEN_CTRL_OFF
#define FUZZ_CTRL_LEN             SCEN_CTRL_LEN
#define fuzz_lane_is_structured(data, ps) \
    ((ps) > FUZZ_CTRL_OFF && (data)[FUZZ_CTRL_OFF] > FUZZ_STRUCTURED_LANE_THRESHOLD)

#include "fuzz_mutator.h"
#include "fuzz_layout_check.h"

size_t LLVMFuzzerCustomMutator(uint8_t *data, size_t size, size_t max_size, unsigned int seed)
{
    return fuzz_custom_mutator(data, size, max_size, seed);
}

#include "fuzz_harness.h"

const fuzz_command_spec_t fuzz_commands[] = {
    {.cla = 0x00, .ins = 0x01},
};
const size_t fuzz_n_commands = 1;

/* ---- bounded reader over the fuzz tail ---- */
#define MAX_ITEMS 8                   /* pairs / switches / infos / choices / bars */
#define MAX_STR   48                  /* per string, including NUL                 */
#define STR_POOL  (2 * MAX_ITEMS + 6) /* item strings + title + description texts   */

typedef struct {
    const uint8_t *p;
    size_t         len;
    size_t         off;
} cursor_t;

static uint8_t rd_u8(cursor_t *c)
{
    return (c->off < c->len) ? c->p[c->off++] : 0;
}

/* string backing store; page structs only keep pointers into it */
static char     g_pool[STR_POOL][MAX_STR];
static unsigned g_used;

static char *next_slot(void)
{
    return g_pool[(g_used < STR_POOL) ? g_used++ : (STR_POOL - 1)];
}

/* Carve a NUL-terminated string of fuzzed length (< MAX_STR) from the tail. */
static const char *rd_str(cursor_t *c)
{
    char   *dst = next_slot();
    uint8_t n   = rd_u8(c) % MAX_STR; /* 0 .. MAX_STR-1 */
    uint8_t i   = 0;
    for (; i < n && c->off < c->len; i++) {
        char ch = (char) c->p[c->off++];
        dst[i]  = (ch == '\0') ? ' ' : ch; /* keep it a single C string */
    }
    dst[i] = '\0';
    return dst;
}

static void build_generic(cursor_t *c)
{
    nbgl_pageContent_t content;
    memset(&content, 0, sizeof(content));
    content.title            = rd_str(c);
    content.isTouchableTitle = rd_u8(c) & 1;
    content.titleToken       = rd_u8(c);
    content.topRightIcon     = NULL;

    /* item arrays live on the stack for the duration of this call */
    nbgl_contentTagValue_t pairs[MAX_ITEMS];
    nbgl_contentSwitch_t   switches[MAX_ITEMS];
    const char            *infoTypes[MAX_ITEMS];
    const char            *infoContents[MAX_ITEMS];
    const char            *names[MAX_ITEMS];
    const char            *barTexts[MAX_ITEMS];
    uint8_t                barTokens[MAX_ITEMS];

    uint8_t sel = rd_u8(c) % 6;
    uint8_t n   = rd_u8(c) % (MAX_ITEMS + 1); /* 0 .. MAX_ITEMS */

    switch (sel) {
        case 0: /* TAG_VALUE_LIST — the transaction-review shape */
            memset(pairs, 0, sizeof(pairs));
            for (uint8_t i = 0; i < n; i++) {
                pairs[i].item  = rd_str(c);
                pairs[i].value = rd_str(c);
            }
            content.type                            = TAG_VALUE_LIST;
            content.tagValueList.pairs              = pairs;
            content.tagValueList.nbPairs            = n;
            content.tagValueList.smallCaseForValue  = rd_u8(c) & 1;
            content.tagValueList.wrapping           = rd_u8(c) & 1;
            content.tagValueList.nbMaxLinesForValue = rd_u8(c) % 8;
            break;

        case 1: /* SWITCHES_LIST */
            memset(switches, 0, sizeof(switches));
            for (uint8_t i = 0; i < n; i++) {
                switches[i].text      = rd_str(c);
                switches[i].subText   = rd_str(c);
                switches[i].initState = rd_u8(c) & 1;
                switches[i].token     = rd_u8(c);
            }
            content.type                    = SWITCHES_LIST;
            content.switchesList.switches   = switches;
            content.switchesList.nbSwitches = n;
            break;

        case 2: /* INFOS_LIST */
            for (uint8_t i = 0; i < n; i++) {
                infoTypes[i]    = rd_str(c);
                infoContents[i] = rd_str(c);
            }
            content.type                   = INFOS_LIST;
            content.infosList.infoTypes    = infoTypes;
            content.infosList.infoContents = infoContents;
            content.infosList.nbInfos      = n;
            break;

        case 3: /* CHOICES_LIST (radio buttons) */
            for (uint8_t i = 0; i < n; i++) {
                names[i] = rd_str(c);
            }
            content.type                   = CHOICES_LIST;
            content.choicesList.names      = names;
            content.choicesList.localized  = false;
            content.choicesList.nbChoices  = n;
            content.choicesList.initChoice = n ? (rd_u8(c) % n) : 0;
            content.choicesList.token      = rd_u8(c);
            break;

        case 4: /* BARS_LIST (touchable bars) */
            for (uint8_t i = 0; i < n; i++) {
                barTexts[i]  = rd_str(c);
                barTokens[i] = rd_u8(c);
            }
            content.type              = BARS_LIST;
            content.barsList.barTexts = barTexts;
            content.barsList.tokens   = barTokens;
            content.barsList.nbBars   = n;
            break;

        default: /* CENTERED_INFO */
            content.type               = CENTERED_INFO;
            content.centeredInfo.text1 = rd_str(c);
            content.centeredInfo.text2 = rd_str(c);
#ifdef HAVE_SE_TOUCH
            content.centeredInfo.text3 = rd_str(c);
#endif
            content.centeredInfo.icon  = NULL;
            content.centeredInfo.onTop = rd_u8(c) & 1;
            content.centeredInfo.style = (nbgl_contentCenteredInfoStyle_t) (rd_u8(c) % 4);
            break;
    }

    /* optional multi-page navigation */
    nbgl_pageNavigationInfo_t  nav;
    nbgl_pageNavigationInfo_t *navp = NULL;
    if (rd_u8(c) & 1) {
        memset(&nav, 0, sizeof(nav));
        nav.nbPages    = rd_u8(c) % 8;
        nav.activePage = nav.nbPages ? (rd_u8(c) % nav.nbPages) : 0;
        nav.navType    = (rd_u8(c) & 1) ? NAV_WITH_BUTTONS : NAV_WITH_TAP;
        if (nav.navType == NAV_WITH_TAP) {
            // The tap path passes quitText/nextPageText straight to the footer
            // builder with no NULL guard (unlike the buttons path below), so a
            // real tap-nav caller always sets them. Feed non-NULL to avoid
            // false-positive NULL-derefs and to exercise footer rendering.
            nav.navWithTap.quitText      = rd_str(c);
            nav.navWithTap.nextPageText  = rd_str(c);
            nav.navWithTap.nextPageToken = rd_u8(c);
            nav.navWithTap.backButton    = rd_u8(c) & 1;
            nav.navWithTap.backToken     = rd_u8(c);
        }
        else {
            // The buttons path explicitly handles quitText == NULL, so leave it
            // optionally NULL to keep that branch reachable.
            nav.navWithButtons.quitText             = (rd_u8(c) & 1) ? rd_str(c) : NULL;
            nav.navWithButtons.quitButton           = rd_u8(c) & 1;
            nav.navWithButtons.backButton           = rd_u8(c) & 1;
            nav.navWithButtons.visiblePageIndicator = rd_u8(c) & 1;
            nav.navWithButtons.navToken             = rd_u8(c);
        }
        navp = &nav;
    }

    nbgl_page_t *page = nbgl_pageDrawGenericContent(NULL, navp, &content);
    if (page != NULL) {
        nbgl_pageRelease(page);
    }
}

static void build_info(cursor_t *c)
{
    nbgl_pageInfoDescription_t info;
    memset(&info, 0, sizeof(info)); /* centeredInfo left empty (no icon/text) */
    info.footerText       = rd_str(c);
    info.footerToken      = rd_u8(c);
    info.tapActionText    = rd_str(c);
    info.tapActionToken   = rd_u8(c);
    info.actionButtonText = rd_str(c);
    info.isSwipeable      = rd_u8(c) & 1;

    nbgl_page_t *page = nbgl_pageDrawInfo(NULL, NULL, &info);
    if (page != NULL) {
        nbgl_pageRelease(page);
    }
}

static void build_confirmation(cursor_t *c)
{
    nbgl_pageConfirmationDescription_t conf;
    memset(&conf, 0, sizeof(conf)); /* centeredInfo left empty */
    conf.confirmationText  = rd_str(c);
    conf.cancelText        = rd_str(c);
    conf.confirmationToken = rd_u8(c);
    conf.cancelToken       = rd_u8(c);
    conf.modal             = rd_u8(c) & 1;

    nbgl_page_t *page = nbgl_pageDrawConfirmation(NULL, &conf);
    if (page != NULL) {
        nbgl_pageRelease(page);
    }
}

void fuzz_app_reset(void) {}

void fuzz_app_dispatch(void *cmd)
{
    (void) cmd;
    if (!fuzz_tail_ptr || fuzz_tail_len == 0) {
        return;
    }

    g_used     = 0;
    cursor_t c = {.p = fuzz_tail_ptr, .len = fuzz_tail_len, .off = 0};

    /* entry-point selector: weight generic content 2x */
    switch (rd_u8(&c) % 4) {
        case 0:
        case 1:
            build_generic(&c);
            break;
        case 2:
            build_info(&c);
            break;
        default:
            build_confirmation(&c);
            break;
    }
}

int fuzz_entry(const uint8_t *data, size_t size)
{
    return fuzz_harness_entry(data, size);
}
