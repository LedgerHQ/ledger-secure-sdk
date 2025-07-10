# Introduction

To build Ledger OS application 3 libraries are used:

| Name                    | Description                                        | Comes from                               | Version                                      | CPU (Arch)                                          |
| ----------------------- | -------------------------------------------------- | ---------------------------------------- | -------------------------------------------- | --------------------------------------------------- |
| libclang_rt.builtins    | Low-level runtime library                          | Prebuilt in the SDK (2)                  | (2)                                          | cortex-m3 (Armv7-M) and cortex-m35p+nodsp (Armv8-M) |
| (or libgcc (1))         | the same as above                                  | Prebuilt in the SDK                      | Unknown                                      | ARM, EABI5 version 1 (SYSV), not stripped           |
| libc                    | C standard library                                 | picolibc-arm-none-eabi Debian 12 package | 1.8-1 in current app builder docker image    | thumb/v7-m/nofp and thumb/v8-m.main/nofp            |
| libm                    | Standard C library of basic mathematical functions | the same as above                        | the same as above                            | the same as above                                   |

* (1) obsolete, replaced by libclang_rt.builtins, used only for Nano S, can be removed soon
* (2) See libclang_rt.builtins build below


# libclang_rt.builtins build

It is built using `.github/workflows/build_clangrt_builtins.yml` and `tools/build_clangrt_builtins.sh` and pushed to the SDK using an explicit PR.
Several symbols that conflict with the ones from picolibc are removed from the library just after build.

The parameters that have been used for the latest library build:
* on the base of ``llvm-toolchain-14-14.0.6/compiler-rt`` package (see also https://github.com/llvm/llvm-project/tree/main/compiler-rt)
* the explicit PR https://github.com/LedgerHQ/ledger-secure-sdk/pull/1035
