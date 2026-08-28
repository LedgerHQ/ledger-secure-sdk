project(lib_cxng C)

set(CXNG_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_Groestl-ref.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_aead.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_aes.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_aes_gcm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_aes_siv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_blake2b.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_blake3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_blake3_ref.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_chacha.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_chacha_poly.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_cipher.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_cmac.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_crc16.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_crc32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_ecdh.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_ecdsa.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_ecfp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_ecschnorr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_eddsa.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_hash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_hkdf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_hmac.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_math.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_mldsa.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_mldsa_internal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_mldsa_lowram.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_mldsa_packing.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_mldsa_params.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_mldsa_poly.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_mldsa_polymat.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_mldsa_polyvec.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_mldsa_rounding.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_mldsa_sample.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_mldsa_smallpoly.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_mldsa_util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_mlkem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_mlkem_indcpa.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_mlkem_internal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_mlkem_params.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_mlkem_poly.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_mlkem_polymat.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_mlkem_polyvec.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_mlkem_sample.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_mlkem_util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_pbkdf2.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_pkcs1.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_poly1305.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_ram.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_ripemd160.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_rng.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_rng_rfc6979.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_rsa.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_selftests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_sha256.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_sha3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_sha512.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cx_utils.c
)

add_library(${PROJECT_NAME} STATIC ${CXNG_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(BUILD_UNIT_TESTS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${UT_INCLUDE_DIRS_SDK})
    target_compile_definitions(${PROJECT_NAME} PRIVATE ${UT_COMPILE_DEFS_SDK})
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ledger::sdk-headers ledger::target-profile ledger::compile-options)
