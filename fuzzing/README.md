# Fuzzing

## Manual usage based on Ledger container

### Preparation

The fuzzer can run from the docker `ledger-app-builder-legacy`. You can download it from the `ghcr.io` docker repository:

```console
sudo docker pull ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder-legacy:latest
```

You can then enter this development environment by executing the following command from the repository root directory:

```console
sudo docker run --rm -ti --user "$(id -u):$(id -g)" -v "$(realpath .):/app" ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder-legacy:latest
```

### Compilation

Once in the container, go into the `fuzzing` folder to compile the fuzzer:

```console
# cmake initialization
cmake -S fuzzing -B fuzzing/build -DCMAKE_C_COMPILER=clang -DSANITIZER=address

# Fuzzer compilation
cmake --build fuzzing/build
```

### Run

```console
./fuzzing/build/fuzz_apdu_parser
./fuzzing/build/fuzz_base58
./fuzzing/build/fuzz_bip32
./fuzzing/build/fuzz_qrcodegen
./fuzzing/build/fuzz_alloc
./fuzzing/build/fuzz_nfc_ndef
```
