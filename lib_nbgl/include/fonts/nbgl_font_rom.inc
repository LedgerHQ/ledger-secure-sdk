
/*******************************************************************************
*   Ledger Nano S - Secure firmware
*   (c) 2022 Ledger
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#ifdef SCREEN_SIZE_WALLET
#ifdef TARGET_STAX
#include "nbgl_font_inter_regular_24.inc"
#include "nbgl_font_inter_semibold_24.inc"
#include "nbgl_font_inter_medium_32.inc"
#include "nbgl_font_inter_regular_24_1bpp.inc"
#include "nbgl_font_inter_semibold_24_1bpp.inc"
#include "nbgl_font_inter_medium_32_1bpp.inc"
#else // TARGET_STAX
#include "nbgl_font_inter_regular_28.inc"
#include "nbgl_font_inter_semibold_28.inc"
#include "nbgl_font_inter_medium_36.inc"
#include "nbgl_font_inter_regular_28_1bpp.inc"
#include "nbgl_font_inter_semibold_28_1bpp.inc"
#include "nbgl_font_inter_medium_36_1bpp.inc"
#endif // TARGET_STAX
#else // SCREEN_SIZE_WALLET
#include "nbgl_font_open_sans_extrabold_11.inc"
#include "nbgl_font_open_sans_regular_11.inc"
#include "nbgl_font_open_sans_light_16.inc"
#endif // SCREEN_SIZE_WALLET
#if (defined(HAVE_BOLOS) && !defined(BOLOS_OS_UPGRADER_APP))
#if !defined(HAVE_LANGUAGE_PACK)
  #if (OS_LANGUAGE==LANG_ENGLISH)
    #include "bolos_pack_en_glyphes.inc"
  #elif (OS_LANGUAGE==LANG_FRENCH)
    #include "bolos_pack_fr_glyphes.inc"
  #elif (OS_LANGUAGE==LANG_SPANISH)
    #include "bolos_pack_es_glyphes.inc"
  #elif (OS_LANGUAGE==LANG_BRAZILIAN)
    #include "bolos_pack_br_glyphes.inc"
  #elif (OS_LANGUAGE==LANG_GERMAN)
    #include "bolos_pack_de_glyphes.inc"
  #elif (OS_LANGUAGE==LANG_RUSSIAN)
    #include "bolos_pack_ru_glyphes.inc"
  #elif (OS_LANGUAGE==LANG_TURKISH)
    #include "bolos_pack_tr_glyphes.inc"
  #else //OS_LANGUAGE
    #error No default language defined!
  #endif //OS_LANGUAGE
#endif //!defined(HAVE_LANGUAGE_PACK)
#endif //HAVE_BOLOS
