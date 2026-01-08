#!/bin/bash
#
# voip-utility Test Suite Runner
# Comprehensive automated testing for the VoIP testing utility
#
# Usage:
#   ./run_test_suite.sh              # Run all tests
#   ./run_test_suite.sh --list       # List available tests
#   ./run_test_suite.sh 01 02 03     # Run specific tests by number
#   ./run_test_suite.sh --quick      # Run quick tests only (01-05)
#   ./run_test_suite.sh --verbose    # Enable verbose output
#   ./run_test_suite.sh --json       # Output results as JSON
#

# Don't use set -e as we handle errors manually
# set -e

# Script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Configuration
VOIP_UTILITY="${PROJECT_DIR}/build/voip-utility"
CONFIG_FILE="${PROJECT_DIR}/examples/config.json"
TESTS_DIR="${SCRIPT_DIR}"
RECORDINGS_DIR="${PROJECT_DIR}/recordings"
SIP_SERVER="192.168.10.10"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color
BOLD='\033[1m'

# Test results
declare -a PASSED_TESTS=()
declare -a FAILED_TESTS=()
declare -a SKIPPED_TESTS=()
TOTAL_DURATION=0

# Options
VERBOSE=false
JSON_OUTPUT=false
STOP_ON_FAIL=false
QUICK_MODE=false

#------------------------------------------------------------------------------
# Helper functions
#------------------------------------------------------------------------------

log_info() {
    if [ "$JSON_OUTPUT" = false ]; then
        echo -e "${BLUE}[INFO]${NC} $1"
    fi
}

log_success() {
    if [ "$JSON_OUTPUT" = false ]; then
        echo -e "${GREEN}[PASS]${NC} $1"
    fi
}

log_error() {
    if [ "$JSON_OUTPUT" = false ]; then
        echo -e "${RED}[FAIL]${NC} $1"
    fi
}

log_warn() {
    if [ "$JSON_OUTPUT" = false ]; then
        echo -e "${YELLOW}[WARN]${NC} $1"
    fi
}

log_skip() {
    if [ "$JSON_OUTPUT" = false ]; then
        echo -e "${CYAN}[SKIP]${NC} $1"
    fi
}

print_header() {
    if [ "$JSON_OUTPUT" = false ]; then
        echo ""
        echo -e "${BOLD}========================================${NC}"
        echo -e "${BOLD}  VoIP Utility Test Suite${NC}"
        echo -e "${BOLD}========================================${NC}"
        echo ""
    fi
}

print_separator() {
    if [ "$JSON_OUTPUT" = false ]; then
        echo -e "${BLUE}----------------------------------------${NC}"
    fi
}

#------------------------------------------------------------------------------
# Pre-flight checks
#------------------------------------------------------------------------------

check_prerequisites() {
    log_info "Running pre-flight checks..."

    # Check if voip-utility binary exists
    if [ ! -x "$VOIP_UTILITY" ]; then
        log_error "voip-utility binary not found at: $VOIP_UTILITY"
        log_info "Please build the project first: cd $PROJECT_DIR && meson setup build && ninja -C build"
        return 1
    fi

    # Check if config file exists
    if [ ! -f "$CONFIG_FILE" ]; then
        log_error "Configuration file not found: $CONFIG_FILE"
        return 1
    fi

    # Create recordings directory if needed
    mkdir -p "$RECORDINGS_DIR"

    # Check SIP server connectivity
    log_info "Checking SIP server connectivity ($SIP_SERVER)..."
    if ! ping -c 1 -W 2 "$SIP_SERVER" > /dev/null 2>&1; then
        log_warn "SIP server at $SIP_SERVER is not responding to ping"
        log_warn "Tests may fail if server is not available"
    else
        log_info "SIP server is reachable"
    fi

    # Verify accounts are configured
    if ! grep -q '"ext6010"' "$CONFIG_FILE" || ! grep -q '"ext6011"' "$CONFIG_FILE"; then
        log_error "Required accounts ext6010 and ext6011 not found in config"
        return 1
    fi

    log_info "Pre-flight checks passed"
    return 0
}

#------------------------------------------------------------------------------
# Test discovery
#------------------------------------------------------------------------------

get_test_files() {
    find "$TESTS_DIR" -maxdepth 1 -name "*.json" -type f | sort
}

get_test_name() {
    local test_file="$1"
    basename "$test_file" .json
}

list_tests() {
    echo ""
    echo "Available tests:"
    echo ""
    local count=0
    for test_file in $(get_test_files); do
        local name=$(get_test_name "$test_file")
        local desc=$(jq -r '.description // "No description"' "$test_file" 2>/dev/null | head -c 60)
        printf "  %-35s %s\n" "$name" "$desc"
        ((count++))
    done
    echo ""
    echo "Total: $count tests"
}

#------------------------------------------------------------------------------
# Test execution
#------------------------------------------------------------------------------

run_single_test() {
    local test_file="$1"
    local test_name=$(get_test_name "$test_file")

    if [ "$JSON_OUTPUT" = false ]; then
        print_separator
        echo -e "${BOLD}Running: $test_name${NC}"
    fi

    local start_time=$(date +%s.%N)
    local output_file=$(mktemp)
    local exit_code=0

    # Build command - note: -v must come BEFORE the test subcommand
    local cmd="$VOIP_UTILITY -c $CONFIG_FILE"
    if [ "$VERBOSE" = true ]; then
        cmd="$cmd -v"
    fi
    cmd="$cmd test -f $test_file"

    # Run the test from the project directory (for relative paths in tests)
    pushd "$PROJECT_DIR" > /dev/null
    if $cmd > "$output_file" 2>&1; then
        exit_code=0
    else
        exit_code=$?
    fi
    popd > /dev/null

    local end_time=$(date +%s.%N)
    local duration=$(echo "$end_time - $start_time" | bc)
    TOTAL_DURATION=$(echo "$TOTAL_DURATION + $duration" | bc)

    # Parse result from output
    local result_status="unknown"
    if grep -q "Test.*passed" "$output_file" 2>/dev/null; then
        result_status="passed"
    elif grep -q "Test.*failed" "$output_file" 2>/dev/null; then
        result_status="failed"
    elif grep -q "Test.*timeout" "$output_file" 2>/dev/null; then
        result_status="timeout"
    elif grep -q "Test.*error" "$output_file" 2>/dev/null; then
        result_status="error"
    fi

    # Determine pass/fail
    if [ "$exit_code" -eq 0 ] && [ "$result_status" = "passed" ]; then
        PASSED_TESTS+=("$test_name")
        log_success "$test_name (${duration}s)"
    else
        FAILED_TESTS+=("$test_name")
        log_error "$test_name (${duration}s)"

        # Show error details
        if [ "$JSON_OUTPUT" = false ]; then
            echo "  Exit code: $exit_code"
            echo "  Status: $result_status"
            if [ "$VERBOSE" = true ]; then
                echo "  Output:"
                sed 's/^/    /' "$output_file"
            else
                # Show last few lines of output
                echo "  Last output:"
                tail -5 "$output_file" | sed 's/^/    /'
            fi
        fi
    fi

    rm -f "$output_file"

    # Stop on failure if requested
    if [ "$STOP_ON_FAIL" = true ] && [ "${#FAILED_TESTS[@]}" -gt 0 ]; then
        log_warn "Stopping due to test failure (--stop-on-fail)"
        return 1
    fi

    return 0
}

run_tests() {
    local test_list=("$@")

    # If no specific tests requested, run all
    if [ ${#test_list[@]} -eq 0 ]; then
        for test_file in $(get_test_files); do
            test_list+=("$test_file")
        done
    fi

    # Filter for quick mode
    if [ "$QUICK_MODE" = true ]; then
        local quick_tests=()
        for test_file in "${test_list[@]}"; do
            local name=$(get_test_name "$test_file")
            if [[ "$name" =~ ^(01|02|03|04|05) ]]; then
                quick_tests+=("$test_file")
            fi
        done
        test_list=("${quick_tests[@]}")
        log_info "Quick mode: running ${#test_list[@]} tests"
    fi

    local total=${#test_list[@]}
    log_info "Running $total tests..."
    echo ""

    local count=0
    for test_file in "${test_list[@]}"; do
        ((count++))

        if [ ! -f "$test_file" ]; then
            # Try to find test by number/name
            local found=$(find "$TESTS_DIR" -maxdepth 1 -name "*${test_file}*.json" -type f | head -1)
            if [ -n "$found" ]; then
                test_file="$found"
            else
                log_skip "Test not found: $test_file"
                SKIPPED_TESTS+=("$test_file")
                continue
            fi
        fi

        if [ "$JSON_OUTPUT" = false ]; then
            echo -ne "\r${BLUE}Progress: $count/$total${NC}  "
        fi

        if ! run_single_test "$test_file"; then
            break
        fi

        # Delay between tests to allow SIP registration cleanup
        sleep 3
    done
}

#------------------------------------------------------------------------------
# Results reporting
#------------------------------------------------------------------------------

print_summary() {
    local passed=${#PASSED_TESTS[@]}
    local failed=${#FAILED_TESTS[@]}
    local skipped=${#SKIPPED_TESTS[@]}
    local total=$((passed + failed + skipped))

    if [ "$JSON_OUTPUT" = true ]; then
        # JSON output
        echo "{"
        echo "  \"total\": $total,"
        echo "  \"passed\": $passed,"
        echo "  \"failed\": $failed,"
        echo "  \"skipped\": $skipped,"
        echo "  \"duration_sec\": $TOTAL_DURATION,"
        echo "  \"passed_tests\": [$(printf '"%s",' "${PASSED_TESTS[@]}" | sed 's/,$//')],"
        echo "  \"failed_tests\": [$(printf '"%s",' "${FAILED_TESTS[@]}" | sed 's/,$//')],"
        echo "  \"skipped_tests\": [$(printf '"%s",' "${SKIPPED_TESTS[@]}" | sed 's/,$//')],"
        echo "  \"success\": $([ $failed -eq 0 ] && echo "true" || echo "false")"
        echo "}"
    else
        # Human-readable output
        echo ""
        print_separator
        echo -e "${BOLD}Test Summary${NC}"
        print_separator
        echo ""
        echo -e "  Total:    $total"
        echo -e "  ${GREEN}Passed:   $passed${NC}"
        echo -e "  ${RED}Failed:   $failed${NC}"
        echo -e "  ${CYAN}Skipped:  $skipped${NC}"
        echo -e "  Duration: ${TOTAL_DURATION}s"
        echo ""

        if [ $failed -gt 0 ]; then
            echo -e "${RED}Failed tests:${NC}"
            for t in "${FAILED_TESTS[@]}"; do
                echo "  - $t"
            done
            echo ""
        fi

        if [ $failed -eq 0 ]; then
            echo -e "${GREEN}${BOLD}All tests passed!${NC}"
        else
            echo -e "${RED}${BOLD}Some tests failed.${NC}"
        fi
        echo ""
    fi
}

#------------------------------------------------------------------------------
# Main
#------------------------------------------------------------------------------

main() {
    local test_args=()

    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --list|-l)
                list_tests
                exit 0
                ;;
            --verbose|-v)
                VERBOSE=true
                shift
                ;;
            --json|-j)
                JSON_OUTPUT=true
                shift
                ;;
            --stop-on-fail|-s)
                STOP_ON_FAIL=true
                shift
                ;;
            --quick|-q)
                QUICK_MODE=true
                shift
                ;;
            --help|-h)
                echo "Usage: $0 [OPTIONS] [TEST_NUMBERS...]"
                echo ""
                echo "Options:"
                echo "  -l, --list         List available tests"
                echo "  -v, --verbose      Verbose output"
                echo "  -j, --json         Output results as JSON"
                echo "  -s, --stop-on-fail Stop on first failure"
                echo "  -q, --quick        Run quick tests only (01-05)"
                echo "  -h, --help         Show this help"
                echo ""
                echo "Examples:"
                echo "  $0                 Run all tests"
                echo "  $0 01 02 03        Run specific tests"
                echo "  $0 --quick -v      Run quick tests with verbose output"
                exit 0
                ;;
            *)
                test_args+=("$1")
                shift
                ;;
        esac
    done

    print_header

    # Pre-flight checks
    if ! check_prerequisites; then
        exit 1
    fi

    # Run tests
    run_tests "${test_args[@]}"

    # Print summary
    print_summary

    # Exit with appropriate code
    if [ ${#FAILED_TESTS[@]} -gt 0 ]; then
        exit 1
    fi
    exit 0
}

main "$@"
