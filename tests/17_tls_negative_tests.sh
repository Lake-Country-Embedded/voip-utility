#!/bin/bash
#
# TLS Security Tier Negative Tests
# Verifies that connections are REJECTED when using the wrong transport/encryption.
#
# These tests complement the JSON-based positive tests (13-16) by checking that
# the Asterisk server properly enforces security tier restrictions.
#
# Usage:
#   ./tests/17_tls_negative_tests.sh
#   ./tests/17_tls_negative_tests.sh --verbose

set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
VOIP_UTILITY="${PROJECT_DIR}/build/voip-utility"
CONFIG_FILE="${PROJECT_DIR}/examples/config-tls-test.json"

VERBOSE=false
[[ "$1" == "--verbose" || "$1" == "-v" ]] && VERBOSE=true

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

PASSED=0
FAILED=0

expect_fail() {
    local desc="$1"
    local account="$2"
    local timeout="${3:-5}"

    echo -n "  $desc ... "
    local output
    output=$("$VOIP_UTILITY" -c "$CONFIG_FILE" register -a "$account" -t "$timeout" 2>&1)
    local rc=$?

    if [[ "$VERBOSE" == true ]]; then
        echo ""
        echo "$output" | sed 's/^/    /'
    fi

    if [[ $rc -ne 0 ]]; then
        echo -e "${GREEN}PASS${NC} (rejected as expected)"
        ((PASSED++))
    else
        echo -e "${RED}FAIL${NC} (should have been rejected but succeeded)"
        ((FAILED++))
    fi
}

expect_pass() {
    local desc="$1"
    local account="$2"
    local timeout="${3:-10}"

    echo -n "  $desc ... "
    local output
    output=$("$VOIP_UTILITY" -c "$CONFIG_FILE" register -a "$account" -t "$timeout" 2>&1)
    local rc=$?

    if [[ "$VERBOSE" == true ]]; then
        echo ""
        echo "$output" | sed 's/^/    /'
    fi

    if [[ $rc -eq 0 ]]; then
        echo -e "${GREEN}PASS${NC}"
        ((PASSED++))
    else
        echo -e "${RED}FAIL${NC} (expected success but got rc=$rc)"
        ((FAILED++))
    fi
}

# --- Prerequisite check ---
if [[ ! -x "$VOIP_UTILITY" ]]; then
    echo "Error: voip-utility not found at $VOIP_UTILITY" >&2
    exit 1
fi
if [[ ! -f "$CONFIG_FILE" ]]; then
    echo "Error: config not found at $CONFIG_FILE" >&2
    exit 1
fi

echo ""
echo "=== TLS Security Tier Negative Tests ==="
echo ""

# We need a temporary config with "bad" accounts for negative tests
TMPCONFIG=$(mktemp /tmp/voip-tls-neg-XXXXXX.json)
trap "rm -f $TMPCONFIG" EXIT

# Create config with accounts that use the wrong transport/encryption
cat > "$TMPCONFIG" <<'NEGEOF'
{
  "accounts": [
    {
      "id": "open-udp-ok",
      "username": "6000",
      "password": "123456",
      "server": "192.168.10.10",
      "port": 5060,
      "transport": "udp",
      "srtp": "disabled",
      "reg_timeout_sec": 300,
      "enabled": true
    },
    {
      "id": "medium-via-udp",
      "username": "6050",
      "password": "123456",
      "server": "192.168.10.10",
      "port": 5060,
      "transport": "udp",
      "srtp": "disabled",
      "reg_timeout_sec": 300,
      "enabled": true
    },
    {
      "id": "strict-via-udp",
      "username": "6070",
      "password": "123456",
      "server": "192.168.10.10",
      "port": 5060,
      "transport": "udp",
      "srtp": "disabled",
      "reg_timeout_sec": 300,
      "enabled": true
    },
    {
      "id": "strict-no-srtp",
      "username": "6070",
      "password": "123456",
      "server": "192.168.10.10",
      "port": 5061,
      "transport": "tls",
      "srtp": "disabled",
      "reg_timeout_sec": 300,
      "enabled": true
    }
  ],
  "tls": {
NEGEOF

# Inject the real cert paths from the main config
python3 -c "
import json, sys
with open('$CONFIG_FILE') as f:
    cfg = json.load(f)
tls = cfg.get('tls', {})
print(json.dumps(tls, indent=4).lstrip('{').rstrip('}'))
" >> "$TMPCONFIG"

cat >> "$TMPCONFIG" <<'NEGEOF2'
  },
  "audio": {"sample_rate": 16000, "frame_duration_ms": 20, "default_codec": "PCMU"},
  "beep_detection": {"min_level_db": -40, "min_duration_sec": 0.2, "max_duration_sec": 0.6, "target_freq_hz": 0, "freq_tolerance_hz": 50, "gap_duration_sec": 0.1},
  "recordings_dir": "./recordings",
  "tests_dir": "./tests",
  "log_level": "info",
  "json_output": false
}
NEGEOF2

# Override CONFIG_FILE for negative tests
CONFIG_FILE="$TMPCONFIG"

echo "Positive controls (should succeed):"
expect_pass "Open tier via UDP (6000)" "open-udp-ok"
sleep 1

echo ""
echo "Negative tests (should be rejected):"
expect_fail "Medium tier via UDP (6050) — should require TLS" "medium-via-udp" 5
sleep 1
expect_fail "Strict tier via UDP (6070) — should require TLS" "strict-via-udp" 5
sleep 1

echo ""
echo "=== Results: ${PASSED} passed, ${FAILED} failed ==="
echo ""

[[ $FAILED -eq 0 ]] && exit 0 || exit 1
