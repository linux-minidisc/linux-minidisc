project('minidisc', 'c')

libusb = dependency('libusb-1.0')
gcrypt = dependency('libgcrypt')
glib = dependency('glib-2.0')
id3tag = dependency('id3tag')
jsonc = dependency('json-c')
mad = dependency('mad', required: false)

libnetmd_sources = [
  'libnetmd/libnetmd.c',
  'libnetmd/common.c',
  'libnetmd/error.c',
  'libnetmd/log.c',
  'libnetmd/netmd_dev.c',
  'libnetmd/playercontrol.c',
  'libnetmd/secure.c',
  'libnetmd/trackinformation.c',
  'libnetmd/utils.c',
  'libnetmd/send.c',
]

# TODO: Make the "surface area" of publicly exposed headers smaller,
# so that internal changes don't cause API/ABI breakages.
libnetmd_headers = [
  'libnetmd/common.h',
  'libnetmd/const.h',
  'libnetmd/error.h',
  'libnetmd/libnetmd_extended.h',
  'libnetmd/libnetmd.h',
  'libnetmd/log.h',
  'libnetmd/netmd_dev.h',
  'libnetmd/netmd_dev_ids.h',
  'libnetmd/playercontrol.h',
  'libnetmd/secure.h',
  'libnetmd/trackinformation.h',
  'libnetmd/utils.h',
]

install_headers(libnetmd_headers, subdir: 'libnetmd')

libnetmd = library('netmd',
  libnetmd_sources,
  dependencies: [libusb, gcrypt],
  install: true,
)

netmdcli_sources = [
  'netmdcli/netmdcli.c',
]

executable('netmdcli',
  netmdcli_sources,
  dependencies: [jsonc],
  link_with: [libnetmd],
  include_directories: 'libnetmd',
  install: true,
)

libhimd_sources = [
  'libhimd/codecinfo.c',
  'libhimd/encryption.c',
  'libhimd/himd.c',
  'libhimd/mdstream.c',
  'libhimd/trackindex.c',
  'libhimd/sony_oma.c',
  'libhimd/frag.c',
  'libhimd/mp3tools.c',
  'libhimd/mp3download.c',
]

himd_config = configuration_data()
himd_config.set('CONFIG_WITH_MAD', mad.found())
himd_config.set('CONFIG_WITH_GCRYPT', gcrypt.found())
himd_config_h = configure_file(
  input: 'libhimd/himd_config.h.in',
  output: 'himd_config.h',
  configuration : himd_config)

libhimd_headers = [
  'libhimd/codecinfo.h',
  'libhimd/himd.h',
  'libhimd/sony_oma.h',
  himd_config_h,
]

install_headers(libhimd_headers, subdir: 'libhimd')

libhimd = library('himd',
  libhimd_sources,
  dependencies: [glib, id3tag, gcrypt, mad],
  install: true,
)

himdcli_sources = [
  'himdcli/himdcli.c',
]

executable('himdcli',
  himdcli_sources,
  dependencies: [glib, jsonc],
  link_with: [libhimd],
  include_directories: 'libhimd',
  install: true,
)

# TODO: Maybe install only on Linux, and ignore for MSVC/MinGW/macOS/*BSD
install_data(
  sources: 'devicedb/udev/60-minidisc.rules',
  install_dir: 'lib/udev/rules.d',
)
