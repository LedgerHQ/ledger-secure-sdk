/**
 * @file json_scenario.h
 * @brief screenshots engine implementation
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// You may have to install packet libjson-c-dev
#include <json-c/json.h>

#include "nbgl_use_case.h"
#include "json_scenario.h"
#include "nbgl_touch.h"
#include "nbgl_driver.h"
#include "os_settings.h"
#include "os_id.h"
#include "apps_api.h"
#include "ux_nbgl.h"

Event_t appEvents[] = {
    {"BTC_SIGN",            BTC_SIGN           },
    {"CARDANO_SIGN",        CARDANO_SIGN       },
    {"STELLAR_SIGN",        STELLAR_SIGN       },
    {"STELLAR_STREAM_SIGN", STELLAR_STREAM_SIGN},
    {"BTC_VERIFY_ADDR",     BTC_VERIFY_ADDR    },
    {"CARDANO_VERIFY_ADDR", CARDANO_VERIFY_ADDR},
    {"BTC_OPEN",            BTC_OPEN           },
    {"ETH_OPEN",            ETH_OPEN           },
    {"ETH_OPEN_2",          ETH_OPEN_2         },
    {"MONERO_OPEN",         MONERO_OPEN        },
    {"MON_MESSAGE_SKIP",    MON_MESSAGE_SKIP   },
    {"ETH_MESSAGE_SKIP",    ETH_MESSAGE_SKIP   },
    {"ETH_BLIND",           ETH_BLIND          },
    {"ETH_ENS",             ETH_ENS            },
    {"ETH_DAPP",            ETH_DAPP           },
    {"ETH_SIGN",            ETH_SIGN           },
    {"ETH_VERIFY_ADDR",     ETH_VERIFY_ADDR    },
    {"ETH_VERIFY_MULTISIG", ETH_VERIFY_MULTISIG},
    {"BTC_SHARE_ADDR",      BTC_SHARE_ADDR     },
    {"RECOV_OPEN",          RECOV_OPEN         },
    {"DOGE_SIGN",           DOGE_SIGN          },
    {"DOGE_PRELUDE_SIGN",   DOGE_PRELUDE_SIGN  }
};

// =--------------------------------------------------------------------------=
// Variables
// =--------------------------------------------------------------------------=
static uint32_t       nb_pages;
static ScenarioPage_t pages[NB_MAX_PAGES];
ScenarioPage_t       *current_scenario_page;
char                  scenario_name[100];
static char          *productName;

static char *jsonName;
extern char *dirName;
extern char *propertiesName;

// Set this flag to true if you want a verbose output
bool verbose = false;

const char *control_names[]
    = {"BOTTOM_BUTTON",  "LEFT_BUTTON",    "RIGHT_BUTTON",      "WHOLE_SCREEN", "TOP_RIGHT_BUTTON",
       "BACK_BUTTON",    "SINGLE_BUTTON",  "EXTRA_BUTTON",      "CHOICE_1",     "CHOICE_2",
       "CHOICE_3",       "KEYPAD",         "KEYBOARD",          "ENTERED_TEXT", "VALUE_BUTTON_1",
       "VALUE_BUTTON_2", "VALUE_BUTTON_3", "LONG_PRESS_BUTTON", "TIP_BOX",      "CONTROLS"};

extern void delete_app_storage(unsigned int app_idx);

// =--------------------------------------------------------------------------=
// Return 0 if both strings are identicall, up to % character in str1

uint8_t strxcmp(char *str1, char *str2)
{
    char c1, c2;

    if (!str1 || !str2) {
        return 1;
    }

    do {
        c1 = *str1++;
        c2 = *str2++;
        if (c1 == '%') {
            return 0;
        }
        if (c1 != c2) {
            return 1;
        }
    } while (c1);

    return 0;
}

void store_button_infos(nbgl_area_t *button_area)
{
    UNUSED(button_area);
}

// =--------------------------------------------------------------------------=
// Store the number of lines used by a specific string ID

void store_string_infos(const char    *text,
                        nbgl_font_id_e font_id,
                        nbgl_area_t   *area,
                        bool           wrapping,
                        uint16_t       nb_lines,
                        uint16_t       nb_pages_,
                        bool           bold)
{
    UNUSED(text);
    UNUSED(font_id);
    UNUSED(area);
    UNUSED(wrapping);
    UNUSED(nb_lines);
    UNUSED(nb_pages_);
    UNUSED(bold);
}

// =--------------------------------------------------------------------------=
static int add_name(ScenarioPage_t *page, char *text)
{
    page->name = malloc(strlen(text) + 1);
    if (page->name == NULL) {
        fprintf(stdout, "No enough memory to store the page name \"%s\"!\n", text);
        return 22;
    }
    strcpy(page->name, text);

    return 0;
}

#ifdef HAVE_SE_TOUCH
static uint8_t control_name_to_id(char *control_name)
{
    uint8_t i;
    for (i = 0; i < (NB_CONTROL_IDS - 1); i++) {
        if (!memcmp(control_names[i], control_name, strlen(control_names[i]))) {
            return i + 1;  // ids start at 1
        }
    }
    return 0;
}
static char *getControlName(uint8_t objTouchId, char objTouchSubId)
{
    static char controlName[100];
    if (objTouchId == KEYBOARD_ID) {
        sprintf(controlName, "KEYBOARD_key[%c]", objTouchSubId);
    }
    else if (objTouchId == KEYPAD_ID) {
        sprintf(controlName, "KEYPAD_key[%c]", objTouchSubId);
    }
    else if (objTouchId >= CONTROLS_ID) {
        sprintf(controlName, "GENERIC_CONTROL[%c]", objTouchSubId);
    }
    else if (objTouchId >= 1) {
        sprintf(controlName, "%s", control_names[objTouchId - 1]);
    }
    else {
        sprintf(controlName, "NO_CONTROL");
    }
    return controlName;
}
#endif  // HAVE_SE_TOUCH

static int add_targets(ScenarioPage_t *page, struct json_object *value)
{
    // Strings are stored in an array
    if (json_object_get_type(value) != json_type_array) {
        return 23;
    }
    // fprintf(stdout, "Will handle %d steps...\n", (int)json_object_array_length(value));
    page->steps   = malloc((int) json_object_array_length(value) * sizeof(PageStep_t));
    page->nbSteps = 0;
    for (int i = 0; i < (int) json_object_array_length(value); i++) {
        bool        notConcerned = false;
        char       *name         = NULL;
        PageStep_t *target       = &page->steps[page->nbSteps];

        struct json_object *page_info = json_object_array_get_idx(value, i);

        target->wait         = 0;
        target->wait_initial = 0;
#if defined(TARGET_FLEX)
        target->long_press_wait = 1500;
#elif defined(TARGET_STAX)
        target->long_press_wait = 1200;
#elif defined(TARGET_APEX)
        target->long_press_wait = 1200;
#endif  // TARGETS

#ifdef HAVE_SE_TOUCH
        target->x          = 0;
        target->y          = 0;
        target->objTouchId = 0;
#endif  // HAVE_SE_TOUCH
        target->power_press      = 0;
        target->forced_key_press = 0;
        target->ble_event_idx    = 0xFF;  // not valid
        target->batt_level       = DEFAULT_BATT_LEVEL;
        target->auto_transition  = false;
        target->batt_event_idx   = 0xFF;  // not valid
        target->ux_event_idx     = 0xFF;  // not valid
        target->app_event_idx    = 0xFF;  // not valid
        target->param            = 0;
        target->reset            = false;
        target->os_flags         = 0;
        target->seed_algorithm   = 0xFF;
        json_object_object_foreach(page_info, k, v)
        {
            if (!strcmp(k, "page")) {
                name         = (char *) json_object_get_string(v);
                target->name = malloc(strlen(name) + 1);
                strcpy(target->name, name);
            }
            else if (!strcmp(k, "product")) {  // to restrict a target to a specific product
                name = (char *) json_object_get_string(v);
                if (!strstr(name, productName)) {
                    notConcerned = true;
                    continue;
                }
            }
#ifdef HAVE_SE_TOUCH
            else if (!strcmp(k, "touch")) {
                name        = (char *) json_object_get_string(v);
                char *comma = strchr(name, ',');
                target->x   = atoi(name);
                target->y   = atoi(comma + 1);
            }
            else if (!strcmp(k, "object")) {
                name               = (char *) json_object_get_string(v);
                char *comma        = strchr(name, ',');
                target->objTouchId = control_name_to_id(name);
                if (comma) {
                    target->objTouchSubId = comma[1];
                }
            }
#else   // HAVE_SE_TOUCH
            else if (!strcmp(k, "key")) {
                name = (char *) json_object_get_string(v);
                if (strcmp("left", name) == 0) {
                    target->keyState = BUTTON_LEFT;
                }
                else if (strcmp("right", name) == 0) {
                    target->keyState = BUTTON_RIGHT;
                }
                else if (strcmp("both", name) == 0) {
                    target->keyState = BUTTON_LEFT | BUTTON_RIGHT;
                }
            }
#endif  // HAVE_SE_TOUCH
            else if (!strcmp(k, "wait")) {
                name                 = (char *) json_object_get_string(v);
                target->wait         = atoi(name);
                target->wait_initial = target->wait;
            }
            else if (!strcmp(k, "forced_key_press")) {
                name                     = (char *) json_object_get_string(v);
                target->forced_key_press = atoi(name);
            }
            else if (!strcmp(k, "param")) {
                name          = (char *) json_object_get_string(v);
                target->param = atoi(name);
            }
            else if (!strcmp(k, "auto_transition")) {
                name                    = (char *) json_object_get_string(v);
                target->auto_transition = (strcmp("true", name) == 0) ? true : false;
            }
            else if (!strcmp(k, "reset")) {
                name          = (char *) json_object_get_string(v);
                target->reset = (strcmp("true", name) == 0) ? true : false;
            }
            else if (!strcmp(k, "os_flags")) {
                name             = (char *) json_object_get_string(v);
                target->os_flags = atoi(name);
            }
            else if (!strcmp(k, "seed_algorithm")) {
                name                   = (char *) json_object_get_string(v);
                target->seed_algorithm = atoi(name);
            }
            else if (!strcmp(k, "app_event")) {
                name = (char *) json_object_get_string(v);
                for (uint32_t j = 0; j < sizeof(appEvents) / sizeof(Event_t); j++) {
                    if (strcmp(appEvents[j].name, name) == 0) {
                        target->app_event_idx = j;
                        break;
                    }
                }
            }
        }
        if (!notConcerned) {
            page->nbSteps++;
        }
    }
    page->currentStep = 0;
    page->saved       = false;

    return 0;
}

// =--------------------------------------------------------------------------=

static char *read_json_data(char *path, char *filename)
{
    FILE  *json_file;
    size_t size;
    char  *buffer;
    char   fullname[MAX_PATH];

    snprintf(fullname, sizeof(fullname), "%s%s", path, filename);

    if ((json_file = fopen(fullname, "r")) == NULL) {
        fprintf(stdout, "Unable to read flow JSON file %s!\n", fullname);
        return NULL;
    }

    // Determine file size
    if (fseek(json_file, 0, SEEK_END) != 0) {
        fprintf(stdout, "Error seeking to the end of JSON file %s!\n", fullname);
        fclose(json_file);
        return NULL;
    }

    if ((size = ftell(json_file)) == 0) {
        fprintf(stdout, "Error getting size of JSON file %s!\n", fullname);
        fclose(json_file);
        return NULL;
    }

    if (fseek(json_file, 0, SEEK_SET) != 0) {
        fprintf(stdout, "Error seeking to the start of JSON file %s!\n", fullname);
        fclose(json_file);
        return NULL;
    }

    // Allocate memory and read the file
    if ((buffer = malloc(size + 1)) == NULL) {
        fprintf(stdout, "Error allocating %lu bytes to read JSON file %s!\n", size, fullname);
        fclose(json_file);
        return NULL;
    }
    if (fread(buffer, 1, size, json_file) != size) {
        fprintf(stdout, "Error reading %lu bytes from JSON file %s!\n", size, fullname);
        free(buffer);
        fclose(json_file);
        return NULL;
    }
    buffer[size] = 0;
    fclose(json_file);

    return buffer;
}

// compute hah of the given framebuffer
static int save_hash(char         *framebuffer,
                     uint16_t      width,
                     uint16_t      height,
                     uint8_t      *hash,
                     unsigned int *hash_length)
{
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL) {
        perror("Failed to create context");
        return -1;
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) != 1) {
        perror("Digest initialization failed");
        EVP_MD_CTX_free(mdctx);
        return -1;
    };

    unsigned int remaining_bytes = width * height;
    char        *buffer          = framebuffer;

    while (remaining_bytes > 0) {
        int nb_bytes = MIN(4096, remaining_bytes);
        if (EVP_DigestUpdate(mdctx, buffer, nb_bytes) != 1) {
            perror("Digest update failed");
            EVP_MD_CTX_free(mdctx);
            return -1;
        }
        buffer += nb_bytes;
        remaining_bytes -= nb_bytes;
    }

    if (EVP_DigestFinal_ex(mdctx, hash, hash_length) != 1) {
        perror("Digest finalization failed");
        EVP_MD_CTX_free(mdctx);
        return -1;
    }

    EVP_MD_CTX_free(mdctx);

    return 0;
}

// =--------------------------------------------------------------------------=

void scenario_parse_args(int argc, char **argv)
{
    int i = 1;
    if (argc < 7) {
        fprintf(stderr, "wrong number of args command line \n");
        exit(-1);
    }
    while (i < argc) {
        if (!strcmp("-j", argv[i])) {
            jsonName = argv[i + 1];
            i += 2;
        }
        else if (!strcmp("-d", argv[i])) {
            dirName = argv[i + 1];
            i += 2;
        }
        else if (!strcmp("-p", argv[i])) {
            propertiesName = argv[i + 1];
            i += 2;
        }
        else if (!strcmp("-n", argv[i])) {
            productName = argv[i + 1];
            i += 2;
        }
        else if (!strcmp("-v", argv[i])) {
            verbose = true;
            i += 1;
        }
        else {
            printf("Unknown arg [%s] in command line \n", argv[i]);
            exit(-1);
        }
    }
}

int scenario_parse_json(void)
{
    int                 error_code;
    char               *buffer;
    struct json_object *json_root;

    // Read the JSON file
    if ((buffer = read_json_data("", jsonName)) == NULL) {
        // Error message is displayed in the called function
        return 10;
    }
    if ((json_root = json_tokener_parse(buffer)) == NULL) {
        fprintf(stdout, "An error occurred while parsing JSON file %s!\n", jsonName);
        free(buffer);
        return 11;
    }

    if (json_type_object != json_object_get_type(json_root)) {
        fprintf(stdout, "There is no valid entry in file %s!\n", jsonName);
        json_object_put(json_root);
        free(buffer);
        return 12;
    }
    // Next line is a macro that iterate through keys/values of that JSON object
    json_object_object_foreach(json_root, key, value)
    {
        if (!strcmp(key, "title")) {
            char *title = (char *) json_object_get_string(value);
            strcpy(scenario_name, title);
        }
        else if (!strcmp(key, "product")) {
            char *name = (char *) json_object_get_string(value);
            if (!strstr(name, productName)) {
                break;
            }
        }
        else {
            // Strings are stored in an array
            if (json_object_get_type(value) != json_type_array) {
                continue;
            }
            for (int i = 0; i < (int) json_object_array_length(value); i++) {
                bool            notConcerned = false;
                char           *name         = NULL;
                ScenarioPage_t *page         = &pages[nb_pages];

                struct json_object *page_info = json_object_array_get_idx(value, i);
                page->optional                = false;  // by default
                page->nb_sub_pages            = 1;      // by default
                page->cur_sub_page            = 0;      // by default
                json_object_object_foreach(page_info, k, v)
                {
                    if (!strcmp(k, "name")) {
                        name = (char *) json_object_get_string(v);
                        if ((error_code = add_name(page, name)) != 0) {
                            json_object_put(json_root);
                            free(buffer);
                            return -error_code;
                        }
                    }
                    else if (!strcmp(k, "product")) {  // to restrict a page to a specific product
                        name = (char *) json_object_get_string(v);
                        if (!strstr(name, productName)) {
                            notConcerned = true;
                            continue;
                        }
                    }
                    else if (!strcmp(k, "optional")) {
                        name           = (char *) json_object_get_string(v);
                        page->optional = (strcmp("true", name) == 0) ? true : false;
                    }
                    else if (!strcmp(k, "targets")) {
                        if ((error_code = add_targets(page, v)) != 0) {
                            json_object_put(json_root);
                            free(buffer);
                            return -error_code;
                        }
                    }
                    else {
                        fprintf(
                            stdout, "WARNING: for page #%d - Ignoring unknown Key \"%s\"!\n", i, k);
                    }
                }
                if (!notConcerned) {
                    nb_pages++;
                }
            }
        }
    }
    json_object_put(json_root);
    free(buffer);
    current_scenario_page = &pages[0];

    return 0;
}

static ScenarioPage_t *scenario_get_page_from_name(char *page_name)
{
    // Next line is a macro that iterate through keys/values of that JSON object
    for (uint32_t i = 0; i < nb_pages; i++) {
        if (!strcmp(pages[i].name, page_name)) {
            return &pages[i];
        }
    }
    printf("ERROR: scenario_get_page_from_name() impossible to find page %s\n", page_name);
    return NULL;
}

static bool get_next_page(const char *eventType, const char *eventName)
{
    ScenarioPage_t *next_page;
    PageStep_t     *cur_target;

    cur_target = &current_scenario_page->steps[current_scenario_page->currentStep];

    // print transition if to a different page
    if (verbose && strcmp(current_scenario_page->name, cur_target->name)) {
        printf("\t%s --> %s: %s[%s]\n",
               current_scenario_page->name,
               cur_target->name,
               eventType,
               eventName);
    }
    //  get the page corresponding to the current step in current page
    next_page = scenario_get_page_from_name(cur_target->name);
    // if this page is NULL, it means that name is malformed.
    if (next_page == NULL) {
        fprintf(stderr, "ERROR: Unknown page name %s\n", cur_target->name);
        current_scenario_page = NULL;
        return false;
    }
    if ((next_page != current_scenario_page) && (current_scenario_page->saved != true)) {
        if ((current_scenario_page->optional == false)
            && (strcmp(current_scenario_page->name, "Entry"))) {
            fprintf(stderr,
                    "ERROR: current page [%s] not saved before moving to a new one [%s]\n",
                    current_scenario_page->name,
                    cur_target->name);
            current_scenario_page = NULL;
            return false;
        }
        //
    }
    // if the reset param was set for the current step in current page, it means that we will
    // restart at step 0 in current_scenario_page
    if (cur_target->reset) {
        next_page->currentStep = 0;
        for (uint32_t i = 0; i < next_page->nbSteps; i++) {
            next_page->steps[i].wait = next_page->steps[i].wait_initial;
        }
    }

    // printf("get_next_page: batt_level %d%%, next_page = %s, pos =
    // [%d,%d]\n",current_scenario_page->steps[current_scenario_page->currentStep].batt_level,
    // current_scenario_page->name,cur_page->steps[cur_page->currentStep].x,cur_page->steps[cur_page->currentStep].y);
    //  update step in current page
    current_scenario_page->currentStep++;
    // update current page
    current_scenario_page               = next_page;
    current_scenario_page->cur_sub_page = 0;
    return true;
}

static int scenario_get_action(void)
{
    // if the last target of the current page has been consumed, stop the simulation
    if (current_scenario_page->currentStep >= current_scenario_page->nbSteps) {
        return 1;
    }
    if ((current_scenario_page->currentStep < current_scenario_page->nbSteps)
        && (current_scenario_page->nb_sub_pages > 1)
        && (current_scenario_page->cur_sub_page < current_scenario_page->nb_sub_pages)) {
        // if multi-page text, automatically press "right" to parse all pages
        return 3;
    }
    // if it's a wait step, simply decrease counter
    if (current_scenario_page->steps[current_scenario_page->currentStep].wait > 0) {
        // printf("scenario_get_action wait %d
        // ms\n",current_scenario_page->steps[current_scenario_page->currentStep].wait);
        current_scenario_page->steps[current_scenario_page->currentStep].wait -= 100;
    }
    // if it's a App event, get value
    else if (current_scenario_page->steps[current_scenario_page->currentStep].app_event_idx
             != 0xFF) {
        uint8_t app_event_idx
            = current_scenario_page->steps[current_scenario_page->currentStep].app_event_idx;
        uint32_t param = current_scenario_page->steps[current_scenario_page->currentStep].param;

        if (!get_next_page("APP", appEvents[app_event_idx].name)) {
            return -1;
        }
        // printf("scenario_get_action app_event %d\n",app_event_idx);
        switch (appEvents[app_event_idx].id) {
            case BTC_SIGN:
                app_bitcoinSignTransaction();
                break;
            case CARDANO_SIGN:
                app_cardanoSignTransaction();
                break;
            case STELLAR_SIGN:
                app_stellarSignTransaction();
                break;
            case STELLAR_STREAM_SIGN:
                app_stellarSignStreamedTransaction();
                break;
            case BTC_VERIFY_ADDR:
                app_bitcoinVerifyAddress();
                break;
            case CARDANO_VERIFY_ADDR:
                app_cardanoVerifyAddress();
                break;
            case BTC_OPEN:
                app_fullBitcoin();
                break;
            case ETH_OPEN:
                app_fullEthereum();
                break;
            case ETH_OPEN_2:
                app_fullEthereum2();
                break;
            case MONERO_OPEN:
                app_fullMonero();
                break;
            case MON_MESSAGE_SKIP:
                app_moneroSignForwardOnlyMessage();
                break;
            case ETH_MESSAGE_SKIP:
                app_ethereumSignForwardOnlyMessage();
                break;
            case ETH_SIGN:
                app_ethereumSignMessage();
                break;
            case ETH_BLIND:
                app_ethereumReviewBlindSigningTransaction();
                break;
            case ETH_ENS:
                app_ethereumReviewENSTransaction();
                break;
            case ETH_DAPP:
                app_ethereumReviewDappTransaction();
                break;
            case ETH_VERIFY_ADDR:
                app_ethereumVerifyAddress();
                break;
            case ETH_VERIFY_MULTISIG:
                app_ethereumVerifyMultiSig();
                break;
            case BTC_SHARE_ADDR:
                app_bitcoinShareAddress();
                break;
            case RECOV_OPEN:
                app_fullRecoveryCheck();
                break;
            case DOGE_SIGN:
                app_dogecoinSignTransaction(param & 1, param & 2, param & 4, param & 8, param & 16);
                break;
            case DOGE_PRELUDE_SIGN:
                app_dogecoinSignWithPrelude();
                break;
            default:
                // error
                return -1;
        }
    }
    else {
        // not consumed
        return 2;
    }
    // consumed
    return 0;
}

#ifdef HAVE_SE_TOUCH
int scenario_get_position(nbgl_touchStatePosition_t *touchStatePosition, bool previousState)
{
    // printf("scenario_get_position: %s, %d/%d, batt_level = %d%%\n",
    //       current_scenario_page->name,
    //       current_scenario_page->currentStep,
    //       current_scenario_page->nbSteps,
    //       current_scenario_page->steps[current_scenario_page->currentStep].batt_level);
    touchStatePosition->state = previousState;

    int ret = scenario_get_action();
    // if not consumed by non-touch action, it's a touch action
    if (ret == 2) {
        uint8_t id = current_scenario_page->steps[current_scenario_page->currentStep].objTouchId;
        if (id != 0) {
            if (id == CONTROLS_ID) {
                uint8_t index
                    = current_scenario_page->steps[current_scenario_page->currentStep].objTouchSubId
                      - '0';
                id += index;
            }
            nbgl_obj_t *obj = nbgl_touchGetObjectFromId(nbgl_screenGetTop(), id);
            if (obj == NULL) {
                printf("scenario_get_position: in %s, Impossible to find object with ID = %d\n",
                       current_scenario_page->name,
                       id);
                return -1;
            }
            // if obj is Keypad or Keyboard, use subObjectId to get the position of the key inside
            if (obj->type == KEYBOARD) {
                if (!nbgl_keyboardGetPosition(
                        (nbgl_keyboard_t *) obj,
                        current_scenario_page->steps[current_scenario_page->currentStep]
                            .objTouchSubId,
                        (uint16_t *) &current_scenario_page
                            ->steps[current_scenario_page->currentStep]
                            .x,
                        (uint16_t *) &current_scenario_page
                            ->steps[current_scenario_page->currentStep]
                            .y)) {
                    printf("scenario_get_position: in %s, Impossible to find key with ID = %d\n",
                           current_scenario_page->name,
                           id);
                    return -1;
                }
            }
            else if (obj->type == KEYPAD) {
                char key = current_scenario_page->steps[current_scenario_page->currentStep]
                               .objTouchSubId;
                if (key == 'b') {
                    key = BACKSPACE_KEY;
                }
                else if (key == 'v') {
                    key = VALIDATE_KEY;
                }
                if (!nbgl_keypadGetPosition((nbgl_keypad_t *) obj,
                                            key,
                                            (uint16_t *) &current_scenario_page
                                                ->steps[current_scenario_page->currentStep]
                                                .x,
                                            (uint16_t *) &current_scenario_page
                                                ->steps[current_scenario_page->currentStep]
                                                .y)) {
                    printf("scenario_get_position: in %s, Impossible to find key with ID = %d\n",
                           current_scenario_page->name,
                           id);
                    return -1;
                }
                // printf("scenario_get_position: %s, x = %d, y = %d\n",current_scenario_page->name,
                // touchStatePosition->x, touchStatePosition->y);
            }
            else {
                // center of the object
                current_scenario_page->steps[current_scenario_page->currentStep].x
                    = obj->area.x0 + obj->area.width / 2;
                current_scenario_page->steps[current_scenario_page->currentStep].y
                    = obj->area.y0 + obj->area.height / 2;
                // printf("scenario_get_position: %s, x = %d, y = %d, type =
                // %d\n",current_scenario_page->name, touchStatePosition->x, touchStatePosition->y,
                // obj->type);
            }
        }
        touchStatePosition->x = current_scenario_page->steps[current_scenario_page->currentStep].x;
        touchStatePosition->y = current_scenario_page->steps[current_scenario_page->currentStep].y;

        touchStatePosition->x += (FULL_SCREEN_WIDTH - SCREEN_WIDTH);
        if (current_scenario_page->steps[current_scenario_page->currentStep].objTouchId
            != LONG_PRESS_BUTTON_ID) {
            touchStatePosition->state = (previousState == PRESSED) ? RELEASED : PRESSED;
        }
        else {
            touchStatePosition->state = PRESSED;
            if (current_scenario_page->steps[current_scenario_page->currentStep].long_press_wait
                > 0) {
                current_scenario_page->steps[current_scenario_page->currentStep].long_press_wait
                    -= 100;
            }
        }

        if ((touchStatePosition->state == RELEASED)
            || ((current_scenario_page->steps[current_scenario_page->currentStep].objTouchId
                 == LONG_PRESS_BUTTON_ID)
                && (current_scenario_page->steps[current_scenario_page->currentStep].long_press_wait
                    == 0))) {
            if (!get_next_page(
                    "TOUCH",
                    getControlName(
                        current_scenario_page->steps[current_scenario_page->currentStep].objTouchId,
                        current_scenario_page->steps[current_scenario_page->currentStep]
                            .objTouchSubId))) {
                return -1;
            }
        }
        return 0;
    }
    else {
        return ret;
    }
}
#else   // HAVE_SE_TOUCH

int scenario_get_keys(uint8_t *state, uint8_t previousState)
{
    // printf("scenario_get_keys: %s, %d/%d, batt_level =
    // %d%%\n",current_scenario_page->name,current_scenario_page->currentStep,current_scenario_page->nbSteps,current_scenario_page->steps[current_scenario_page->currentStep].batt_level);
    *state = previousState;

    int ret = scenario_get_action();
    // if not consumed by non-key action, it's a key action
    if (ret == 2) {
        if (current_scenario_page->steps[current_scenario_page->currentStep].forced_key_press
            == 0) {
            *state
                = (previousState != 0)
                      ? 0
                      : current_scenario_page->steps[current_scenario_page->currentStep].keyState;
            if ((*state == 0) && (!get_next_page("KEY", ""))) {
                return -1;
            }
        }
        else if (current_scenario_page->steps[current_scenario_page->currentStep].forced_key_press
                 == 1) {  // release
            *state = 0;
            if (!get_next_page("KEY", "")) {
                return -1;
            }
        }
        else if (current_scenario_page->steps[current_scenario_page->currentStep].forced_key_press
                 == 2) {  // press
            *state = current_scenario_page->steps[current_scenario_page->currentStep].keyState;
            if (!get_next_page("KEY", "")) {
                return -1;
            }
        }

        return 0;
    }
    else if (ret == 3) {
        // navigation across the sub-pages of a string, for Nano
        *state = (previousState != 0) ? 0 : BUTTON_RIGHT;
        return 0;
    }
    else {
        return ret;
    }
}
#endif  // HAVE_SE_TOUCH

int scenario_save_screen(char *framebuffer, nbgl_area_t *area)
{
    if (current_scenario_page != NULL) {
        char fullPath[256];
        if (verbose) {
            fprintf(stdout,
                    "scenario_name=%s, current_scenario_page->name=%s, area->width=%d, "
                    "area->height=%d\n",
                    scenario_name,
                    current_scenario_page->name,
                    area->width,
                    area->height);
        }
        if (current_scenario_page->name) {
            if (current_scenario_page->cur_sub_page < current_scenario_page->nb_sub_pages) {
                current_scenario_page->cur_sub_page++;
            }
            snprintf(fullPath,
                     sizeof(fullPath),
#ifdef SCREEN_SIZE_WALLET
                     "%s/%s/%s.png",
#else   // SCREEN_SIZE_WALLET
                     "%s/%s/%s.%d.png",
#endif  // SCREEN_SIZE_WALLET
                     dirName,
                     scenario_name,
                     current_scenario_page->name
#ifdef SCREEN_SIZE_NANO
                     ,
                     current_scenario_page->cur_sub_page
#endif  // SCREEN_SIZE_NANO
            );
            if (framebuffer && verbose) {
                fprintf(stdout, "(saving %s)\n", fullPath);
            }
#ifndef NO_SCREENSHOTS_PNG
            save_png((char *) framebuffer, fullPath, FULL_SCREEN_WIDTH, SCREEN_HEIGHT);
#endif  // NO_SCREENSHOTS_PNG
            save_hash((char *) framebuffer,
                      FULL_SCREEN_WIDTH,
                      SCREEN_HEIGHT,
                      current_scenario_page->hash,
                      &current_scenario_page->hash_len);
            current_scenario_page->saved = true;
        }
        // special case when a screen is transient and automatically closed, without user action
        if ((current_scenario_page->currentStep < current_scenario_page->nbSteps)
            && (current_scenario_page->steps[current_scenario_page->currentStep].auto_transition)
            && (current_scenario_page->cur_sub_page == current_scenario_page->nb_sub_pages)) {
            ScenarioPage_t *cur_page = current_scenario_page;
            current_scenario_page    = scenario_get_page_from_name(
                current_scenario_page->steps[current_scenario_page->currentStep].name);
            if (verbose) {
                printf("\t%s --> %s: AUTOMATIC\n", cur_page->name, current_scenario_page->name);
            }
            cur_page->currentStep++;
        }
    }
    return 0;
}

char *scenario_get_current_page(void)
{
    return current_scenario_page->name;
}

void scenario_save_json(void)
{
    char     fullPath[256];
    uint32_t i, j;
    bool     first = true;

    // this is the end of the scenario so reset current_scenario_page to avoid
    // adding string_ids in this page
    current_scenario_page = NULL;
    snprintf(fullPath, sizeof(fullPath), "%s/%s_flow.json", dirName, scenario_name);
    FILE *fptr = fopen(fullPath, "w");
    if (!fptr) {
        fprintf(stderr, "Error creating file %s.\n", fullPath);
        return;
    }

    fprintf(fptr, "{\n");
    fprintf(fptr, "\t\"name\":\"%s\",\n", scenario_name);
    fprintf(fptr, "\t\"pages\":[\n");
    for (i = 0; i < nb_pages; i++) {
        for (uint32_t k = 1; k <= pages[i].nb_sub_pages; k++) {
            fprintf(fptr, "\t\t{\n");
#ifdef SCREEN_SIZE_WALLET
            fprintf(fptr, "\t\t\t\"name\": \"%s\",\n", pages[i].name);
            fprintf(fptr, "\t\t\t\"image\": \"%s/%s.png\",\n", scenario_name, pages[i].name);
#else   // SCREEN_SIZE_WALLET
            fprintf(fptr, "\t\t\t\"name\": \"%s.%d\",\n", pages[i].name, k);
            fprintf(fptr, "\t\t\t\"image\": \"%s/%s.%d.png\",\n", scenario_name, pages[i].name, k);
#endif  // SCREEN_SIZE_WALLET
            fprintf(fptr, "\t\t\t\"hash\": \"");
            for (j = 0; j < pages[i].hash_len; j++) {
                fprintf(fptr, "%02x", pages[i].hash[j]);
            }
            fprintf(fptr, "\",\n");
            fprintf(fptr, "\t\t\t\"transitions\": [\n");
            first = true;
            for (j = 0; j < pages[i].nbSteps; j++) {
                PageStep_t *step = &pages[i].steps[j];
#ifdef SCREEN_SIZE_WALLET
                if (step->objTouchId > 0) {
#else   // SCREEN_SIZE_WALLET
                if (step->keyState > 0) {
#endif  // SCREEN_SIZE_WALLET
                    if (first) {
                        first = false;
                    }
                    else {
                        fprintf(fptr, ",");
                    }
#ifdef SCREEN_SIZE_WALLET
                    fprintf(fptr, "\n\t\t\t\t{\"dest_page\":\"%s\",", step->name);
                    fprintf(fptr,
                            "\n\t\t\t\t \"objectId\":\"%s\",",
                            getControlName(step->objTouchId, step->objTouchSubId));
                    fprintf(fptr, "\n\t\t\t\t \"coords\":\"%d,%d\"}", step->x, step->y);
#else   // SCREEN_SIZE_WALLET
                    fprintf(fptr, "\n\t\t\t\t{\"dest_page\":\"%s\"}", step->name);
#endif  // SCREEN_SIZE_WALLET
                }
            }
            fprintf(fptr, "\n\t\t\t]\n");
            fprintf(fptr, "\t\t}");
            if (k < pages[i].nb_sub_pages) {
                fprintf(fptr, ",\n");
            }
            else {
                fprintf(fptr, "\n");
            }
        }

        if (i < (nb_pages - 1)) {
            fprintf(fptr, ",\n");
        }
        else {
            fprintf(fptr, "\n");
        }
    }
    fprintf(fptr, "\t]\n}\n");
    fclose(fptr);

    snprintf(fullPath, sizeof(fullPath), "%s/%s_strings.json", dirName, scenario_name);
    fptr = fopen(fullPath, "w");
    fprintf(fptr, "{\n");
    fprintf(fptr, "\t\"flow\":\"%s\",\n", scenario_name);
    fclose(fptr);
}
