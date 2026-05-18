#!/bin/bash

#===============================================================================
#
#     Env Variables
#
#===============================================================================
REBUILD=0
COMPUTE_COVERAGE=0
RUN_FUZZER=0
RUN_CRASH=0
SANITIZER="address"
BOLOS_SDK=""
FUZZING_PATH="$(pwd)"
NUM_CPUS=1
FUZZER=""
FUZZERNAME=""
if [ ! -f "$FUZZING_PATH/local_run.sh" ]; then
    APP_BUILD_PATH="/app/$(cd /app && ledger-manifest -ob ledger_app.toml)"
fi
RED="\033[0;31m"
GREEN="\033[0;32m"
BLUE="\033[0;34m"
YELLOW="\033[0;33m"
NC="\033[0m"

#===============================================================================
#
#     Help message
#
#===============================================================================
function show_help() {
    echo -e "${BLUE}Usage: ./local_run.sh --fuzzer=/path/to/fuzz_binary [--build=1|0] [other options:]${NC}"
    echo
    echo -e "  --BOLOS_SDK=PATH            Path to the BOLOS SDK (required if fuzzing an App)"
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

#===============================================================================
#
#     Build the fuzzer
#
#===============================================================================
function build() {
    cd "$FUZZING_PATH" || exit

    # Install clang_rt while it is not added to the docker image
    CLANG_RT_PATH="$(clang -print-resource-dir)/lib/linux"
    if [ ! -f "$CLANG_RT_PATH/libclang_rt.asan-x86_64.a" ]; then
    apt update && apt install -y libclang-rt-dev
    fi

    echo -e "${BLUE}Building the project...${NC}"
    if [ ! -f "$FUZZING_PATH/local_run.sh" ]; then
        cmake -S . -B build -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Debug -DSANITIZER="$SANITIZER" -DAPP_BUILD_PATH="${APP_BUILD_PATH}" -DBOLOS_SDK="$BOLOS_SDK" -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=On
    else
        cmake -S . -B build -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Debug -DSANITIZER="$SANITIZER" -DBOLOS_SDK="$BOLOS_SDK" -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=On
    fi
    cmake --build build
}


#===============================================================================
#
#     Parsing parameters
#
#===============================================================================
for arg in "$@"; do
    case $arg in
        --fuzzer=*)
            FUZZER="${arg#*=}"
            ;;
        --BOLOS_SDK=*)
            BOLOS_SDK="${arg#*=}"
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

#===============================================================================
#
#     Set paths
#
#===============================================================================
FUZZERNAME=$(basename "$FUZZER")
OUT_DIR="./out/$FUZZERNAME"
CORPUS_DIR="$OUT_DIR/corpus"
CRASH_DIR="$OUT_DIR/crashes"

#===============================================================================
#
#     Validate required args
#
#===============================================================================
if [ "$SANITIZER" != "address" ] && [ "$SANITIZER" != "memory" ]; then
    echo -e "${RED}Unsupported SANITIZER: $SANITIZER | Must be 'address' or 'memory'${NC}"
    exit 1
fi

#===============================================================================
#
#     Build
#
#===============================================================================
if [ "$REBUILD" -eq 1 ]; then
    build
fi

#===============================================================================
#
#     Fuzz
#
#===============================================================================
if [ -z "$FUZZER" ]; then
    exit 0
fi

if [ ! -x "$FUZZER" ]; then
    echo -e "${RED}\nFuzzer binary '$FUZZER' is not executable.\n${NC}"
    exit 1
fi

mkdir -p "$CORPUS_DIR"

INITIAL_CORPUS_DIR="$FUZZING_PATH/harness/$FUZZERNAME"
if [ "$RUN_FUZZER" -eq 1 ]; then
    if [ -d "$INITIAL_CORPUS_DIR" ]; then
        echo -e "${BLUE}Checking for initial corpus in: $INITIAL_CORPUS_DIR${NC}"
        for file in "$INITIAL_CORPUS_DIR"/*; do
            filename=$(basename "$file")
            if [ ! -f "$CORPUS_DIR/$filename" ]; then
                echo -e "${YELLOW}Copying initial input '$filename' to corpus...${NC}"
                cp "$file" "$CORPUS_DIR/"
            fi
        done
    else
        echo -e "${YELLOW}No initial corpus found at $INITIAL_CORPUS_DIR${NC}"
    fi
    echo -e "${GREEN}\n----------\nStarting fuzzer '$FUZZERNAME'...\n----------\n${NC}"
    LLVM_PROFILE_FILE="$OUT_DIR/fuzzer.profraw" "$FUZZER" -detect_leaks=0 -max_len=8192 -jobs="$NUM_CPUS" -timeout=10 "$CORPUS_DIR"
    if compgen -G "$FUZZING_PATH/crash-*" > /dev/null; then
        mkdir -p "$CRASH_DIR"
        mv -- crash-* "$CRASH_DIR"
    fi
    mv -- *.log *.profraw "$OUT_DIR" 2>/dev/null
fi

#===============================================================================
#
#     Compute coverage
#
#===============================================================================
if [ "$COMPUTE_COVERAGE" -eq 1 ]; then
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

#===============================================================================
#
#     Run a crash
#
#===============================================================================
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
