#######################################################################
#        ledgerblue.loadApp, run by ledger_app() and its load target  #
#######################################################################
# Without LOAD: writes app.apdu and app.sha256. With LOAD: installs on the device.
execute_process(COMMAND llvm-nm ${ELF} OUTPUT_VARIABLE _symbols COMMAND_ERROR_IS_FATAL ANY)

function(_symbol_address name out)
    if(NOT _symbols MATCHES "([0-9a-fA-F]+) [A-Za-z] ${name}\n")
        message(FATAL_ERROR "Symbol ${name} not found in ${ELF}")
    endif()
    set(${out} ${CMAKE_MATCH_1} PARENT_SCOPE)
endfunction()

_symbol_address(_nvram_data _nvram)
_symbol_address(_envram_data _envram)
_symbol_address(_install_parameters _install)
_symbol_address(_einstall_parameters _einstall)
math(EXPR _data_size "0x${_envram} - 0x${_nvram}")
math(EXPR _install_size "0x${_einstall} - 0x${_install}")

# Same parameters as APP_LOAD_PARAMS in Makefile.app_params.
set(_load_app ${PYTHON} -m ledgerblue.loadApp
    --targetId ${TARGET_ID}
    --targetVersion=
    --apiLevel ${API_LEVEL}
    --fileName ${BIN_DIR}/app.hex
    --appName ${APPNAME}
    --appFlags ${APP_FLAGS}
    --delete
    --tlv
    --dataSize ${_data_size}
    --installparamsSize ${_install_size})

if(LOAD)
    execute_process(COMMAND ${_load_app} COMMAND_ERROR_IS_FATAL ANY)
else()
    # Fail closed, as Makefile.rules_generic.
    file(REMOVE ${BIN_DIR}/app.apdu ${BIN_DIR}/app.apdu.tmp ${BIN_DIR}/app.sha256 ${BIN_DIR}/app.sha256.tmp)
    execute_process(
        COMMAND ${_load_app} --offline ${BIN_DIR}/app.apdu.tmp
        OUTPUT_VARIABLE _output
        COMMAND_ERROR_IS_FATAL ANY)

    string(REPEAT "[0-9A-Fa-f]" 64 _hex64)
    string(REGEX MATCHALL "(^|\n)Application full hash : [^\n]*" _hash_lines "${_output}")
    list(LENGTH _hash_lines _hash_count)
    if(NOT _hash_count EQUAL 1 OR NOT _hash_lines MATCHES "Application full hash : (${_hex64})$")
        file(REMOVE ${BIN_DIR}/app.apdu.tmp)
        message(FATAL_ERROR "loadApp did not report exactly one valid application hash:\n${_output}")
    endif()
    set(_hash ${CMAKE_MATCH_1})

    file(SIZE ${BIN_DIR}/app.apdu.tmp _apdu_size)
    if(_apdu_size EQUAL 0)
        file(REMOVE ${BIN_DIR}/app.apdu.tmp)
        message(FATAL_ERROR "loadApp produced an empty APDU file")
    endif()

    file(WRITE ${BIN_DIR}/app.sha256.tmp "${_hash}\n")
    file(RENAME ${BIN_DIR}/app.apdu.tmp ${BIN_DIR}/app.apdu)
    file(RENAME ${BIN_DIR}/app.sha256.tmp ${BIN_DIR}/app.sha256)
endif()
