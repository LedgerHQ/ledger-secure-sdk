#pragma once
/*
 * Compile-time layout consistency checks for Absolution-based fuzzers.
 *
 * Include after scenario_layout.h and the FUZZ_* mutator macros so the
 * mutator and harness see a validated layout.
 *
 * Required (defined by the app's scenario_layout.h):
 * - SCEN_CTRL_OFF, SCEN_CTRL_LEN
 * Required (defined by the app's fuzz_dispatcher.c):
 * - FUZZ_CTRL_OFF, FUZZ_CTRL_LEN
 *
 * Optional:
 * - SCEN_PREFIX_SIZE       (single-target apps; multi-target SDK builds
 *                           leave it undefined and resolve at runtime)
 * - SCEN_APP_DATA_OFF/LEN  (apps that expose an extra app-data window)
 * - FUZZ_APP_DATA_OFF/LEN  (mutator-side knob for that window)
 * - FUZZ_PREFIX_SIZE_FALLBACK (cross-checked against SCEN_PREFIX_SIZE
 *                              when both are defined)
 *
 * See docs/APP_CONTRACT.md § "mock/scenario_layout.h".
 */

#ifdef SCEN_PREFIX_SIZE

_Static_assert(SCEN_PREFIX_SIZE > 0, "SCEN_PREFIX_SIZE must be positive");

#if defined(SCEN_CTRL_OFF) && defined(SCEN_CTRL_LEN)
_Static_assert(SCEN_CTRL_LEN > 0, "SCEN_CTRL_LEN must be positive");
_Static_assert(SCEN_CTRL_OFF + SCEN_CTRL_LEN <= SCEN_PREFIX_SIZE,
               "Control header region exceeds prefix size");
#endif

#if defined(SCEN_APP_DATA_OFF) && defined(SCEN_APP_DATA_LEN)
#if SCEN_APP_DATA_LEN > 0
_Static_assert(SCEN_APP_DATA_OFF + SCEN_APP_DATA_LEN <= SCEN_PREFIX_SIZE,
               "App data region exceeds prefix size");
#endif
#endif

#if defined(FUZZ_PREFIX_SIZE_FALLBACK) && FUZZ_PREFIX_SIZE_FALLBACK != 0
_Static_assert(FUZZ_PREFIX_SIZE_FALLBACK == SCEN_PREFIX_SIZE,
               "FUZZ_PREFIX_SIZE_FALLBACK must match SCEN_PREFIX_SIZE");
#endif

#if defined(FUZZ_CTRL_OFF) && defined(FUZZ_CTRL_LEN)
_Static_assert(FUZZ_CTRL_OFF + FUZZ_CTRL_LEN <= SCEN_PREFIX_SIZE,
               "FUZZ_CTRL region exceeds prefix size");
#endif

#if defined(FUZZ_APP_DATA_OFF) && defined(FUZZ_APP_DATA_LEN)
#if FUZZ_APP_DATA_LEN > 0
_Static_assert(FUZZ_APP_DATA_OFF + FUZZ_APP_DATA_LEN <= SCEN_PREFIX_SIZE,
               "FUZZ_APP_DATA region exceeds prefix size");
#endif
#endif

#endif /* SCEN_PREFIX_SIZE */

#if defined(FUZZ_CTRL_LEN)
_Static_assert(FUZZ_CTRL_LEN > 0, "FUZZ_CTRL_LEN must be positive");
#endif
