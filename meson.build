project('minidisc', ['c', 'cpp'],
  default_options: ['c_std=gnu11', 'cpp_std=c++11', 'werror=true'])

libusb = dependency('libusb-1.0')
gcrypt = dependency('libgcrypt')
glib = dependency('glib-2.0')
id3tag = dependency('id3tag')
taglib = dependency('taglib')
jsonc = dependency('json-c')
mad = dependency('mad', required: false)

libusbmd_sources = [
  'libusbmd/libusbmd.c',
]

libusbmd_headers = [
  'libusbmd/libusbmd.h',
]

install_headers(libusbmd_headers, subdir: 'libusbmd')

libusbmd = library('usbmd',
  libusbmd_sources,
  soversion: '0',
  version: '0.0.1',
  install: true,
)

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
  'libnetmd/recv.c',
  'libnetmd/timeformat.c',
  'libnetmd/groups.c',
  'libnetmd/query.c',
  'libnetmd/descriptor.c',
]

# TODO: Make the "surface area" of publicly exposed headers smaller,
# so that internal changes don't cause API/ABI breakages.
libnetmd_headers = [
  'libnetmd/common.h',
  'libnetmd/const.h',
  'libnetmd/error.h',
  'libnetmd/libnetmd.h',
  'libnetmd/log.h',
  'libnetmd/netmd_dev.h',
  'libnetmd/playercontrol.h',
  'libnetmd/secure.h',
  'libnetmd/trackinformation.h',
  'libnetmd/utils.h',
  'libnetmd/groups.h',
  'libnetmd/query.h',
  'libnetmd/descriptor.h',
]

install_headers(libnetmd_headers, subdir: 'libnetmd')

libnetmd = library('netmd',
  libnetmd_sources,
  dependencies: [libusb, gcrypt],
  link_with: [libusbmd],
  include_directories: ['libusbmd'],
  soversion: '0',
  version: '0.0.1',
  install: true,
)

executable('libnetmd_test_query',
  'libnetmd/test_query.c',
  link_with: [libnetmd],
  include_directories: ['libnetmd'],
)

netmdcli_sources = [
  'netmdcli/netmdcli.c',
  'netmdcli/cmds.c',
]

executable('netmdcli',
  netmdcli_sources,
  dependencies: [jsonc],
  link_with: [libusbmd, libnetmd],
  include_directories: ['libusbmd', 'libnetmd'],
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
  link_with: [libusbmd],
  soversion: '0',
  version: '0.0.1',
  install: true,
)

himdcli_sources = [
  'himdcli/himdcli.c',
]

executable('himdcli',
  himdcli_sources,
  dependencies: [glib, jsonc],
  link_with: [libusbmd, libhimd],
  include_directories: ['libusbmd', 'libhimd'],
  install: true,
)

if get_option('target_os') == 'linux'
  install_data(
    sources: 'devicedb/udev/60-minidisc.rules',
    install_dir: 'lib/udev/rules.d',
  )
endif

if get_option('with_gui')
  qt5 = import('qt5')
  qt5_dep = dependency('qt5', modules: ['Core', 'Gui', 'Widgets', 'DBus'])
  additional_dependencies = []

  qhimdtransfer_sources = [
    'qhimdtransfer/main.cpp',
    'qhimdtransfer/qhimdaboutdialog.cpp',
    'qhimdtransfer/qhimddetection.cpp',
    'qhimdtransfer/qhimdmainwindow.cpp',
    'qhimdtransfer/qhimduploaddialog.cpp',
    'qhimdtransfer/qmddevice.cpp',
    'qhimdtransfer/qmdmodel.cpp',
    'qhimdtransfer/qmdtrack.cpp',
    'qhimdtransfer/qmdutil.cpp',
    'qhimdtransfer/wavefilewriter.cpp',
    'qhimdtransfer/editmetadatadialog.cpp',
  ]

  if get_option('target_os') == 'linux'
    qhimdtransfer_sources += 'qhimdtransfer/qhimdlinuxdetection.cpp'
    qhimdtransfer_sources += qt5.preprocess(moc_headers: [
      'qhimdtransfer/qhimdlinuxdetection.h',
    ])
  elif get_option('target_os') == 'windows'
    qhimdtransfer_sources += 'qhimdtransfer/qhimdwindetection.cpp'
    compiler = meson.get_compiler('c')
    additional_dependencies += compiler.find_library('cfgmgr32')
    windows = import('windows')
    qhimdtransfer_sources += windows.compile_resources('qhimdtransfer/qhimdtransfer.rc')
  elif get_option('target_os') == 'macos'
    add_languages('objc')
    qhimdtransfer_sources += 'qhimdtransfer/qhimdmacdetection.mm'
    additional_dependencies += dependency('appleframeworks', modules: ['IOKit', 'DiskArbitration', 'Foundation'])
  else
    qhimdtransfer_sources += 'qhimdtransfer/qhimddummydetection.cpp'
  endif

  qhimdtransfer_sources += qt5.preprocess(ui_files: [
    'qhimdtransfer/qhimdaboutdialog.ui',
    'qhimdtransfer/qhimdmainwindow.ui',
    'qhimdtransfer/qhimduploaddialog.ui',
    'qhimdtransfer/editmetadatadialog.ui',
  ], moc_headers: [
    'qhimdtransfer/qhimdaboutdialog.h',
    'qhimdtransfer/qhimdmainwindow.h',
    'qhimdtransfer/qhimduploaddialog.h',
    'qhimdtransfer/editmetadatadialog.h',

    'qhimdtransfer/qhimddetection.h',
    'qhimdtransfer/qmdmodel.h',
    'qhimdtransfer/qmddevice.h',
  ], qresources: [
    'qhimdtransfer/icons.qrc',
  ])

  qhimdtransfer_sources += qt5.compile_translations(ts_files: [
    'qhimdtransfer/qhimdtransfer_ar.ts',
    'qhimdtransfer/qhimdtransfer_br.ts',
    'qhimdtransfer/qhimdtransfer_cs.ts',
    'qhimdtransfer/qhimdtransfer_da.ts',
    'qhimdtransfer/qhimdtransfer_de.ts',
    'qhimdtransfer/qhimdtransfer_el.ts',
    'qhimdtransfer/qhimdtransfer_es.ts',
    'qhimdtransfer/qhimdtransfer_fi.ts',
    'qhimdtransfer/qhimdtransfer_fr.ts',
    'qhimdtransfer/qhimdtransfer_it.ts',
    'qhimdtransfer/qhimdtransfer_ja.ts',
    'qhimdtransfer/qhimdtransfer_nb.ts',
    'qhimdtransfer/qhimdtransfer_nl.ts',
    'qhimdtransfer/qhimdtransfer_pl.ts',
    'qhimdtransfer/qhimdtransfer_pt.ts',
    'qhimdtransfer/qhimdtransfer_ru.ts',
    'qhimdtransfer/qhimdtransfer_sv.ts',
    'qhimdtransfer/qhimdtransfer_tr.ts',
    'qhimdtransfer/qhimdtransfer_uk.ts',
  ])

  # TODO: Fallback to reading from VERSION and stuff
  qhimdtransfer_config = configuration_data()
  qhimdtransfer_config_h = vcs_tag(
    command: ['git', 'describe', '--tags'],
    input: 'qhimdtransfer/qhimdtransfer_config.h.in',
    output: 'qhimdtransfer_config.h',
  )

  executable('qhimdtransfer',
    qhimdtransfer_sources,
    dependencies: [qt5_dep, libusb, taglib] + additional_dependencies,
    link_with: [libusbmd, libnetmd, libhimd],
    # Workaround for taglib: class TagLib::FileRef::FileTypeResolver has virtual functions and accessible non-virtual destructor
    cpp_args: ['-Wno-non-virtual-dtor'],
    include_directories: ['libusbmd', 'libnetmd', 'libhimd', 'qhimdtransfer'],
    gui_app: true,
    install: true,
  )
endif
