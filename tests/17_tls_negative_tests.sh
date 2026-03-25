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
SETUP_SCRIPT="${SCRIPT_DIR}/setup_test_env.sh"

# Generate config from environment.json
if [[ -x "$SETUP_SCRIPT" ]]; then
    eval "$("$SETUP_SCRIPT" 2>/dev/null)"
    CONFIG_FILE="${VOIP_UTIL_CONFIG:-${PROJECT_DIR}/examples/config-tls-test.json}"
else
    CONFIG_FILE="${PROJECT_DIR}/examples/config-tls-test.json"
fi

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

# Build negative-test config: deliberately wrong transport/encryption combos
# Derive server/port/TLS from the generated config
TMPCONFIG=$(mktemp /tmp/voip-tls-neg-XXXXXX.json)
trap "rm -f $TMPCONFIG" EXIT

python3 -c "
import json, sys
with open('$CONFIG_FILE') as f:
    cfg = json.load(f)

accts = {a['id']: a for a in cfg['accounts']}
udp_acct = next(a for a in cfg['accounts'] if a['transport'] == 'udp')
server = udp_acct['server']
udp_port = udp_acct['port']

def bad_account(aid, src_id, transport, srtp='disabled', port=None):
    src = accts[src_id]
    return {
        'id': aid, 'username': src['username'], 'password': src['password'],
        'server': server, 'port': port or udp_port,
        'transport': transport, 'srtp': srtp,
        'reg_timeout_sec': 300, 'enabled': True
    }

cfg['accounts'] = [
    bad_account('open-udp-ok', 'ext6010', 'udp'),
    bad_account('medium-via-udp', 'ext6050', 'udp'),
    bad_account('strict-via-udp', 'ext6070', 'udp'),
]
json.dump(cfg, sys.stdout, indent=2)
" > "$TMPCONFIG"

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
