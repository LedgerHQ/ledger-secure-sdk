#!/bin/bash

set -e

# Categories are listed twice: first to demote fatal errors to warnings,
# second to suppress the warning message (lcov convention).
IGNORE_ERRORS=(--ignore-errors "negative,negative,gcov,gcov,inconsistent,inconsistent,corrupt,corrupt,unused,unused")

build_dir="$(realpath .)"

if [ -d CMakeFiles/app.dir ]; then
    # for app tests
    BASE_DIR=CMakeFiles/app.dir
else
    # for sdk tests
    BASE_DIR=.
fi

# --rc geninfo_unexecuted_blocks=1: gcov 14 reports unexecuted blocks on
# non-branch lines that still carry a non-zero hit count; this rc resets
# them to zero at the source rather than masking the inconsistency downstream.
lcov --branch-coverage --rc geninfo_unexecuted_blocks=1 "${IGNORE_ERRORS[@]}" \
    --capture --initial --directory "${BASE_DIR}" -b "${build_dir}" -o coverage.base
lcov --branch-coverage --rc geninfo_unexecuted_blocks=1 "${IGNORE_ERRORS[@]}" \
    --capture --directory . -b "${build_dir}" -o coverage.capture
lcov --branch-coverage "${IGNORE_ERRORS[@]}" \
    --add-tracefile coverage.base --add-tracefile coverage.capture \
    -b "${build_dir}" -o coverage.info

if [ -n "$1" ] && [ -d "$1" ]; then
    # for app tests: filter to only app source files
    lcov --branch-coverage "${IGNORE_ERRORS[@]}" \
        --extract coverage.info "$(realpath "$1")/*" -o coverage.info
else
    # for sdk tests (or when the filter path does not exist)
    lcov --branch-coverage "${IGNORE_ERRORS[@]}" \
        --remove coverage.info '*/unit-tests/*' '*/_deps/*' -o coverage.info
fi

echo "Generated 'coverage.info'."
genhtml --branch-coverage "${IGNORE_ERRORS[@]}" coverage.info -o coverage

# generate cobertura report (coverage.xml) for CI/CD pipelines
cobertura_base="${GITHUB_WORKSPACE:-$(realpath .)}"
if command -v lcov_cobertura >/dev/null; then
    lcov_cobertura coverage.info -b "${cobertura_base}" -o coverage.xml \
        || echo "lcov_cobertura failed; coverage.xml not generated"
else
    echo "lcov_cobertura not installed; coverage.xml not generated"
fi

rm -f coverage.base coverage.capture
