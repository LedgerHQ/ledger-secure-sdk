#######################################################################
#                       App manifest (ledger_app.toml)                #
#######################################################################
set(LEDGER_APP_MANIFEST ${CMAKE_SOURCE_DIR}/ledger_app.toml)

if(NOT EXISTS ${LEDGER_APP_MANIFEST})
    message(FATAL_ERROR "App manifest not found: ${LEDGER_APP_MANIFEST}")
endif()

set_property(DIRECTORY ${CMAKE_SOURCE_DIR} APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${LEDGER_APP_MANIFEST})

# Reads a string value; only headers and key lines, to keep list items balanced.
function(ledger_app_manifest_read section key out)
    file(STRINGS ${LEDGER_APP_MANIFEST} _lines REGEX "^(\\[.*\\]|${key} *=.*)$")
    set(_in_section FALSE)
    foreach(_line IN LISTS _lines)
        if(_line MATCHES "^\\[")
            string(COMPARE EQUAL "${_line}" "[${section}]" _in_section)
        elseif(_in_section AND _line MATCHES "^${key} *= *\"(.+)\"$")
            set(${out} "${CMAKE_MATCH_1}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
    message(FATAL_ERROR "Missing [${section}] ${key} in ${LEDGER_APP_MANIFEST}")
endfunction()
