#!/bin/bash
#
# Generate voip-utility test configuration from centralized environment.json
#
# Reads testing/config/environment.json and produces a voip-utility config
# at /tmp/voip-util-conf/config.json with all tester accounts and TLS settings.
#
# Usage:
#   ./tests/setup_test_env.sh              # Generate config, print path
#   ./tests/setup_test_env.sh --print-path # Just print the output path
#   eval $(./tests/setup_test_env.sh)      # Sets VOIP_UTIL_CONFIG env var

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
REPO_ROOT="$(cd "$PROJECT_DIR/../../../" && pwd)"
ENV_FILE="${REPO_ROOT}/testing/config/environment.json"
OUTPUT_DIR="/tmp/voip-util-conf"
OUTPUT_FILE="${OUTPUT_DIR}/config.json"

if [[ "$1" == "--print-path" ]]; then
    echo "$OUTPUT_FILE"
    exit 0
fi

if [[ ! -f "$ENV_FILE" ]]; then
    echo "Error: environment.json not found at $ENV_FILE" >&2
    exit 1
fi

if ! command -v jq &>/dev/null; then
    echo "Error: jq is required but not installed" >&2
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

# Generate the voip-utility config from environment.json
jq --arg repo_root "$REPO_ROOT" '
# Resolve server name to host IP
def resolve_server:
    . as $name |
    if $name == "primary" then .
    elif $name == "alternate" then .
    else $name
    end;

# Build the accounts array from tester_accounts
.sip_servers.primary.host as $primary_host |
.sip_servers.primary.port as $primary_port |
.sip_servers.primary.tls_port as $primary_tls_port |
.sip_servers.alternate.host as $alt_host |

# Map tester_accounts to voip-utility account format
[.sip_accounts.tester_accounts[] | {
    id: .id,
    username: .username,
    password: .password,
    server: (if .server == "primary" then $primary_host
             elif .server == "alternate" then $alt_host
             else .server end),
    port: (if .transport == "tls" then $primary_tls_port else $primary_port end),
    transport: .transport,
    srtp: (.srtp // "disabled"),
    reg_timeout_sec: 300,
    enabled: true
}] as $accounts |

# Build TLS config with absolute paths
(.tls | {
    ca_file: ($repo_root + "/" + .ca_file),
    cert_file: ($repo_root + "/" + .host_cert_file),
    key_file: ($repo_root + "/" + .host_key_file),
    verify_server: .verify_server
}) as $tls |

# Assemble the full config
{
    accounts: $accounts,
    tls: $tls,
    audio: {
        sample_rate: 16000,
        frame_duration_ms: 20,
        default_codec: "PCMU"
    },
    beep_detection: {
        min_level_db: -40,
        min_duration_sec: 0.2,
        max_duration_sec: 0.6,
        target_freq_hz: 0,
        freq_tolerance_hz: 50,
        gap_duration_sec: 0.1
    },
    recordings_dir: "./recordings",
    tests_dir: "./tests",
    log_level: "info",
    json_output: false
}
' "$ENV_FILE" > "$OUTPUT_FILE"

if [[ $? -ne 0 ]]; then
    echo "Error: Failed to generate config" >&2
    exit 1
fi

echo "VOIP_UTIL_CONFIG=${OUTPUT_FILE}"
