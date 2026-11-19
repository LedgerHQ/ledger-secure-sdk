#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "unity.h"

#include "bagl_animate.h"

// Must match DELIMITER in bagl_animate.c (7 spaces + NUL)
#define DELIMITER_SIZE       8
// Must match NR_MAX_SPACE_PADDING in bagl_animate.c
#define NR_MAX_SPACE_PADDING 38
// Longest label accepted by circle animations: two label + delimiter patterns
#define CIRCLE_MAX_LEN       ((MAX_LABEL_SIZE / 2) - DELIMITER_SIZE - 1)
// Enough steps to go through several full animation cycles
#define NR_STEPS             2000

static const bagl_anim_t *current_anim;

// Every text read by the drawing stubs that targets the animation context must
// stay inside anim->label (the current char width is computed on 1 char even
// when anim->len is 0, hence the check against the label buffer size)
static void check_text_bounds(const void *text, size_t text_length)
{
    const char *start = (const char *) text;

    if ((current_anim == NULL) || (start < (const char *) current_anim)
        || (start >= (const char *) (current_anim + 1))) {
        return;
    }
    TEST_ASSERT_TRUE(start >= current_anim->label);
    TEST_ASSERT_TRUE(start + text_length <= current_anim->label + MAX_LABEL_SIZE);
}

/**
 * BAGL stubs
 */

unsigned short bagl_compute_line_width(unsigned short font_id,
                                       unsigned short width,
                                       const void    *text,
                                       unsigned char  text_length,
                                       unsigned char  text_encoding)
{
    const volatile char *p = text;
    unsigned short       w = 0;

    (void) font_id;
    (void) width;
    (void) text_encoding;
    check_text_bounds(text, text_length);
    for (unsigned int i = 0; i < text_length; i++) {
        w += (p[i] != '\0') ? 6 : 0;
    }
    return w;
}

int bagl_draw_string(unsigned short font_id,
                     unsigned int   color1,
                     unsigned int   color0,
                     int            x,
                     int            y,
                     unsigned int   width,
                     unsigned int   height,
                     const void    *text,
                     unsigned int   text_length,
                     unsigned char  text_encoding)
{
    const volatile char *p = text;

    (void) font_id;
    (void) color1;
    (void) color0;
    (void) x;
    (void) y;
    (void) width;
    (void) height;
    (void) text_encoding;
    check_text_bounds(text, text_length);
    for (unsigned int i = 0; i < text_length; i++) {
        (void) p[i];
    }
    return 0;
}

void bagl_draw_bg(unsigned int color)
{
    (void) color;
}

void bagl_hal_draw_rect(unsigned int color, int x, int y, unsigned int width, unsigned int height)
{
    (void) color;
    (void) x;
    (void) y;
    (void) width;
    (void) height;
}

void screen_update(void) {}

/**
 * Helpers
 */

// Label without NUL terminator, allocated with its exact size so that any
// over-read is caught by ASan
static unsigned char *alloc_label(size_t len)
{
    unsigned char *label = malloc((len > 0) ? len : 1);

    TEST_ASSERT_NOT_NULL(label);
    for (size_t i = 0; i < len; i++) {
        label[i] = 'A' + (i % 26);
    }
    return label;
}

// Animation context allocated with its exact size so that any overflow of
// anim->label beyond the structure is caught by ASan
static bagl_anim_t *alloc_anim(void)
{
    bagl_anim_t *anim = malloc(sizeof(*anim));

    TEST_ASSERT_NOT_NULL(anim);
    memset(anim, 0xA5, sizeof(*anim));
    current_anim = anim;
    return anim;
}

static void free_anim(bagl_anim_t *anim)
{
    current_anim = NULL;
    free(anim);
}

static void run_steps(bagl_anim_t *anim)
{
    for (int i = 0; i < NR_STEPS; i++) {
        bagl_animation_step(anim);
        TEST_ASSERT_TRUE(anim->len <= MAX_LABEL_SIZE);
        if (anim->len > 0) {
            TEST_ASSERT_TRUE(anim->cur_char_idx < anim->len);
        }
    }
}

static void check_rejected(anim_type_t type, const unsigned char *label, size_t len)
{
    bagl_anim_t *anim = alloc_anim();

    bagl_animation_init(anim, type, label, len);
    TEST_ASSERT_EQUAL(ANIMATION_TYPE_END, anim->type);
    TEST_ASSERT_EQUAL(0, anim->len);
    run_steps(anim);
    free_anim(anim);
}

/**
 * Tests
 */

void test_classic_valid_lengths(void)
{
    for (size_t len = 0; len <= MAX_LABEL_SIZE; len++) {
        unsigned char *label    = alloc_label(len);
        bagl_anim_t   *anim     = alloc_anim();
        size_t         nr_space = (MAX_LABEL_SIZE - len) / 2;

        if (nr_space > NR_MAX_SPACE_PADDING) {
            nr_space = NR_MAX_SPACE_PADDING;
        }

        bagl_animation_init(anim, ANIMATION_TYPE_CLASSIC, label, len);
        TEST_ASSERT_EQUAL(ANIMATION_TYPE_CLASSIC, anim->type);
        TEST_ASSERT_EQUAL(len + 2 * nr_space, anim->len);
        TEST_ASSERT_TRUE(anim->len <= MAX_LABEL_SIZE);
        for (size_t i = 0; i < nr_space; i++) {
            TEST_ASSERT_EQUAL_CHAR(' ', anim->label[i]);
            TEST_ASSERT_EQUAL_CHAR(' ', anim->label[nr_space + len + i]);
        }
        if (len > 0) {
            TEST_ASSERT_EQUAL_MEMORY(label, anim->label + nr_space, len);
        }
        run_steps(anim);

        free_anim(anim);
        free(label);
    }
}

void test_classic_too_long(void)
{
    const size_t   lens[] = {MAX_LABEL_SIZE + 1, 1000, SIZE_MAX};
    unsigned char *label  = alloc_label(MAX_LABEL_SIZE + 1);

    for (size_t i = 0; i < sizeof(lens) / sizeof(lens[0]); i++) {
        check_rejected(ANIMATION_TYPE_CLASSIC, label, lens[i]);
    }
    free(label);
}

void test_circle_valid_lengths(void)
{
    const anim_type_t types[] = {ANIMATION_TYPE_CIRCLE_RL, ANIMATION_TYPE_CIRCLE_LR};

    for (size_t t = 0; t < sizeof(types) / sizeof(types[0]); t++) {
        for (size_t len = 1; len <= CIRCLE_MAX_LEN; len++) {
            unsigned char *label = alloc_label(len);
            bagl_anim_t   *anim  = alloc_anim();
            size_t         nr_patterns;

            bagl_animation_init(anim, types[t], label, len);
            TEST_ASSERT_EQUAL(types[t], anim->type);
            TEST_ASSERT_TRUE(anim->len <= MAX_LABEL_SIZE);
            TEST_ASSERT_TRUE(anim->circle.char_idx_limit < anim->len);

            // [label][delimiter]...[label]
            nr_patterns = (anim->len + DELIMITER_SIZE) / (len + DELIMITER_SIZE);
            TEST_ASSERT_TRUE(nr_patterns >= 2);
            TEST_ASSERT_EQUAL(nr_patterns * (len + DELIMITER_SIZE) - DELIMITER_SIZE, anim->len);
            for (size_t p = 0; p < nr_patterns; p++) {
                TEST_ASSERT_EQUAL_MEMORY(label, anim->label + p * (len + DELIMITER_SIZE), len);
            }
            run_steps(anim);

            free_anim(anim);
            free(label);
        }
    }
}

void test_circle_invalid_lengths(void)
{
    const anim_type_t types[] = {ANIMATION_TYPE_CIRCLE_RL, ANIMATION_TYPE_CIRCLE_LR};
    const size_t      lens[]  = {0,
                                 CIRCLE_MAX_LEN + 1,
                                 MAX_LABEL_SIZE - DELIMITER_SIZE - 1,
                                 MAX_LABEL_SIZE - DELIMITER_SIZE,
                                 MAX_LABEL_SIZE,
                                 MAX_LABEL_SIZE + 1,
                                 SIZE_MAX};
    unsigned char    *label   = alloc_label(MAX_LABEL_SIZE + 1);

    for (size_t t = 0; t < sizeof(types) / sizeof(types[0]); t++) {
        for (size_t i = 0; i < sizeof(lens) / sizeof(lens[0]); i++) {
            check_rejected(types[t], label, lens[i]);
        }
    }
    free(label);
}

void test_invalid_arguments(void)
{
    const unsigned char label[] = {'a', 'b', 'c'};

    // NULL context must not crash
    bagl_animation_init(NULL, ANIMATION_TYPE_CLASSIC, label, sizeof(label));

    // NULL label
    check_rejected(ANIMATION_TYPE_CLASSIC, NULL, 0);
    check_rejected(ANIMATION_TYPE_CLASSIC, NULL, sizeof(label));
    check_rejected(ANIMATION_TYPE_CIRCLE_RL, NULL, sizeof(label));
    check_rejected(ANIMATION_TYPE_CIRCLE_LR, NULL, sizeof(label));

    // Disabled or unknown animation type
    check_rejected(ANIMATION_TYPE_END, label, sizeof(label));
    check_rejected((anim_type_t) (ANIMATION_TYPE_END + 1), label, sizeof(label));
}

void setUp(void) {}
void tearDown(void) {}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_classic_valid_lengths);
    RUN_TEST(test_classic_too_long);
    RUN_TEST(test_circle_valid_lengths);
    RUN_TEST(test_circle_invalid_lengths);
    RUN_TEST(test_invalid_arguments);
    return UNITY_END();
}
