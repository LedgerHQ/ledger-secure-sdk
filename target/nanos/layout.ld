/*******************************************************************************
*   Ledger Blue - Secure firmware
*   (c) 2016, 2017, 2018, 2019, 2020 Ledger
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

MEMORY
{
  FLASH   (rx)  : ORIGIN = 0xc0d00000, LENGTH = 400K
  SRAM    (rwx) : ORIGIN = 0x20000200, LENGTH = 4K+512
  CXSRAM  (rwx) : ORIGIN = 0x20001400, LENGTH = 1K
}

PAGE_SIZE  = 64;
STACK_SIZE = DEFINED(stack_size) ? stack_size : 1024;

SECTIONS {
  .cx_ram : {
    *(.bss.G_cx)
  } > CXSRAM = 0x00
}
