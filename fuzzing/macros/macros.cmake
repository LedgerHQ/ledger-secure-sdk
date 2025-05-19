
include_guard()

add_library(macros INTERFACE)

# Building from App
if (NOT "${BOLOS_SDK}/fuzzing" STREQUAL ${CMAKE_SOURCE_DIR})
    message("Importing macros from ${CMAKE_SOURCE_DIR}/generated_macros/generated/macros.txt")
    file(STRINGS "${CMAKE_SOURCE_DIR}/macros/generated/macros.txt" MACRO_LIST)
    target_compile_definitions(macros INTERFACE ${MACRO_LIST})

# Building from SDK
else()
    if("${TARGET_DEVICE}" STREQUAL "stax")
        message("Importing macros from ${CMAKE_SOURCE_DIR}/macros/macros-stax.txt")
        file(STRINGS "${CMAKE_SOURCE_DIR}/macros/macros-stax.txt" MACRO_LIST)
        target_compile_definitions(macros INTERFACE ${MACRO_LIST})
    
    elseif("${TARGET_DEVICE}" STREQUAL "flex")
        message("Importing macros from ${CMAKE_SOURCE_DIR}/macros/macros-flex.txt")
        file(STRINGS "${CMAKE_SOURCE_DIR}/macros/macros-flex.txt" MACRO_LIST)
        target_compile_definitions(macros INTERFACE ${MACRO_LIST})
    endif()

endif()
