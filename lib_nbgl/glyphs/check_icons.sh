#!/usr/bin/env bash

set -e

# shellcheck source=/dev/null
source "$(dirname "$0")/logger.sh"


check_geometry() (
    file="$1"
    device="$2"

    # Icons must have a single exact geometry per device, while glyphs are
    # allowed to match any value of a per-device set.
    case "${device}" in
        nanox)
            geometries="14x14 16x16"
            ;;
        apex)
            geometries="24x24 32x32 48x48"
            ;;
        flex)
            geometries="40x40 64x64"
            ;;
        stax)
            geometries="32x32 64x64"
            ;;
        *)
            log_error "Device '${device}' not recognized"
            return 1
            ;;
    esac

    # Check if the file matches at least one of the expected geometries
    actual_geometry=$(identify -format "%wx%h" "${file}")

    for geometry in ${geometries}; do
        if [[ "${actual_geometry}" == "${geometry}" ]]; then
            log_success "Glyph '${file}' has a correct '${geometry}' geometry"
            return 0
        fi
    done

    # If we get here, no geometry matched
    log_error "Glyph '${file}' should have one of these geometries: ${geometries}, but has '${actual_geometry}'"
    return 1
)

check_glyph() (
    error=0
    file="$1"
    device="$2"

    log_info "Checking glyph file '${file}'"

    extension=$(basename "${file}" | cut -d'.' -f2)
    if [[ "${extension}" != "gif" && "${extension}" != "bmp" && "${extension}" != "png" ]]; then
        log_error "Glyph extension should be '.gif', '.bmp', or '.png', not '.${extension}'";
        return 1
    fi

    content=$(identify -verbose "${file}")

    if echo "${content}" | grep -q "Alpha"; then
        log_error "Glyph should have no alpha channel"
        error=1
    fi

    # Determine whether the device's screen only supports monochrome.
    # Nano and Apex screens are monochrome only, while Stax and Flex also support grayscale.
    # The device name itself is validated by check_geometry.
    case "${device}" in
        nanox | apex)
            monochrome=1
            ;;
        *)
            monochrome=0
            ;;
    esac

    # Monochrome picture
    if echo "${content}" | grep -q "Type: Bilevel"; then
        log_info "Monochrome image type"
        if ! echo "${content}" | grep -q "Colors: 2"; then
            log_error "Glyph should have only 2 colors"
            error=1
        fi

        # get the color lines
        color_lines=$(echo "${content}" | grep -A3 "Colors: " | tail -n -2)

        if ! echo "${color_lines}" | grep -q " #000000 "; then
            log_error "Glyph should have the black color defined"
            error=1
        fi

        if ! echo "${color_lines}" | grep -q " #FFFFFF "; then
            log_error "Glyph should have the white color defined"
            error=1
        fi

        # Be somewhat tolerant to different possible wordings for depth "1 bit" "1-bit" "8/1 bit" etc
        if ! echo "${content}" | grep -q "Depth: \(8/\)\?1.bit"; then
            log_error "Glyph should have 1 bit depth"
            error=1
        fi

    # Grayscale picture
    elif echo "${content}" | grep -q "Type: Grayscale"; then
        log_info "Grayscale image type"

        # If device only has monochrome screens: grayscale is not allowed
        if [[ "${monochrome}" -eq 1 ]]; then
            log_error "Glyph '${file}' must be monochrome, not grayscale"
            error=1
        fi

        # Use rev + cut f1 trick to grab last word ie the value of field 'Colors'
        colors_nb=$(echo "${content}" | grep "Colors: " | rev | cut -d' ' -f1 | rev)
        if [[ "${colors_nb}" -gt 16 ]]; then
            log_error "4bpp glyphs can't have more than 16 colors, ${colors_nb} found"
            error=1
        fi

        # Be somewhat tolerant to different possible wordings for depth "8 bit" "8-bit" "8/8 bit" etc
        if ! echo "${content}" | grep -q "Depth: \(8/\)\?8.bit"; then
            log_error "Glyph should have 8 bits depth"
            error=1
        fi

    else
        log_error "Glyph should be Monochrome or Grayscale"
        error=1
    fi

    check_geometry "${file}" "${device}" || error=1

    if [[ "${error}" -eq 0 ]]; then
        log_success "Glyph '${file}' is compliant"
    else
        log_error_no_header "To check the glyph content, run \"identify -verbose '${file}'\""
    fi

    return "${error}"
)

main() (
    glyph_errors=0
    glyphs_count=0
    dir="$1"
    device="$2"

    log_info "Checking glyphs for '${device}' in directory '${dir}'"
    log_info "----------------------------------------"
    all_files=$(find "${dir}" -type f -name "*.gif" -o -name "*.bmp" -o -name "*.png")
    # Récupère les fichiers nouvellement ajoutés par rapport à master
    new_files=$(git diff --name-only --diff-filter=A "$(git merge-base HEAD master)..HEAD")

    for file in ${all_files}; do
        # Ignore les fichiers qui existaient déjà
        if ! echo "${new_files}" | grep -qF "${file}"; then
            continue
        fi
        glyphs_count=$((glyphs_count + 1))
        log_info "Checking glyph '${file}'"
        check_glyph "${file}" "${device}" || glyph_errors=$((glyph_errors + 1))
    done
    log_info "----------------------------------------"

    if [[ "${glyph_errors}" -ne 0 ]]; then
        log_error_no_header "${glyph_errors} error(s) found:"
        [[ "${glyph_errors}" -ne 0 ]] && log_error_no_header "  -> ${glyph_errors} in glyphs (out of ${glyphs_count} checked)"
        return 1
    fi

    log_success "All graphical elements are compliant: ${glyphs_count} glyph(s) checked"
    return 0
)

main "$@"
