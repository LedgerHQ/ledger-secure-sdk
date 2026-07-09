/**
 * @file fuzz_app_defaults.c
 * @brief Weak defaults for optional app harness callbacks.
 *
 * Linked into every target through the shared @c mock library. Apps that need
 * no per-iteration teardown can leave @c fuzz_app_cleanup() undefined; an app
 * that defines its own overrides this weak no-op.
 */

__attribute__((weak)) void fuzz_app_cleanup(void) {}
