# Automated VoIP Testing Guide

This guide explains how to create automated test cases for VoIP systems using voip-utility's test framework.

## Overview

The test framework allows you to define call scenarios in JSON files that specify:
- **Caller** and **Receiver** accounts
- **Actions** each party performs during the call
- **Expected results** to verify

Tests run both parties within a single process, making calls through your PBX and verifying the results automatically.

## Quick Start

Run a test with:
```bash
./voip-utility -c config.json test -f my_test.json
```

## Test Definition Format

```json
{
  "name": "Test Name",
  "description": "What this test verifies",
  "timeout": 30,

  "caller": {
    "account": "ext6004",
    "uri": "sip:6003@192.168.10.10",
    "timeout": 10,
    "actions": [
      {"action": "wait", "seconds": 1},
      {"action": "send_dtmf", "digits": "123"}
    ]
  },

  "receiver": {
    "account": "ext6003",
    "auto_answer": true,
    "timeout": 15,
    "actions": [
      {"action": "record_audio", "file": "recordings/test.wav"}
    ]
  },

  "expect": {
    "connected": true,
    "beep_count": 3
  }
}
```

## Configuration Reference

### Top-Level Fields

| Field | Type | Description |
|-------|------|-------------|
| `name` | string | Test name (shown in output) |
| `description` | string | Optional description |
| `timeout` | integer | Overall test timeout in seconds |

### Caller Configuration

| Field | Type | Description |
|-------|------|-------------|
| `account` | string | Account ID from config.json |
| `uri` | string | SIP URI to call (e.g., `sip:1234@pbx.local`) |
| `timeout` | integer | Call connection timeout in seconds |
| `actions` | array | Actions to perform after call connects |

### Receiver Configuration

| Field | Type | Description |
|-------|------|-------------|
| `account` | string | Account ID from config.json |
| `auto_answer` | boolean | Automatically answer incoming calls |
| `timeout` | integer | Timeout for receiver operations |
| `actions` | array | Actions to perform after answering |

### Expected Results

| Field | Type | Description |
|-------|------|-------------|
| `connected` | boolean | Expect call to connect successfully |
| `beep_count` | integer | Number of beeps to detect in recording |
| `beep_frequency` | number | Expected beep frequency in Hz (optional) |

## Available Actions

### wait
Pause execution for a specified duration.

```json
{"action": "wait", "seconds": 1.5}
```

### send_dtmf
Send DTMF digits via RFC2833.

```json
{"action": "send_dtmf", "digits": "1234#"}
```

Supported digits: `0-9`, `*`, `#`, `A-D`

### expect_dtmf
Verify DTMF digits were received. Place this in the receiver's actions.

```json
{"action": "expect_dtmf", "pattern": "1234", "timeout": 10}
```

### play_audio
Play a WAV file into the call.

```json
{"action": "play_audio", "file": "audio/prompt.wav"}
```

Audio files should be:
- WAV format (PCM)
- 8000 or 16000 Hz sample rate
- 16-bit mono

### record_audio
Record audio from the call to a WAV file.

```json
{"action": "record_audio", "file": "recordings/call.wav"}
```

Recording starts immediately and continues until call ends.

### hangup
End the call with an optional SIP response code.

```json
{"action": "hangup", "code": 200}
```

Common codes:
- `200` - Normal hangup
- `486` - Busy
- `603` - Decline

## Example Test Cases

### Example 1: Basic Call Connection

Verify two extensions can call each other.

```json
{
  "name": "Basic Call Test",
  "description": "Verify ext6004 can call ext6003",
  "timeout": 20,

  "caller": {
    "account": "ext6004",
    "uri": "sip:6003@192.168.10.10",
    "timeout": 10,
    "actions": [
      {"action": "wait", "seconds": 2},
      {"action": "hangup", "code": 200}
    ]
  },

  "receiver": {
    "account": "ext6003",
    "auto_answer": true,
    "timeout": 15,
    "actions": []
  },

  "expect": {
    "connected": true
  }
}
```

### Example 2: DTMF IVR Navigation

Test navigating an IVR menu with DTMF.

```json
{
  "name": "IVR Navigation Test",
  "description": "Navigate to sales department via IVR",
  "timeout": 30,

  "caller": {
    "account": "ext6004",
    "uri": "sip:8000@192.168.10.10",
    "timeout": 15,
    "actions": [
      {"action": "wait", "seconds": 3},
      {"action": "send_dtmf", "digits": "1"},
      {"action": "wait", "seconds": 2},
      {"action": "send_dtmf", "digits": "2"},
      {"action": "wait", "seconds": 5},
      {"action": "hangup", "code": 200}
    ]
  },

  "receiver": {
    "account": "ext6003",
    "auto_answer": true,
    "timeout": 20,
    "actions": []
  },

  "expect": {
    "connected": true
  }
}
```

### Example 3: DTMF Echo Test

Verify DTMF digits are transmitted correctly between endpoints.

```json
{
  "name": "DTMF Transmission Test",
  "description": "Verify DTMF digits are received correctly",
  "timeout": 25,

  "caller": {
    "account": "ext6004",
    "uri": "sip:6003@192.168.10.10",
    "timeout": 10,
    "actions": [
      {"action": "wait", "seconds": 1},
      {"action": "send_dtmf", "digits": "0123456789*#"},
      {"action": "wait", "seconds": 3},
      {"action": "hangup", "code": 200}
    ]
  },

  "receiver": {
    "account": "ext6003",
    "auto_answer": true,
    "timeout": 15,
    "actions": [
      {"action": "expect_dtmf", "pattern": "0123456789*#", "timeout": 10}
    ]
  },

  "expect": {
    "connected": true
  }
}
```

### Example 4: Audio Quality Test with Beeps

Play audio from caller and verify it's received.

```json
{
  "name": "Audio Path Test",
  "description": "Verify audio is transmitted from caller to receiver",
  "timeout": 30,

  "caller": {
    "account": "ext6004",
    "uri": "sip:6003@192.168.10.10",
    "timeout": 10,
    "actions": [
      {"action": "wait", "seconds": 0.5},
      {"action": "play_audio", "file": "test_audio/three_beeps.wav"},
      {"action": "wait", "seconds": 3},
      {"action": "hangup", "code": 200}
    ]
  },

  "receiver": {
    "account": "ext6003",
    "auto_answer": true,
    "timeout": 15,
    "actions": [
      {"action": "record_audio", "file": "recordings/audio_test.wav"}
    ]
  },

  "expect": {
    "connected": true,
    "beep_count": 3,
    "beep_frequency": 1000
  }
}
```

### Example 5: Voicemail Deposit

Leave a voicemail with DTMF confirmation.

```json
{
  "name": "Voicemail Deposit",
  "description": "Call voicemail, leave message, confirm with #",
  "timeout": 45,

  "caller": {
    "account": "ext6004",
    "uri": "sip:*986003@192.168.10.10",
    "timeout": 15,
    "actions": [
      {"action": "wait", "seconds": 5},
      {"action": "play_audio", "file": "test_audio/test_message.wav"},
      {"action": "wait", "seconds": 1},
      {"action": "send_dtmf", "digits": "#"},
      {"action": "wait", "seconds": 2},
      {"action": "hangup", "code": 200}
    ]
  },

  "receiver": {
    "account": "ext6003",
    "auto_answer": false,
    "timeout": 30,
    "actions": []
  },

  "expect": {
    "connected": true
  }
}
```

### Example 6: Conference Bridge Test

Join a conference and verify audio.

```json
{
  "name": "Conference Bridge Test",
  "description": "Both parties join conference, verify audio path",
  "timeout": 40,

  "caller": {
    "account": "ext6004",
    "uri": "sip:800@192.168.10.10",
    "timeout": 15,
    "actions": [
      {"action": "wait", "seconds": 3},
      {"action": "send_dtmf", "digits": "1234#"},
      {"action": "wait", "seconds": 2},
      {"action": "play_audio", "file": "test_audio/three_beeps.wav"},
      {"action": "wait", "seconds": 5},
      {"action": "hangup", "code": 200}
    ]
  },

  "receiver": {
    "account": "ext6003",
    "auto_answer": false,
    "timeout": 20,
    "actions": [
      {"action": "record_audio", "file": "recordings/conference.wav"}
    ]
  },

  "expect": {
    "connected": true
  }
}
```

### Example 7: Call Forward Test

Verify call forwarding is working.

```json
{
  "name": "Call Forward Test",
  "description": "Call ext6003 which forwards to ext6005",
  "timeout": 30,

  "caller": {
    "account": "ext6004",
    "uri": "sip:6003@192.168.10.10",
    "timeout": 15,
    "actions": [
      {"action": "wait", "seconds": 1},
      {"action": "send_dtmf", "digits": "999"},
      {"action": "wait", "seconds": 2},
      {"action": "hangup", "code": 200}
    ]
  },

  "receiver": {
    "account": "ext6005",
    "auto_answer": true,
    "timeout": 20,
    "actions": [
      {"action": "expect_dtmf", "pattern": "999", "timeout": 10}
    ]
  },

  "expect": {
    "connected": true
  }
}
```

### Example 8: Hold and Resume Test

Test call hold functionality with audio verification.

```json
{
  "name": "Hold Music Test",
  "description": "Verify hold music is played when call is held",
  "timeout": 35,

  "caller": {
    "account": "ext6004",
    "uri": "sip:6003@192.168.10.10",
    "timeout": 10,
    "actions": [
      {"action": "wait", "seconds": 2},
      {"action": "record_audio", "file": "recordings/hold_music.wav"},
      {"action": "wait", "seconds": 8},
      {"action": "hangup", "code": 200}
    ]
  },

  "receiver": {
    "account": "ext6003",
    "auto_answer": true,
    "timeout": 15,
    "actions": [
      {"action": "wait", "seconds": 3}
    ]
  },

  "expect": {
    "connected": true
  }
}
```

## Creating Test Audio Files

Generate test tones with ffmpeg:

```bash
# Single 1kHz beep (0.3 seconds)
ffmpeg -f lavfi -i "sine=frequency=1000:duration=0.3" -ar 8000 beep.wav

# Three beeps with gaps
ffmpeg -f lavfi -i "sine=frequency=1000:duration=0.3" \
       -f lavfi -i "anullsrc=r=8000:cl=mono" \
       -filter_complex "[0][1][0][1][0]concat=n=5:v=0:a=1,atrim=0:1.5" \
       -ar 8000 three_beeps.wav

# DTMF tone sequence
ffmpeg -f lavfi -i "sine=frequency=697:duration=0.1,sine=frequency=1209:duration=0.1" \
       -filter_complex "amix=inputs=2" dtmf_1.wav
```

## Configuration Tips

### Beep Detection Settings

In your `config.json`, tune beep detection:

```json
{
  "beep_detection": {
    "min_level_db": -40,
    "min_duration_sec": 0.1,
    "max_duration_sec": 1.0,
    "target_freq_hz": 1000,
    "freq_tolerance_hz": 100,
    "gap_duration_sec": 0.05
  }
}
```

| Setting | Description |
|---------|-------------|
| `min_level_db` | Minimum signal level to detect (lower = more sensitive) |
| `min_duration_sec` | Minimum beep length |
| `max_duration_sec` | Maximum beep length |
| `target_freq_hz` | Expected frequency (0 = any) |
| `freq_tolerance_hz` | Allowed frequency deviation |
| `gap_duration_sec` | Minimum silence between beeps |

### Account Configuration

Ensure accounts in `config.json` match your PBX:

```json
{
  "accounts": [
    {
      "id": "ext6004",
      "username": "6004",
      "password": "secret",
      "server": "192.168.10.10",
      "port": 5060,
      "transport": "udp",
      "enabled": true
    }
  ]
}
```

## Troubleshooting

### Test times out waiting for call to connect

- Verify account credentials are correct
- Check PBX is reachable
- Ensure destination extension exists
- Check firewall allows SIP (5060) and RTP (10000-20000)

### DTMF not received

- Increase wait time before sending DTMF
- Verify RFC2833 is enabled on both endpoints
- Check DTMF mode matches PBX configuration

### Beeps not detected

- Lower `min_level_db` threshold (try -50 or -60)
- Verify audio file plays correctly
- Check recording contains audio (not silence)
- Adjust `min_duration_sec` to match your beep length

### RTP port conflicts

- The utility auto-selects RTP ports
- Ensure no other VoIP applications are running
- Check `rtp_port_start` and `rtp_port_count` in config

## Running Multiple Tests

### Using the Test Suite Runner

A comprehensive test suite is provided with a full-featured test runner script:

```bash
# Run all tests
./tests/run_test_suite.sh

# Run specific tests by number
./tests/run_test_suite.sh 01 04 07

# Quick mode (tests 01-05 only)
./tests/run_test_suite.sh --quick

# Verbose output
./tests/run_test_suite.sh --verbose

# JSON output for CI integration
./tests/run_test_suite.sh --json

# Stop on first failure
./tests/run_test_suite.sh --stop-on-fail

# List available tests
./tests/run_test_suite.sh --list
```

### Pre-built Test Suite

The `tests/` directory contains 12+ comprehensive tests covering:

| Test | Feature Tested |
|------|----------------|
| 01_basic_connection | SIP call connect/disconnect |
| 02_dtmf_send_receive | DTMF transmission verification |
| 03_dtmf_bidirectional | Bidirectional DTMF |
| 04_audio_playback_beep_detect | Audio playback with beep detection |
| 05_single_beep_detect | Single beep detection accuracy |
| 06_multiple_frequencies | Multi-frequency tone detection |
| 07_complex_flow_dtmf_then_audio | Complex scripted call flows |
| 08_expect_beeps_then_dtmf | Sequential verification |
| 09_bidirectional_audio | Full-duplex audio |
| 10_long_duration_call | Call stability (30+ seconds) |
| 11_rapid_dtmf_sequence | Complete DTMF keypad test |
| 12_silence_detection | False positive rejection |

### Simple Test Loop

For basic sequential testing without the runner:

```bash
#!/bin/bash
for test in tests/*.json; do
    echo "Running: $test"
    ./voip-utility -c config.json test -f "$test"
    if [ $? -ne 0 ]; then
        echo "FAILED: $test"
        exit 1
    fi
done
echo "All tests passed!"
```

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Test passed |
| 1 | Test failed or error |

## Best Practices

1. **Start simple** - Test basic call connectivity before complex scenarios
2. **Add delays** - Use `wait` actions to allow time for call setup and audio processing
3. **Use unique recordings** - Give each test its own recording filename
4. **Test both directions** - Audio may work one way but not the other
5. **Verify manually first** - Ensure your scenario works with softphones before automating
6. **Check logs** - Use `-v` flag for verbose output when debugging
