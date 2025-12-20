project('voip-utility', 'c',
  version : '0.1.0',
  license : 'MIT',
  default_options : [
    'c_std=c11',
    'warning_level=2',
    'werror=false',
    'b_lto=false',
  ]
)

# Compiler setup
cc = meson.get_compiler('c')
add_project_arguments('-D_GNU_SOURCE', language : 'c')
add_project_arguments('-D_POSIX_C_SOURCE=200809L', language : 'c')

# Dependencies

# OpenSSL - required by PJSIP
openssl_dep = dependency('openssl', required : true)

# ALSA - required by PJSIP audio
alsa_dep = dependency('alsa', required : true)

# UUID
uuid_dep = dependency('uuid', required : true)

# libsrtp2
srtp2_dep = dependency('libsrtp2', required : true)

# Opus codec (optional)
opus_dep = dependency('opus', required : false)

# WebRTC audio processing (optional)
webrtc_dep = dependency('webrtc-audio-processing', required : false)

# PJSIP bundled libraries (built from source, static)
# These are in /usr/local/lib with architecture suffix
pjsip_libdir = '/usr/local/lib'
arch_suffix = '-x86_64-pc-linux-gnu'

# PJSIP core libraries
pjsua2_lib = cc.find_library('pjsua2' + arch_suffix, dirs : pjsip_libdir, required : false)
pjsua_lib = cc.find_library('pjsua' + arch_suffix, dirs : pjsip_libdir, required : true)
pjsip_ua_lib = cc.find_library('pjsip-ua' + arch_suffix, dirs : pjsip_libdir, required : true)
pjsip_simple_lib = cc.find_library('pjsip-simple' + arch_suffix, dirs : pjsip_libdir, required : true)
pjsip_lib = cc.find_library('pjsip' + arch_suffix, dirs : pjsip_libdir, required : true)
pjmedia_codec_lib = cc.find_library('pjmedia-codec' + arch_suffix, dirs : pjsip_libdir, required : true)
pjmedia_videodev_lib = cc.find_library('pjmedia-videodev' + arch_suffix, dirs : pjsip_libdir, required : true)
pjmedia_audiodev_lib = cc.find_library('pjmedia-audiodev' + arch_suffix, dirs : pjsip_libdir, required : true)
pjmedia_lib = cc.find_library('pjmedia' + arch_suffix, dirs : pjsip_libdir, required : true)
pjnath_lib = cc.find_library('pjnath' + arch_suffix, dirs : pjsip_libdir, required : true)
pjlib_util_lib = cc.find_library('pjlib-util' + arch_suffix, dirs : pjsip_libdir, required : true)
pj_lib = cc.find_library('pj' + arch_suffix, dirs : pjsip_libdir, required : true)

# PJSIP bundled third-party libraries
gsmcodec_lib = cc.find_library('gsmcodec' + arch_suffix, dirs : pjsip_libdir, required : true)
ilbccodec_lib = cc.find_library('ilbccodec' + arch_suffix, dirs : pjsip_libdir, required : true)
speex_lib = cc.find_library('speex' + arch_suffix, dirs : pjsip_libdir, required : true)
resample_lib = cc.find_library('resample' + arch_suffix, dirs : pjsip_libdir, required : true)
g7221codec_lib = cc.find_library('g7221codec' + arch_suffix, dirs : pjsip_libdir, required : true)
webrtc_bundled_lib = cc.find_library('webrtc' + arch_suffix, dirs : pjsip_libdir, required : false)
yuv_lib = cc.find_library('yuv' + arch_suffix, dirs : pjsip_libdir, required : false)
srtp_bundled_lib = cc.find_library('srtp' + arch_suffix, dirs : pjsip_libdir, required : false)

# Combine all PJSIP libraries in correct order
pjsip_libs = []
if pjsua2_lib.found()
  pjsip_libs += pjsua2_lib
endif
pjsip_libs += [
  pjsua_lib,
  pjsip_ua_lib,
  pjsip_simple_lib,
  pjsip_lib,
  pjmedia_codec_lib,
  pjmedia_videodev_lib,
  pjmedia_audiodev_lib,
  pjmedia_lib,
  pjnath_lib,
  pjlib_util_lib,
  pj_lib,
  gsmcodec_lib,
  ilbccodec_lib,
  speex_lib,
  resample_lib,
  g7221codec_lib,
]
if webrtc_bundled_lib.found()
  pjsip_libs += webrtc_bundled_lib
endif
if yuv_lib.found()
  pjsip_libs += yuv_lib
endif
if srtp_bundled_lib.found()
  pjsip_libs += srtp_bundled_lib
endif

pjsip_dep = declare_dependency(
  dependencies : pjsip_libs,
  include_directories : include_directories('/usr/local/include'),
)

# cJSON - try system first, then subproject
cjson_dep = dependency('libcjson', required : false)
if not cjson_dep.found()
  cjson_lib = cc.find_library('cjson', required : false)
  if cjson_lib.found()
    cjson_dep = cjson_lib
  else
    cjson_proj = subproject('cjson')
    cjson_dep = cjson_proj.get_variable('cjson_dep')
  endif
endif

# FFTW3 (single precision)
fftw_dep = dependency('fftw3f', required : false)
if not fftw_dep.found()
  fftw_dep = cc.find_library('fftw3f', required : true)
endif

# Math library
m_dep = cc.find_library('m', required : true)

# pthreads
threads_dep = dependency('threads')

# C++ standard library (required by pjsua2)
stdc_lib = cc.find_library('stdc++', required : false)

# Source files
src_util = files(
  'src/util/error.c',
  'src/util/log.c',
  'src/util/json_output.c',
  'src/util/time_util.c',
)

src_config = files(
  'src/config/config.c',
)

src_core = files(
  'src/core/sip_ua.c',
  'src/core/account.c',
  'src/core/call.c',
  'src/core/media.c',
  'src/core/dtmf.c',
)

src_audio = files(
  'src/audio/analyzer.c',
  'src/audio/beep_detector.c',
)

src_test_engine = files(
  'src/test/test_engine.c',
  'src/test/test_parser.c',
  'src/test/event_system.c',
  'src/test/action_executor.c',
  'src/test/condition_eval.c',
)

src_cli = files(
  'src/cli/cli.c',
  'src/cli/cmd_register.c',
  'src/cli/cmd_call.c',
  'src/cli/cmd_receive.c',
  'src/cli/cmd_test.c',
  'src/cli/cmd_interactive.c',
  'src/cli/cmd_analyze.c',
)

all_sources = [
  src_util,
  src_config,
  src_core,
  src_audio,
  src_test_engine,
  src_cli,
  'src/main.c',
]

inc = include_directories('src')

# Collect all deps
all_deps = [
  pjsip_dep,
  openssl_dep,
  alsa_dep,
  uuid_dep,
  srtp2_dep,
  cjson_dep,
  fftw_dep,
  m_dep,
  threads_dep,
]
if opus_dep.found()
  all_deps += opus_dep
endif
if webrtc_dep.found()
  all_deps += webrtc_dep
endif
if stdc_lib.found()
  all_deps += stdc_lib
endif

# Main executable
voip_utility = executable('voip-utility',
  all_sources,
  include_directories : inc,
  dependencies : all_deps,
  install : true,
)

# Unit tests
if get_option('tests')
  subdir('tests')
endif
