#!/bin/bash
#
# Run all test cases in the examples directory
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
VOIP_UTILITY="$PROJECT_DIR/build/voip-utility"
CONFIG="$SCRIPT_DIR/config.json"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if utility exists
if [ ! -x "$VOIP_UTILITY" ]; then
    echo -e "${RED}Error: voip-utility not found at $VOIP_UTILITY${NC}"
    echo "Run 'ninja -C build' to build the project first."
    exit 1
fi

# Check if config exists
if [ ! -f "$CONFIG" ]; then
    echo -e "${RED}Error: Config file not found at $CONFIG${NC}"
    exit 1
fi

# Counters
PASSED=0
FAILED=0
TOTAL=0

echo "=========================================="
echo "VoIP Utility Automated Test Suite"
echo "=========================================="
echo ""

# Run each test file
for test_file in "$SCRIPT_DIR"/*_test.json; do
    if [ ! -f "$test_file" ]; then
        continue
    fi

    TOTAL=$((TOTAL + 1))
    TEST_NAME=$(basename "$test_file" .json)

    echo -e "${YELLOW}Running: $TEST_NAME${NC}"

    if "$VOIP_UTILITY" -c "$CONFIG" test -f "$test_file" > /tmp/test_output.log 2>&1; then
        echo -e "${GREEN}  PASSED${NC}"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}  FAILED${NC}"
        echo "  Output:"
        tail -5 /tmp/test_output.log | sed 's/^/    /'
        FAILED=$((FAILED + 1))
    fi

    # Small delay between tests
    sleep 2
done

echo ""
echo "=========================================="
echo "Results: $PASSED passed, $FAILED failed (of $TOTAL total)"
echo "=========================================="

if [ $FAILED -gt 0 ]; then
    exit 1
fi

exit 0
