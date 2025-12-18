# Fuzzing

## Manual usage based on Ledger container

### About Fuzzing Framework
The code is divided into the following folders:

```bash
├── fuzzing
│   ├── build
│   │   ├── ...
│   │   └── generated_glyphs   #  generated glyphs
│   ├── extra                  #  .cmake files for building SDK's function harness
│   ├── harness                #  libFuzzer .c files for harness
│   │   └── fuzz_{}/           #  Optional folders for corpus of each harness [with the same name as the harness]
│   ├── libs                   #  .cmake files for building SDK libraries
│   ├── macros
│   │   ├── Makefile           #  Makefile used to expose the macros used when fuzzing the SDK
│   │   └── macros.cmake       #  creates an INTERFACE for using macros in cmake targets
│   │   └── add_macros.txt     #  macro list to add in SDK fuzzer compilation process
│   │   └── exclude_macros.txt #  macro list to exclude from the SDK fuzzer compilation process
│   ├── mock
│   │   ├── custom             #  Custom mock implementations for specific use cases (folder name must appear before 'generated' to override __weak__ functions)
│   │   ├── generated          #  automatically generated mock functions from src/syscalls.c
│   │   └── mock.cmake         #  .cmake file for building mock functions
│   ├── out                    #  Fuzzing output files
│   ├── CMakeLists.txt         #  .cmake file that builds SDK Fuzzers and exposes an INTERFACE for SDK libs for fuzzing APPs
│   ├── local_run.sh           #  Script for building and running fuzzers.
└────── README.md

```

### Preparation

The fuzzer can run from the docker `ledger-app-dev-tools`. You can download it from the `ghcr.io` docker repository:

```console
sudo docker pull ghcr.io/ledgerhq/ledger-app-builder/ledger-app-dev-tools
```

You can then enter this development environment by executing the following command from the repository root directory:

```console
docker run --rm -ti -v "$(realpath .):/app" ghcr.io/ledgerhq/ledger-app-builder/ledger-app-dev-tools
```

```console
export BOLOS_SDK=/app

cd fuzzing # You must run it from the fuzzing folder

./local_run.sh --build=1 --fuzzer=build/fuzz_bip32 --j=4 --run-fuzzer=1 --compute-coverage=1 --BOLOS_SDK=${BOLOS_SDK}
```

### About local_run.sh

| Parameter              | Type                | Description                                                          |
| :--------------------- | :------------------ | :------------------------------------------------------------------- |
| `--BOLOS_SDK`          | `PATH TO BOLOS SDK` | **Required**. Path to the BOLOS SDK                                  |
| `--build`              | `bool`              | **Optional**. Whether to build the project (default: 0)              |
| `--fuzzer`             | `PATH`              | **Required**. Path to the fuzzer binary                              |
| `--compute-coverage`   | `bool`              | **Optional**. Whether to compute coverage after fuzzing (default: 0) |
| `--run-fuzzer`         | `bool`              | **Optional**. Whether to run or not the fuzzer (default: 0)          |
| `--run-crash`          | `FILENAME`          | **Optional**. Run the fuzzer on a specific crash input file (default: 0) |
| `--sanitizer`          | `address or memory` | **Optional**. Compile fuzzer with sanitizer (default: address)       |
| `--j`                  | `int`               | **Optional**. Number of parallel jobs/CPUs for build and fuzzing (default: 1) |
| `--help`               |                     | **Optional**. Display help message                                   |


### Writing your Harness

When writing your harness, keep the following points in mind:

- An SDK's interface for compilation is provided via the target `secure_sdk` in CMakeLists.txt
- If you are running it for the first time, consider using the script `local_run` from inside the
  Docker container using the flag build=1, if you need to manually
  add/remove macros you can then do it using the files macros/add_macros.txt or
  macros/exclude_macros.txt and rerunning it, or directly change the generated macros/generated/macros.txt.
- A typical harness looks like this:

  ```console

  int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (sigsetjmp(fuzz_exit_jump_ctx.jmp_buf, 1)) return 0;

    ### harness code ###

    return 0;
  }

  ```

  This allows a return point when the `os_sched_exit()` function is mocked.

- To provide an SDK interface, we automatically generate syscall mock functions located in
  `SECURE_SDK_PATH/fuzzing/mock/generated/generated_syscalls.c`, if you need a more specific mock,
  you can define it in `APP_PATH/fuzzing/mock` with the same name and without the WEAK attribute.

### Adding an initial Corpus

```bash
├── fuzzing
│   ├── harness              # libFuzzer .c files for harness
│   │   └── fuzz_{}/         # Optional folders for corpus of each harness [with the same name as the harness]
```

To add an initial corpus for a specific harness, create a folder with the same name of the harness
inside `fuzzing/harness` with the binary input files.

The `local_run.sh` script will move them to the corpus before the fuzzing. If committed those folders will also
be used by ClusterFuzz in CI.

### Manual compilation

Once in the container, go into the `fuzzing` folder to compile the fuzzer:

```console
# Install missing dependencies
apt update && apt install -y libclang-rt-dev

# cmake initialization
cmake -S . -B build -DCMAKE_C_COMPILER=clang -DSANITIZER=address -G Ninja

# Fuzzer compilation
cmake --build build
```

One can still use his own modified `ledgere-secure-sdk`. If it doesn't contain a .target, you can pass it in the compilation
parameters:

```bash
cmake -S . -B build -DCMAKE_C_COMPILER=clang -DSANITIZER=address -G Ninja -DTARGET=stax
```

### Run

```bash
./build/fuzz_apdu_parser
./build/fuzz_base58
./build/fuzz_bip32
./build/fuzz_qrcodegen
./build/fuzz_alloc
./build/fuzz_alloc_utils
./build/fuzz_nfc_ndef
```
