#!/bin/bash

# Defaults
REBUILD=0
COMPUTE_COVERAGE=1
RUN_FUZZER=1
TARGET_DEVICE="flex"
REGENERATE_MACROS=0

BOLOS_SDK=""
FUZZING_PATH="$(pwd)"
NUM_CPUS=1
FUZZER=""
FUZZERNAME=""

# Help message
function show_help() {
    echo "Usage: ./local_run.sh --fuzzer=/path/to/fuzz_dispatcher [--build=1|0] [--compute-coverage=1|0]"
    echo
    echo "  --BOLOS_SDK=PATH            Path to the BOLOS SDK (required if fuzzing an App)"
    echo "  --TARGET_DEVICE=[flex|stax] Whether it is a flex or stax device (default: flex)"
    echo "  --fuzzer=PATH               Path to the fuzzer binary (required)"
    echo "  --build=1|0                 Whether to build the project (default: 0)"
    echo "  --re-generate-macros=0|1    Whether to regenerate macros (default: 0)"
    echo "  --compute-coverage=1|0      Whether to compute coverage after fuzzing (default: 1)"
    echo "  --run-fuzzer=1|0            Whether to run the fuzzer (default: 1)"
    echo "  --j=1|...|n_cpus            Number of CPUs to use (default: 1)"
    echo "  --help                      Show this help message"
    exit 0
}

function gen_macros() {
    if [ "$BOLOS_SDK" ]; then
        mkdir -p "$FUZZING_PATH/macros/generated"
        apt-get update && apt-get install -y bear

        cd "$FUZZING_PATH/.." || exit 1
        case "$TARGET_DEVICE" in
            flex)
                make clean BOLOS_SDK=/opt/flex-secure-sdk
                bear --output "$FUZZING_PATH/macros/generated/used_macros.json" -- make -j"$NUM_CPUS" BOLOS_SDK=/opt/flex-secure-sdk
                ;;
            stax)
                make clean BOLOS_SDK=/opt/stax-secure-sdk
                bear --output "$FUZZING_PATH/macros/generated/used_macros.json" -- make -j"$NUM_CPUS" BOLOS_SDK=/opt/stax-secure-sdk
                ;;
            *)
                echo "Unsupported device: $TARGET_DEVICE"
                exit 1
                ;;
        esac
        cd "$FUZZING_PATH/macros/" || exit
        python3 extract_macros.py --file generated/used_macros.json --exclude exclude_macros.txt --add add_macros.txt --output generated/macros.txt
        cd "$FUZZING_PATH" || exit 1
    fi
}

function build() {
    cd "$FUZZING_PATH" || exit
    apt update && apt install -y libclang-rt-dev

    cmake -S . -B build -DCMAKE_C_COMPILER=clang -DSANITIZER=address -DTARGET_DEVICE="$TARGET_DEVICE" -DBOLOS_SDK="$BOLOS_SDK" -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS:bool=On
    cmake --build build
}

# Parse args
for arg in "$@"; do
    case $arg in
        --fuzzer=*)
            FUZZER="${arg#*=}"
            ;;
        --BOLOS_SDK=*)
            BOLOS_SDK="${arg#*=}"
            ;;
        --TARGET_DEVICE=*)
            TARGET_DEVICE="${arg#*=}"
            ;;
        --re-generate-macros=*)
            REGENERATE_MACROS="${arg#*=}"
            ;;
        --build=*)
            REBUILD="${arg#*=}"
            ;;
        --compute-coverage=*)
            COMPUTE_COVERAGE="${arg#*=}"
            ;;
        --run-fuzzer=*)
            RUN_FUZZER="${arg#*=}"
            ;;
        --j=*)
            NUM_CPUS="${arg#*=}"
            ;;
        --help)
            show_help
            ;;
        *)
            echo "Unknown argument: $arg"
            show_help
            ;;
    esac
done

# Set paths
FUZZERNAME=$(basename "$FUZZER")
OUT_DIR="./out/$FUZZERNAME"
CORPUS_DIR="$OUT_DIR/corpus"

# Validate required args
if [ -z "$BOLOS_SDK" ]; then
    echo "Note: If you are fuzzing an App --BOLOS_SDK=$BOLOS_SDK is required."
fi

# Sanity checks
if [ "$TARGET_DEVICE" != "flex" ] && [ "$TARGET_DEVICE" != "stax" ]; then
    echo "Unsupported TARGET_DEVICE: $TARGET_DEVICE"
    exit 1
fi

if [ "$REGENERATE_MACROS" -ne 0 ]; then
    gen_macros
fi

if [ "$REBUILD" -eq 1 ]; then
    build
    echo -e "\n----------\nFuzzer built at $FUZZING_PATH/build/fuzz_*\n----------"
fi

if [ -z "$FUZZER" ] || [ ! -x "$FUZZER" ]; then
    echo -e "\nFuzzer binary '$FUZZER' not set or not executable.\n"
    exit 1
fi

mkdir -p "$CORPUS_DIR"

if [ "$RUN_FUZZER" -eq 1 ]; then
    echo -e "\n----------\nStarting fuzzer '$FUZZERNAME'...\n----------\n"
    LLVM_PROFILE_FILE="$OUT_DIR/fuzzer.profraw" "$FUZZER" -max_len=8192 -jobs="$NUM_CPUS" -timeout=10 "$CORPUS_DIR"
fi

# Early exit if coverage not needed
if [ "$COMPUTE_COVERAGE" -ne 1 ]; then
    mv -- *.log *.profraw "$OUT_DIR" 2>/dev/null
    echo -e "\n----------\nFuzzing done. Data saved to $OUT_DIR\n----------"
    exit 0
fi

# Coverage computation
echo -e "\n----------\nComputing coverage...\n----------"

rm -f "$OUT_DIR/default.profdata" "$OUT_DIR/default.profraw"
LLVM_PROFILE_FILE="$OUT_DIR/coverage.profraw" "$FUZZER" -max_len=8192 -runs=0 "$CORPUS_DIR"

mv -- *.log *.profraw "$OUT_DIR" 2>/dev/null
llvm-profdata merge -sparse "$OUT_DIR"/*.profraw -o "$OUT_DIR/default.profdata"
llvm-cov show "$FUZZER" --ignore-filename-regex="$BOLOS_SDK" -instr-profile="$OUT_DIR/default.profdata" -format=html -output-dir="$OUT_DIR"
llvm-cov report "$FUZZER" --ignore-filename-regex="$BOLOS_SDK" -instr-profile="$OUT_DIR/default.profdata"

echo -e "\n----------"
echo "Report available at $OUT_DIR/index.html"
echo "To view: xdg-open $OUT_DIR/index.html"
echo "----------"
