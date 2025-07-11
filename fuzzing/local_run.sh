#!/bin/bash

# Defaults
REBUILD=0
COMPUTE_COVERAGE=0
RUN_FUZZER=0
TARGET_DEVICE="flex"
REGENERATE_MACROS=0
RUN_CRASH=0
SANITIZER="address"
BOLOS_SDK=""
FUZZING_PATH="$(pwd)"
NUM_CPUS=1
FUZZER=""
FUZZERNAME=""

# Colors
RED="\033[0;31m"
GREEN="\033[0;32m"
BLUE="\033[0;34m"
YELLOW="\033[0;33m"
NC="\033[0m"

# Help message
function show_help() {
    echo -e "${BLUE}Usage: ./local_run.sh --fuzzer=/path/to/fuzz_binary [--build=1|0] [other options:]${NC}"
    echo
    echo -e "  --BOLOS_SDK=PATH            Path to the BOLOS SDK (required if fuzzing an App)"
    echo -e "  --TARGET_DEVICE=[flex|stax] Whether it is a flex or stax device (default: flex)"
    echo -e "  --fuzzer=PATH               Path to the fuzzer binary (required)"
    echo -e "  --build=1|0                 Whether to build the project (default: 0)"
    echo -e "  --sanitizer=address|memory  Compiles fuzzer with -fsanitize=fuzzer,undefined,[address|memory](default: address)"
    echo -e "  --re-generate-macros=0|1    Whether to regenerate macros (default: 0)"
    echo -e "  --compute-coverage=1|0      Whether to compute coverage after fuzzing (default: 0)"
    echo -e "  --run-fuzzer=1|0            Whether to run the fuzzer (default: 0)"
    echo -e "  --run-crash=FILENAME        Run the fuzzer on a specific crash input file (default: 0)"
    echo -e "  --j=N                       Number of parallel jobs/CPUs to use for build and fuzzing (default: 1)"
    echo -e "  --help                      Show this help message and exit"
    echo -e "\nExample:"
    echo -e "  ./local_run.sh --fuzzer=./build/fuzz_myapp --build=1 --run-fuzzer=1${NC}"
    exit 0
}

# Does generated_macros[] = generated_macros[] + add_macros[] - exclude_macros[]
function custom_macros(){
    echo -e "${BLUE}Customizing macros...${NC}"
    if [ "$BOLOS_SDK" ]; then
        cd "$FUZZING_PATH/macros" || exit 1
        python3 "$BOLOS_SDK/fuzzing/macros/extract_macros.py" \
                                --file "generated/used_macros.json" \
                                --exclude "exclude_macros.txt" \
                                --add "add_macros.txt" \
                                --output "generated/macros.txt" \
                                --customsonly=1
        cd "$FUZZING_PATH" || exit 1
    fi
}

function gen_macros() {
    mkdir -p "$FUZZING_PATH/macros/generated"

    # Install bear while it is not added to the docker image
    if ! command -v bear >/dev/null 2>&1; then
        apt-get update && apt-get install -y bear
    fi

    cd "/app" || exit 1
    echo -e "${BLUE}Generating macros...${NC}"
    # $FLEX_SDK and $STAX_SDK are set in the docker image
    case "$TARGET_DEVICE" in
        flex)
            make clean BOLOS_SDK="$FLEX_SDK"
            bear --output "$FUZZING_PATH/macros/generated/used_macros.json" -- make -j"$NUM_CPUS" BOLOS_SDK="$FLEX_SDK"
            ;;
        stax)
            make clean BOLOS_SDK="$STAX_SDK"
            bear --output "$FUZZING_PATH/macros/generated/used_macros.json" -- make -j"$NUM_CPUS" BOLOS_SDK="$STAX_SDK"
            ;;
        *)
            echo -e "${RED}Unsupported TARGET_DEVICE: $TARGET_DEVICE${NC}"
            exit 1
            ;;
    esac
    cd "$FUZZING_PATH/macros/" || exit 1
    python3 "$BOLOS_SDK/fuzzing/macros/extract_macros.py" \
                            --file "generated/used_macros.json" \
                            --exclude exclude_macros.txt \
                            --add add_macros.txt \
                            --output generated/macros.txt

    rm "$FUZZING_PATH/macros/generated/used_macros.json"
    cd "$FUZZING_PATH" || exit 1
}

function build() {
    cd "$FUZZING_PATH" || exit

    # Install clang_rt while it is not added to the docker image
    CLANG_RT_PATH="$(clang -print-resource-dir)/lib/linux"
    if [ ! -f "$CLANG_RT_PATH/libclang_rt.asan-x86_64.a" ]; then
    apt update && apt install -y libclang-rt-dev
    fi

    echo -e "${BLUE}Building the project...${NC}"
    cmake -S . -B build -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Debug -DSANITIZER="$SANITIZER" -DTARGET_DEVICE="$TARGET_DEVICE" -DBOLOS_SDK="$BOLOS_SDK" -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=On
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
        --sanitizer=*)
            SANITIZER="${arg#*=}"
            ;;
        --compute-coverage=*)
            COMPUTE_COVERAGE="${arg#*=}"
            ;;
        --run-crash=*)
            RUN_CRASH="${arg#*=}"
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
            echo -e "${RED}Unknown argument: $arg${NC}"
            show_help
            ;;
    esac
done

# Set paths
FUZZERNAME=$(basename "$FUZZER")
OUT_DIR="./out/$FUZZERNAME"
CORPUS_DIR="$OUT_DIR/corpus"
CRASH_DIR="$OUT_DIR/crashes"

### Validate required args

if [ "$TARGET_DEVICE" != "flex" ] && [ "$TARGET_DEVICE" != "stax" ]; then
    echo -e "${RED}Unsupported TARGET_DEVICE: $TARGET_DEVICE | Must be STAX or FLEX${NC}"
    exit 1
fi

if [ "$SANITIZER" != "address" ] && [ "$SANITIZER" != "memory" ]; then
    echo -e "${RED}Unsupported SANITIZER: $SANITIZER | Must be 'address' or 'memory'${NC}"
    exit 1
fi

if [ ! -f "$FUZZING_PATH/local_run.sh" ]; then
    #Fuzzing an APP
    echo "Fuzzing PATH = $FUZZING_PATH"
    if [ -z "$BOLOS_SDK" ]; then
        echo -e "${RED}Error: --BOLOS_SDK=\$BOLOS_SDK is required.${NC}"
        show_help
    fi
    if [ "$REGENERATE_MACROS" -ne 0 ]; then
        echo -e "${YELLOW}Generating custom macros...${NC}"
        custom_macros
    fi
    if [ "$REBUILD" -eq 1 ] || [ "$REGENERATE_MACROS" -ne 0 ]; then
        if [ ! -s "$FUZZING_PATH/macros/generated/macros.txt" ]; then
            echo -e "${YELLOW}macros.txt is missing or empty. Generating macros...${NC}"
            gen_macros
        fi
        if [ -s "$FUZZING_PATH/macros/generated/macros.txt" ]; then
            echo -e "${YELLOW}Generating custom macros...${NC}"
            custom_macros
        fi
        echo -e "${GREEN}\n----------\nFuzzer built at $FUZZING_PATH/build/fuzz_*\n----------${NC}"
    fi
fi
if [ "$REBUILD" -eq 1 ]; then
    build
fi

### Execute commands

if [ -z "$FUZZER" ]; then
    exit 0
fi

if [ ! -x "$FUZZER" ]; then
    echo -e "${RED}\nFuzzer binary '$FUZZER' is not executable.\n${NC}"
    exit 1
fi

mkdir -p "$CORPUS_DIR"

if [ "$RUN_FUZZER" -eq 1 ]; then
    echo -e "${GREEN}\n----------\nStarting fuzzer '$FUZZERNAME'...\n----------\n${NC}"
    LLVM_PROFILE_FILE="$OUT_DIR/fuzzer.profraw" "$FUZZER" -detect_leaks=0 -max_len=8192 -jobs="$NUM_CPUS" -timeout=10 "$CORPUS_DIR"
    if compgen -G "$FUZZING_PATH/crash-*" > /dev/null; then
        mkdir -p "$CRASH_DIR"
        mv -- crash-* "$CRASH_DIR"
    fi
    mv -- *.log *.profraw "$OUT_DIR" 2>/dev/null
fi

if [ "$COMPUTE_COVERAGE" -eq 1 ]; then
    # Coverage computation
    echo -e "${BLUE}\n----------\nComputing coverage...\n----------${NC}"

    rm -f "$OUT_DIR/default.profdata" "$OUT_DIR/default.profraw"
    LLVM_PROFILE_FILE="$OUT_DIR/coverage.profraw" "$FUZZER" -max_len=8192 -runs=0 "$CORPUS_DIR"

    mv -- *.log *.profraw "$OUT_DIR" 2>/dev/null
    llvm-profdata merge -sparse "$OUT_DIR"/*.profraw -o "$OUT_DIR/default.profdata"
    llvm-cov show "$FUZZER" --ignore-filename-regex="$BOLOS_SDK" -instr-profile="$OUT_DIR/default.profdata" -format=html -output-dir="$OUT_DIR"
    llvm-cov report "$FUZZER" --ignore-filename-regex="$BOLOS_SDK" -instr-profile="$OUT_DIR/default.profdata"

    echo -e "${GREEN}\n----------"
    echo "Report available at $OUT_DIR/index.html"
    echo "To view: xdg-open $OUT_DIR/index.html"
    echo -e "----------${NC}"
fi

if [ -n "$RUN_CRASH" ] && [ "$RUN_CRASH" != "0" ]; then
    echo -e "${BLUE}\n-------- Running crash input: ${RUN_CRASH} --------${NC}"

    CRASH_INPUT="$CRASH_DIR/$RUN_CRASH"
    if [ ! -f "$CRASH_INPUT" ]; then
        echo -e "${RED}Crash file '$CRASH_INPUT' not found!${NC}\n"
        exit 1
    fi

    echo -e "${YELLOW}Executing crash input under fuzzer...${NC}"
    LLVM_PROFILE_FILE="$OUT_DIR/crash.profraw" "$FUZZER" -runs=1 -timeout=10 "$CRASH_INPUT"

    echo -e "${GREEN}\n----------"
    echo "Crash execution complete for $RUN_CRASH"
    echo -e "----------${NC}"
fi
