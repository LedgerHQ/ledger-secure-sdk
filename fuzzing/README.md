# Fuzzing

## Manual usage based on Ledger container

### Preparation

The fuzzer can run from the docker `ledger-app-dev-tools`. You can download it from the `ghcr.io` docker repository:

```console
sudo docker pull ghcr.io/ledgerhq/ledger-app-builder/ledger-app-dev-tools:latest
```

You can then enter this development environment by executing the following command from the repository root directory:

```console
sudo docker run --rm -ti -v "$(realpath .):/app" ghcr.io/ledgerhq/ledger-app-builder/ledger-app-dev-tools:latest
```

### About Fuzzing Framework
The code is divided into the following folders:

```bash
├── fuzzing
│   ├── build
│   │   ├── ...
│   │   └── src_gen         # generated glyphs
│   ├── extra               # .cmake files for building function harness
│   ├── harness             # libFuzzer .c files for harness
│   ├── libs                # .cmake files for building SDK libraries
│   ├── macros
│   │   ├── macros-flex.txt # for flex targets
│   │   ├── macros-stax.txt # for stax targets
│   │   └── macros.cmake    # creates an INTERFACE for using macros in cmake targets
│   ├── mock
│   │   ├── custom          # Custom mock implementations for specific use cases (folder name must appear before 'generated' to override __weak__ functions)
│   │   ├── generated       # automatically generated mock functions from src/syscalls.c
│   │   └── mock.cmake      # .cmake file for building mock functions
│   ├── out                 # Fuzzing output files
│   ├── CMakeLists.txt      # .cmake file that builds SDK Fuzzers and exposes an INTERFACE for SDK libs for fuzzing APPs
│   ├── local_run.sh        # Script for building and running fuzzers.
└────── README.md

```

### About local_run.sh

```
./local_run.sh --build=1
./local_run.sh --fuzzer=[PATH_TO_FUZZER] --compute-coverage=1
```

| Parameter | Type     | Description                |
| :-------- | :------- | :------------------------- |
| `--TARGET_DEVICE` | `flex or stax` | **Optional**. Whether it is a flex or stax device (default: flex) |
| `--build` | `bool` | **Optional**. Whether to build the project (default: 0) |
| `--fuzzer` | `PATH` | **Required**. Path to the fuzzer binary |
| `--compute-coverage` | `bool` | **Optional**. Whether to compute coverage after fuzzing (default: 0) |
| `--run-fuzzer` | `bool` | **Optional**. Whether to run or not the fuzzer (default: 1) |
| `--help` |  | **Optional**. Dsplay help message |


### Manual compilation

Once in the container, go into the `fuzzing` folder to compile the fuzzer:

```console
# cmake initialization
cmake -S . -B build -DCMAKE_C_COMPILER=clang -DSANITIZER=address -DTARGET_DEVICE="$TARGET_DEVICE" -G Ninja

# Fuzzer compilation
cmake --build build
```

### Run

```console
./build/fuzz_apdu_parser
./build/fuzz_base58
./build/fuzz_bip32
./build/fuzz_qrcodegen
./build/fuzz_alloc
./build/fuzz_nfc_ndef
```
