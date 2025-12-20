# Unit tests

# Test dependencies
test_deps = [
  cjson_dep,
  fftw_dep,
  m_dep,
  threads_dep,
]

# Helper library for tests (exclude main.c and PJSIP-dependent code for now)
test_lib_sources = [
  '../src/util/error.c',
  '../src/util/log.c',
  '../src/util/time_util.c',
  '../src/util/json_output.c',
  '../src/config/config.c',
  '../src/audio/analyzer.c',
  '../src/audio/beep_detector.c',
  '../src/audio/recorder.c',
]

# Note: Unit tests would be added here once we have a test framework
# For now, we'll skip building tests without PJSIP
