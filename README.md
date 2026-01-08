# voip-utility

A command-line SIP VoIP testing utility built on PJSIP/PJSUA for automated telephony testing, audio analysis, and call verification.

## Features

- **SIP Registration** - Register accounts with SIP servers/PBX systems
- **Outbound Calls** - Make calls with DTMF, audio playback, and recording
- **Inbound Calls** - Receive and auto-answer calls
- **DTMF Testing** - Send and verify DTMF digit transmission
- **Audio Analysis** - FFT-based frequency detection and beep counting
- **Automated Testing** - JSON-defined test scenarios with pass/fail verification
- **WAV Recording** - Record call audio for analysis

## Building

### Quick Start (Ubuntu 22.04+ / Debian 12+)

```bash
# Install dependencies
sudo apt install build-essential meson ninja-build pkg-config
sudo apt install libssl-dev libfftw3-dev libcjson-dev libasound2-dev \
                 uuid-dev libsrtp2-dev libopus-dev

# PJSIP must be built from source - see BUILD.md

# Build voip-utility
meson setup build
ninja -C build
```

The binary will be at `build/voip-utility`.

### Dependencies

| Dependency | Version | Required | Notes |
|------------|---------|----------|-------|
| PJSIP/PJSUA | 2.x | Yes | Must build from source |
| **OpenSSL** | **3.0+** | Yes | **Critical version requirement** |
| ALSA | 1.x | Yes | Audio devices |
| FFTW3 | 3.x | Yes | Single precision (`fftw3f`) |
| cJSON | 1.x | Yes | JSON parsing |
| libsrtp2 | 2.x | Yes | SRTP support |

### OpenSSL Version Requirement

**Important:** This project requires OpenSSL 3.0 or later. The PJSIP libraries must be built against the same OpenSSL version.

If you see this error during linking:
```
undefined reference to `SSL_get1_peer_certificate'
```

Your OpenSSL version is too old (1.1.x). See [BUILD.md](BUILD.md) for solutions.

### Detailed Build Instructions

See **[BUILD.md](BUILD.md)** for:
- Complete dependency installation for all platforms
- PJSIP build instructions
- Troubleshooting common build errors
- Docker build environment
- Cross-compilation notes

## Configuration

Create a JSON configuration file with your SIP accounts:

```json
{
  "accounts": [
    {
      "id": "myphone",
      "username": "1001",
      "password": "secret",
      "server": "pbx.example.com",
      "port": 5060,
      "transport": "udp",
      "enabled": true
    }
  ],
  "beep_detection": {
    "min_level_db": -40,
    "min_duration_sec": 0.1,
    "max_duration_sec": 1.0,
    "freq_tolerance_hz": 50
  },
  "recordings_dir": "./recordings"
}
```

See `examples/config.json` for a complete example.

## Usage

### Register with SIP Server

Test account registration:

```bash
./voip-utility -c config.json register -a myphone
```

Options:
- `-a, --account` - Account ID from config
- `-t, --timeout` - Registration timeout (default: 30s)

### Make a Call

Place an outbound call:

```bash
# Basic call
./voip-utility -c config.json call -a myphone -u sip:1002@pbx.example.com

# Call with DTMF
./voip-utility -c config.json call -a myphone -u sip:1002@pbx.example.com -d "123#"

# Call with audio playback
./voip-utility -c config.json call -a myphone -u sip:1002@pbx.example.com -p audio.wav

# Call with recording
./voip-utility -c config.json call -a myphone -u sip:1002@pbx.example.com -r recording.wav

# Auto-hangup after duration
./voip-utility -c config.json call -a myphone -u sip:1002@pbx.example.com --hangup-after 10
```

Options:
- `-a, --account` - Account ID to use
- `-u, --uri` - SIP URI to call
- `-d, --dtmf` - DTMF digits to send after connect
- `-p, --play` - WAV file to play
- `-r, --record` - WAV file to record to
- `--hangup-after` - Hangup after N seconds
- `-t, --timeout` - Call timeout (default: 60s)

### Receive Calls

Wait for and answer incoming calls:

```bash
# Answer one call
./voip-utility -c config.json receive -a myphone --auto-answer

# Answer with recording
./voip-utility -c config.json receive -a myphone --auto-answer -r recording.wav

# Wait for specific duration
./voip-utility -c config.json receive -a myphone --auto-answer --timeout 120
```

Options:
- `-a, --account` - Account ID to register
- `--auto-answer` - Automatically answer calls
- `--answer-delay` - Delay before answering (ms)
- `-r, --record` - WAV file to record to
- `-p, --play` - WAV file to play when answered
- `-t, --timeout` - How long to wait for calls

### Analyze Audio

Analyze WAV files for frequencies and beeps:

```bash
# Basic analysis
./voip-utility -c config.json analyze recording.wav

# Detect beeps
./voip-utility -c config.json analyze recording.wav --detect-beeps

# Verbose output
./voip-utility -c config.json -v analyze recording.wav
```

### Run Automated Tests

Execute test scenarios defined in JSON:

```bash
./voip-utility -c config.json test -f tests/my_test.json
```

Test definition example:
```json
{
  "name": "DTMF Test",
  "timeout": 30,
  "caller": {
    "account": "ext1",
    "uri": "sip:1002@pbx.example.com",
    "timeout": 10,
    "actions": [
      {"action": "wait", "seconds": 1},
      {"action": "send_dtmf", "digits": "12345"},
      {"action": "wait", "seconds": 2},
      {"action": "hangup"}
    ]
  },
  "receiver": {
    "account": "ext2",
    "auto_answer": true,
    "actions": [
      {"action": "expect_dtmf", "pattern": "12345", "timeout": 10}
    ]
  },
  "expect": {
    "connected": true
  }
}
```

See `docs/AUTOMATED_TESTING.md` for complete documentation.

## Examples

### Test Call Connectivity

```bash
# Terminal 1: Start receiver
./voip-utility -c config.json receive -a ext1 --auto-answer --timeout 30

# Terminal 2: Make call
./voip-utility -c config.json call -a ext2 -u sip:1001@pbx.local --hangup-after 5
```

### Test DTMF Through PBX

```bash
./voip-utility -c config.json test -f examples/dtmf_test.json
```

### Test Audio Path

```bash
./voip-utility -c config.json test -f examples/audio_test.json
```

### Navigate IVR Menu

```bash
./voip-utility -c config.json call -a myphone -u sip:8000@pbx.local \
  -d "1" --dtmf-delay 3000 -d "2" --hangup-after 15
```

### Record Voicemail Greeting

```bash
./voip-utility -c config.json call -a myphone -u sip:*98@pbx.local \
  -r voicemail_greeting.wav --hangup-after 30
```

## Test Actions Reference

| Action | Description | Parameters |
|--------|-------------|------------|
| `wait` | Pause execution | `seconds`: duration |
| `send_dtmf` | Send DTMF tones | `digits`: string (0-9,*,#,A-D) |
| `expect_dtmf` | Verify received DTMF | `pattern`: string, `timeout`: seconds |
| `play_audio` | Play WAV file | `file`: path to WAV |
| `record_audio` | Record to WAV | `file`: path to save WAV |
| `expect_beeps` | Detect beeps in audio | `count`: number, `frequency`: Hz, `timeout`: seconds |
| `hangup` | End call | `code`: SIP code (default 200) |

## Automated Test Suite

A comprehensive test suite is provided in the `tests/` directory that exercises all voip-utility features using a self-testing approach (the utility tests itself using two SIP extensions).

### Running the Test Suite

```bash
# Run all tests
./tests/run_test_suite.sh

# Run specific tests by number
./tests/run_test_suite.sh 01 04 07

# Quick tests only (tests 01-05)
./tests/run_test_suite.sh --quick

# Verbose output (shows detailed PJSIP logs)
./tests/run_test_suite.sh --verbose

# JSON output (for CI integration)
./tests/run_test_suite.sh --json

# Stop on first failure
./tests/run_test_suite.sh --stop-on-fail

# List available tests
./tests/run_test_suite.sh --list
```

### Test Coverage

| Test | Description |
|------|-------------|
| 01_basic_connection | Basic SIP call connect/disconnect between two extensions |
| 02_dtmf_send_receive | Caller sends DTMF digits, receiver verifies pattern |
| 03_dtmf_bidirectional | Both sides send and receive DTMF simultaneously |
| 04_audio_playback_beep_detect | Play audio file on caller, detect beeps on receiver |
| 05_single_beep_detect | Single beep detection accuracy verification |
| 06_multiple_frequencies | Detect tones at different frequencies (400Hz, 1000Hz, 1500Hz) |
| 07_complex_flow_dtmf_then_audio | Scriptable flow: DTMF sequence followed by audio playback |
| 08_expect_beeps_then_dtmf | Receiver detects beeps, then expects DTMF |
| 09_bidirectional_audio | Full-duplex audio testing (both sides play and record) |
| 10_long_duration_call | Extended call stability test (30+ seconds) |
| 11_rapid_dtmf_sequence | Full DTMF sequence verification (0-9, *, #) |
| 12_silence_detection | Verify no false positive beep detection on silence |

### Prerequisites

The test suite requires:
- Two SIP accounts configured in `examples/config.json` (default: ext6010, ext6011)
- SIP server accessible at the configured address (default: 192.168.10.10)
- Test audio files in `test_audio/` directory
- `recordings/` directory for test recordings (created automatically)

### Creating Custom Tests

Test files are JSON documents that define caller and receiver behavior:

```json
{
  "name": "My Custom Test",
  "description": "Description of what this test verifies",
  "timeout": 30,
  "caller": {
    "account": "ext6010",
    "uri": "sip:6011@192.168.10.10",
    "timeout": 15,
    "actions": [
      {"action": "wait", "seconds": 1},
      {"action": "send_dtmf", "digits": "123#"},
      {"action": "play_audio", "file": "test_audio/beep_1000hz.wav"},
      {"action": "wait", "seconds": 3},
      {"action": "hangup", "code": 200}
    ]
  },
  "receiver": {
    "account": "ext6011",
    "auto_answer": true,
    "timeout": 20,
    "actions": [
      {"action": "record_audio", "file": "recordings/my_test.wav"},
      {"action": "expect_dtmf", "pattern": "123#", "timeout": 10}
    ]
  },
  "expect": {
    "connected": true,
    "beep_count": 1,
    "beep_frequency": 1000
  }
}
```

### Test Expectations

The `expect` section defines pass/fail criteria:

| Field | Description |
|-------|-------------|
| `connected` | Verify call was successfully connected |
| `beep_count` | Number of beeps that should be detected in recording |
| `beep_frequency` | Expected frequency of detected beeps (Hz) |
| `dtmf_received` | DTMF pattern that should be received |

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success / Test passed |
| 1 | Error / Test failed |

## Troubleshooting

### Registration fails

- Verify server address and port
- Check username/password
- Ensure UDP port 5060 is not blocked
- Try with `-v` for verbose output

### No audio in calls

- Check RTP ports (default 4000+) are not blocked
- Verify codec compatibility with PBX
- Ensure WAV files are correct format (PCM, 8/16kHz, 16-bit mono)

### DTMF not received

- Add delay before sending DTMF (`wait` action)
- Verify RFC2833 is enabled on PBX
- Check DTMF mode configuration

### Beeps not detected

- Lower `min_level_db` in config (try -50 or -60)
- Verify recording contains audio (not silence)
- Adjust `min_duration_sec` to match beep length

## Project Structure

```
voip-utility/
├── src/
│   ├── main.c              # Entry point
│   ├── cli/                # Command-line interface
│   │   ├── cli.c           # Argument parsing
│   │   ├── cmd_register.c  # Register command
│   │   ├── cmd_call.c      # Call command
│   │   ├── cmd_receive.c   # Receive command
│   │   ├── cmd_analyze.c   # Analyze command
│   │   └── cmd_test.c      # Test command
│   ├── core/               # SIP/VoIP core
│   │   ├── sip_ua.c        # PJSUA wrapper
│   │   ├── account.c       # Account management
│   │   ├── call.c          # Call handling
│   │   ├── dtmf.c          # DTMF send/receive
│   │   └── media.c         # Audio playback/recording
│   ├── audio/              # Audio processing
│   │   ├── analyzer.c      # FFT frequency analysis
│   │   └── beep_detector.c # Beep detection
│   ├── config/             # Configuration
│   │   └── config.c        # JSON config parsing
│   ├── test/               # Test framework
│   │   ├── test_parser.c   # Test JSON parser
│   │   └── test_engine.c   # Test execution
│   └── util/               # Utilities
│       ├── error.c         # Error handling
│       ├── log.c           # Logging
│       └── time_util.c     # Time utilities
├── tests/                  # Automated test suite
│   ├── run_test_suite.sh   # Test runner script
│   ├── 01_basic_connection.json
│   ├── 02_dtmf_send_receive.json
│   ├── ...                 # Additional test cases
│   └── 12_silence_detection.json
├── examples/               # Example configs
├── docs/                   # Documentation
├── test_audio/             # Test audio files (WAV)
└── recordings/             # Call recordings output
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/my-feature`)
3. Commit your changes (`git commit -am 'Add new feature'`)
4. Push to the branch (`git push origin feature/my-feature`)
5. Open a Pull Request

### Development Setup

```bash
# Clone and build
git clone https://github.com/yourusername/voip-utility.git
cd voip-utility
meson setup build
ninja -C build

# Run test suite
./tests/run_test_suite.sh
```

### Code Style

- Use 4-space indentation
- Follow existing code patterns
- Add comments for complex logic
- Update documentation for new features
