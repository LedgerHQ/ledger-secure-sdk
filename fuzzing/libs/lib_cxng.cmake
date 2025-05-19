include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/mock/mock.cmake)

file(
  GLOB
  LIB_CXNG_SOURCES
  "${BOLOS_SDK}/lib_cxng/src/cx_aead.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_aes_gcm.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_aes_siv.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_aes.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_blake2b.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_blake3_ref.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_blake3.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_chacha_poly.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_chacha.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_cipher.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_cmac.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_crc16.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_crc32.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_ecdh.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_ecdsa.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_ecfp.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_ecschnorr.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_eddsa.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_Groestl-ref.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_hash.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_hkdf.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_hmac.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_math.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_pbkdf2.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_pkcs1.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_poly1305.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_ram.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_ripemd160.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_rng_rfc6979.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_rng.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_rsa.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_selftests.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_sha3.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_sha256.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_sha512_alt_m0.s"
  "${BOLOS_SDK}/lib_cxng/src/cx_sha512.c"
  "${BOLOS_SDK}/lib_cxng/src/cx_utils.c")

add_library(cxng ${LIB_CXNG_SOURCES})
target_compile_options(cxng PUBLIC ${COMPILATION_FLAGS})
target_link_libraries(cxng PUBLIC macros mock)
target_include_directories(cxng PUBLIC ${BOLOS_SDK}/include/)
target_include_directories(cxng
                           PUBLIC ${BOLOS_SDK}/target/${TARGET_DEVICE}/include)
target_include_directories(cxng PUBLIC ${BOLOS_SDK}/lib_cxng/)
target_include_directories(cxng PUBLIC ${BOLOS_SDK}/lib_cxng/include)
target_include_directories(cxng PUBLIC ${BOLOS_SDK}/lib_cxng/src)
target_include_directories(cxng PUBLIC ${BOLOS_SDK}/)
