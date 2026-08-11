#######################################################################
#                             ledger_app                              #
#######################################################################
get_filename_component(_LEDGER_APP_SDK_DIR ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)

# SDK options, read from their declarations, to reject unknown PERMISSIONS, FEATURES and DISABLE.
set(_LEDGER_SDK_OPTIONS "")
foreach(_file target_profile.cmake compile_options.cmake)
    file(STRINGS ${CMAKE_CURRENT_LIST_DIR}/sdk/${_file} _lines REGEX "^option\\([A-Z0-9_]+ ")
    list(TRANSFORM _lines REPLACE "^option\\(([A-Z0-9_]+) .*$" "\\1")
    list(APPEND _LEDGER_SDK_OPTIONS ${_lines})
endforeach()

# Macro, so that the SDK inputs and the coverage flags land in the app directory.
macro(ledger_app)
    set(_ledger_app_icons ICON_NANOX ICON_NANOS2 ICON_STAX ICON_FLEX ICON_APEX_P ICON_APEX_M)
    cmake_parse_arguments(_LEDGER_APP "" "NAME;COPYRIGHT;VARIANT_PARAM;GLYPHS_DIR;ICON_HOME_NANO;CUSTOM_APP_FLAGS;${_ledger_app_icons}"
                          "VARIANTS;PERMISSIONS;FEATURES;DISABLE;EXCLUDE_SOURCES;INCLUDE_DIRS;CURVES;PATHS" ${ARGN})
    foreach(_arg NAME COPYRIGHT VARIANT_PARAM VARIANTS)
        if(NOT _LEDGER_APP_${_arg})
            message(FATAL_ERROR "ledger_app: ${_arg} is required")
        endif()
    endforeach()

    # The first variant is the default one.
    list(GET _LEDGER_APP_VARIANTS 0 _variant)
    set(${_LEDGER_APP_VARIANT_PARAM} ${_variant} CACHE STRING "Application variant")
    set_property(CACHE ${_LEDGER_APP_VARIANT_PARAM} PROPERTY STRINGS ${_LEDGER_APP_VARIANTS})
    if(NOT "${${_LEDGER_APP_VARIANT_PARAM}}" IN_LIST _LEDGER_APP_VARIANTS)
        message(FATAL_ERROR "ledger_app: unsupported ${_LEDGER_APP_VARIANT_PARAM} '${${_LEDGER_APP_VARIANT_PARAM}}', "
                            "use one of: ${_LEDGER_APP_VARIANTS}")
    endif()

    # Read by the SDK, so set before adding it.
    set(LEDGER_APPNAME "${_LEDGER_APP_NAME}")
    set(LEDGER_APP_COPYRIGHT "${_LEDGER_APP_COPYRIGHT}")
    foreach(_icon IN LISTS _ledger_app_icons)
        if(_LEDGER_APP_${_icon})
            string(REPLACE ICON_ "" _target ${_icon})
            string(TOLOWER ${_target} _target)
            file(REAL_PATH ${_LEDGER_APP_${_icon}} LEDGER_APP_ICON_${_target} BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        endif()
    endforeach()
    if(_LEDGER_APP_GLYPHS_DIR)
        file(REAL_PATH ${_LEDGER_APP_GLYPHS_DIR} LEDGER_APP_GLYPHS_DIR BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endif()
    set(LEDGER_APP_ICON_HOME_NANO "${_LEDGER_APP_ICON_HOME_NANO}")
    foreach(_permission IN LISTS _LEDGER_APP_PERMISSIONS)
        if(NOT "HAVE_APPLICATION_FLAG_${_permission}" IN_LIST _LEDGER_SDK_OPTIONS)
            message(FATAL_ERROR "ledger_app: unknown PERMISSIONS '${_permission}', no HAVE_APPLICATION_FLAG_${_permission} SDK option")
        endif()
        set(HAVE_APPLICATION_FLAG_${_permission} ON)
    endforeach()
    if(_LEDGER_APP_CUSTOM_APP_FLAGS)
        set(CUSTOM_APP_FLAGS ${_LEDGER_APP_CUSTOM_APP_FLAGS})
    endif()
    foreach(_feature IN LISTS _LEDGER_APP_FEATURES)
        if(NOT "ENABLE_${_feature}" IN_LIST _LEDGER_SDK_OPTIONS)
            message(FATAL_ERROR "ledger_app: unknown FEATURES '${_feature}', no ENABLE_${_feature} SDK option")
        endif()
        set(ENABLE_${_feature} ON)
    endforeach()
    foreach(_feature IN LISTS _LEDGER_APP_DISABLE)
        if(NOT "DISABLE_${_feature}" IN_LIST _LEDGER_SDK_OPTIONS)
            message(FATAL_ERROR "ledger_app: unknown DISABLE '${_feature}', no DISABLE_${_feature} SDK option")
        endif()
        set(DISABLE_${_feature} ON)
    endforeach()

    add_subdirectory(${_LEDGER_APP_SDK_DIR} ${CMAKE_BINARY_DIR}/sdk)

    if(BUILD_UNIT_TESTS)
        ledger_unit_tests_init()
    endif()
    _ledger_app(
        VARIANT_PARAM   ${_LEDGER_APP_VARIANT_PARAM}
        VARIANTS        ${_LEDGER_APP_VARIANTS}
        EXCLUDE_SOURCES ${_LEDGER_APP_EXCLUDE_SOURCES}
        INCLUDE_DIRS    ${_LEDGER_APP_INCLUDE_DIRS}
        CURVES          ${_LEDGER_APP_CURVES}
        PATHS           ${_LEDGER_APP_PATHS})
endmacro()

function(_ledger_app)
    cmake_parse_arguments(ARG "" "VARIANT_PARAM" "VARIANTS;EXCLUDE_SOURCES;INCLUDE_DIRS;CURVES;PATHS" ${ARGN})

    file(GLOB_RECURSE _sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
    list(TRANSFORM ARG_EXCLUDE_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)
    list(REMOVE_ITEM _sources ${ARG_EXCLUDE_SOURCES})

    # Unit tests provide their own main.
    if(BUILD_UNIT_TESTS)
        list(FILTER _sources EXCLUDE REGEX "/(app_)?main\\.c$")
    endif()

    # Every src/ directory holding a header, as Makefile.rules does.
    file(GLOB_RECURSE _headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
    set(_include_dirs "")
    foreach(_header IN LISTS _headers)
        get_filename_component(_dir ${_header} DIRECTORY)
        list(APPEND _include_dirs ${_dir})
    endforeach()
    list(REMOVE_DUPLICATES _include_dirs)
    set(LEDGER_APP_INCLUDE_DIRS ${_include_dirs} PARENT_SCOPE)

    add_library(app STATIC ${_sources})
    target_include_directories(app PUBLIC ${_include_dirs} ${ARG_INCLUDE_DIRS})

    if(BUILD_UNIT_TESTS)
        target_link_libraries(app PRIVATE ledger::sdk-headers ledger::target-profile ledger::ut-host-shims)
    endif()

    if(NOT LEDGER_DEVICE_BUILD)
        return()
    endif()

    foreach(_arg CURVES PATHS)
        if(NOT ARG_${_arg})
            message(FATAL_ERROR "ledger_app: ${_arg} is required")
        endif()
    endforeach()

    target_link_libraries(app PUBLIC ledger::sdk)

    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    # Nano icons are reversed, as in Makefile.app_params.
    set(_icon_opts "")
    if(LEDGER_TARGET_IS_NANO)
        set(_icon_opts --reverse)
    endif()
    execute_process(
        COMMAND ${Python3_EXECUTABLE} ${LEDGER_SDK_ROOT}/lib_nbgl/tools/icon2glyph.py
                ${_icon_opts} --hexbitmap ${CMAKE_BINARY_DIR}/icon.hex ${LEDGER_APP_ICON_${TARGET}}
        COMMAND_ERROR_IS_FATAL ANY)
    file(READ ${CMAKE_BINARY_DIR}/icon.hex _icon_hex)
    string(STRIP "${_icon_hex}" _icon_hex)

    set(_install_params --appName ${LEDGER_APPNAME} --appVersion ${LEDGER_APP_VERSION} --icon ${_icon_hex})
    foreach(_curve IN LISTS ARG_CURVES)
        list(APPEND _install_params --curve ${_curve})
    endforeach()
    foreach(_path IN LISTS ARG_PATHS)
        list(APPEND _install_params --path ${_path})
    endforeach()
    execute_process(
        COMMAND ${Python3_EXECUTABLE} ${LEDGER_SDK_ROOT}/install_params.py ${_install_params}
        OUTPUT_VARIABLE _install_params_data OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
    target_compile_definitions(sdk_app_metadata PRIVATE
        APP_INSTALL_PARAMS_DATA=${_install_params_data}
        APP_FLAGS_APP_LOAD_PARAMS=${LEDGER_APP_FLAGS})

    # Same header the C code reads, so the id cannot drift from the target.
    file(STRINGS ${LEDGER_SDK_ROOT}/target/${TARGET}/include/bolos_target.h
         _target_id_line REGEX "define[ \t]+TARGET_ID")
    string(REGEX MATCH "0x[0-9a-fA-F]+" _target_id "${_target_id_line}")

    # ragger resolves the app binary as <repo>/build/<device>/bin/app.elf.
    set(_bin_dir ${CMAKE_BINARY_DIR}/bin)

    add_library(ledger_app_main OBJECT ${LEDGER_SDK_ROOT}/lib_standard_app/main.c)
    target_link_libraries(ledger_app_main PRIVATE ledger::sdk)

    # Glyphs first, as in the Makefile: lld keeps the first .ARM.attributes.
    add_executable(app.elf $<TARGET_OBJECTS:nbgl_glyphs> $<TARGET_OBJECTS:ledger_app_main>)
    set_target_properties(app.elf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${_bin_dir})
    target_link_libraries(app.elf PRIVATE app ledger::sdk)

    set(_load_app_args
        -DPYTHON=${Python3_EXECUTABLE}
        -DELF=$<TARGET_FILE:app.elf>
        -DBIN_DIR=${_bin_dir}
        -DTARGET_ID=${_target_id}
        -DAPI_LEVEL=${LEDGER_API_LEVEL}
        -DAPPNAME=${LEDGER_APPNAME}
        -DAPP_FLAGS=${LEDGER_APP_FLAGS})

    # Same post-processing as Makefile.rules_generic.
    add_custom_command(TARGET app.elf POST_BUILD
        COMMAND size -A $<TARGET_FILE:app.elf>
        COMMAND llvm-objcopy -O ihex -S $<TARGET_FILE:app.elf> ${_bin_dir}/app.hex
        COMMAND llvm-objdump -h -t $<TARGET_FILE:app.elf> > ${CMAKE_BINARY_DIR}/app.map
        COMMAND ${CMAKE_COMMAND} ${_load_app_args} -P ${LEDGER_SDK_ROOT}/cmake/app/apdu.cmake
        COMMENT "Generating app.hex, app.map, app.apdu and app.sha256"
        VERBATIM)

    # Same as `make load` and `make delete`.
    add_custom_target(load
        COMMAND ${CMAKE_COMMAND} ${_load_app_args} -DLOAD=ON -P ${LEDGER_SDK_ROOT}/cmake/app/apdu.cmake
        DEPENDS app.elf
        USES_TERMINAL
        VERBATIM)
    add_custom_target(delete
        COMMAND ${Python3_EXECUTABLE} -m ledgerblue.deleteApp --targetId ${_target_id} --appName ${LEDGER_APPNAME}
        USES_TERMINAL
        VERBATIM)

    # Same outputs as `make listvariants` and `make listparams`, read by the guidelines enforcer.
    list(JOIN ARG_VARIANTS " " _variants)
    add_custom_target(listvariants
        COMMAND ${CMAKE_COMMAND} -E echo "VARIANTS ${ARG_VARIANT_PARAM} ${_variants}"
        VERBATIM)

    # App glyphs relative to the app, SDK glyphs absolute, generated ones skipped, as in Makefile.glyphs.
    get_target_property(_glyphs nbgl_glyphs LEDGER_GLYPH_FILES)
    set(_glyph_files "")
    foreach(_glyph IN LISTS _glyphs)
        cmake_path(IS_PREFIX CMAKE_BINARY_DIR ${_glyph} _generated)
        if(_generated)
            continue()
        endif()
        cmake_path(IS_PREFIX CMAKE_CURRENT_SOURCE_DIR ${_glyph} _in_app)
        if(_in_app)
            file(RELATIVE_PATH _glyph ${CMAKE_CURRENT_SOURCE_DIR} ${_glyph})
        endif()
        list(APPEND _glyph_files ${_glyph})
    endforeach()
    list(JOIN _glyph_files " " _glyph_files)
    file(RELATIVE_PATH _icon ${CMAKE_CURRENT_SOURCE_DIR} ${LEDGER_APP_ICON_${TARGET}})
    list(JOIN ARG_CURVES " " _curves)
    list(JOIN ARG_PATHS " " _paths)

    add_custom_target(listparams
        COMMAND ${CMAKE_COMMAND} -E echo "Start dumping params"
        COMMAND ${CMAKE_COMMAND} -E echo "GLYPH_FILES=${_glyph_files}"
        COMMAND ${CMAKE_COMMAND} -E echo "ICONNAME=${_icon}"
        COMMAND ${CMAKE_COMMAND} -E echo "TARGET=${TARGET}"
        COMMAND ${CMAKE_COMMAND} -E echo "TARGET_NAME=${TARGET_NAME}"
        COMMAND ${CMAKE_COMMAND} -E echo "TARGET_ID=${_target_id}"
        COMMAND ${CMAKE_COMMAND} -E echo "APPNAME=${LEDGER_APPNAME}"
        COMMAND ${CMAKE_COMMAND} -E echo "APPVERSION=${LEDGER_APP_VERSION}"
        COMMAND ${CMAKE_COMMAND} -E echo "API_LEVEL=${LEDGER_API_LEVEL}"
        COMMAND ${CMAKE_COMMAND} -E echo "appFlags=${LEDGER_APP_FLAGS}"
        COMMAND ${CMAKE_COMMAND} -E echo "curve=${_curves}"
        COMMAND ${CMAKE_COMMAND} -E echo "path=${_paths}"
        COMMAND ${CMAKE_COMMAND} -E echo "Stop dumping params"
        VERBATIM)
endfunction()
