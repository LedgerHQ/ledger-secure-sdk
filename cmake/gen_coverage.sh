#!/bin/bash

set -x
set -e

IGNORE_ERRORS="--ignore-errors inconsistent,corrupt"

if [ -d CMakeFiles/app.dir ]; then
    # for app tests
    BASE_DIR=CMakeFiles/app.dir
else
    # for sdk tests
    BASE_DIR=.
fi

lcov --rc branch_coverage=1 ${IGNORE_ERRORS} --capture --initial --directory "${BASE_DIR}" -o coverage.base
lcov --rc branch_coverage=1 ${IGNORE_ERRORS} --capture --directory . -o coverage.capture
lcov --rc branch_coverage=1 ${IGNORE_ERRORS} --add-tracefile coverage.base --add-tracefile coverage.capture -o coverage.info

if [ -n "$1" ]; then
    # for app tests
    lcov --rc branch_coverage=1 ${IGNORE_ERRORS} --extract coverage.info "$(realpath "$1")/*" -o coverage.info
else
    # for sdk tests
    lcov --rc branch_coverage=1 ${IGNORE_ERRORS} --remove coverage.info '*/unit-tests/*' '*/_deps/*' -o coverage.info
fi

echo "Generated 'coverage.info'."
genhtml --rc branch_coverage=1 ${IGNORE_ERRORS} --branch-coverage --flat coverage.info -o coverage

# generate cobertura report (coverage.xml) for CI/CD pipelines
cobertura_base="${BASE_DIR}"
if command -v lcov_cobertura >/dev/null; then
    lcov_cobertura coverage.info -b "${cobertura_base}" -o coverage.xml \
        || echo "lcov_cobertura failed; coverage.xml not generated"
else
    echo "lcov_cobertura not installed; coverage.xml not generated"
fi

rm -f coverage.base coverage.capture
