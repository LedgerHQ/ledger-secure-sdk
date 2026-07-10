#!/usr/bin/env bash

# Reset
COLOR_OFF="\e[0m"

BOLD="1"
CUSTOM_FG="38;5"
CUSTOM_BG="48;5"

WHITE="15"
DARK_RED="52"
DARK_GREN="22"
DARK_YELLOW="94"
BRIGHT_RED="196"
BRIGHT_GREN="154"
BRIGHT_YELLOW="226"

BG_ERROR="\e[${CUSTOM_BG};${DARK_RED}m"
BG_SUCCESS="\e[${CUSTOM_BG};${DARK_GREN}m"
BG_WARNING="\e[${CUSTOM_BG};${DARK_YELLOW}m"
FG_ERROR="\e[${BOLD};${CUSTOM_FG};${BRIGHT_RED}m"
FG_SUCCESS="\e[${BOLD};${CUSTOM_FG};${BRIGHT_GREN}m"
FG_WARNING="\e[${BOLD};${CUSTOM_FG};${BRIGHT_YELLOW}m"
FG_TEXT="\e[${BOLD};${CUSTOM_FG};${WHITE}m"

_log_colored_line () {
    bg="$1"
    header="$2"
    header_fg="$3"
    text="$4"
    echo -e "${header_fg}${bg}${header}${COLOR_OFF}${FG_TEXT}${bg}${text}${COLOR_OFF}"
}

log_error () {
    _log_colored_line "$BG_ERROR" "Error: " "$FG_ERROR" "$1"
}
log_error_no_header () {
    _log_colored_line "$BG_ERROR" "" "$FG_ERROR" "$1"
}

log_warning () {
    _log_colored_line "$BG_WARNING" "Warning: " "$FG_WARNING" "$1"
}

log_success () {
    _log_colored_line "$BG_SUCCESS" "Success: " "$FG_SUCCESS" "$1"
}

log_info () {
    echo "$1"
}

#===============================================================================
#
#     GitHub Actions helper functions
#
#===============================================================================

# Check if running in GitHub Actions
is_github_actions() {
    [[ "${GITHUB_ACTIONS}" == "true" ]]
}

# Echo GitHub Actions group commands only if in CI
gh_group() {
    if is_github_actions; then
        echo "::group::$1"
    fi
}

gh_endgroup() {
    if is_github_actions; then
        echo "::endgroup::"
    fi
}
