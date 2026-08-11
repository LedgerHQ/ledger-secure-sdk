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
    execute_process(
        COMMAND ${_load_app} --offline ${BIN_DIR}/app.apdu
        OUTPUT_VARIABLE _output
        COMMAND_ERROR_IS_FATAL ANY)

    if(NOT _output MATCHES "Application full hash : ([0-9a-fA-F]+)")
        message(FATAL_ERROR "No application hash in loadApp output:\n${_output}")
    endif()
    file(WRITE ${BIN_DIR}/app.sha256 "${CMAKE_MATCH_1}\n")
endif()
