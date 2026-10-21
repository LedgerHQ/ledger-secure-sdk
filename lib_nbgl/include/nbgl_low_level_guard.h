/**
 * @file nbgl_low_level_guard.h
 * @brief Included by every low-level NBGL header, to forbid them in apollo app code
 *
 */

// No include guard: every forbidden header must trigger the error
#ifdef NBGL_USE_CASE_ONLY
#error "low-level NBGL header included: apollo apps may only use the nbgl_useCase* API"
#endif
