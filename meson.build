project('atlantis-little-helper', 'cpp',
  version: '2.8.0',
  default_options: ['cpp_std=c++17', 'warning_level=0'])

if host_machine.system() == 'darwin'
  add_languages('objcpp', required: true, native: false)
endif

wx_dep = dependency('wxwidgets')

root_inc = include_directories('.')
src_inc = include_directories('src')
tests_inc = include_directories('tests')

ah_sources = []
if host_machine.system() == 'darwin'
  ah_sources += files('src/macos_appearance.mm')
endif
ah_sources += files(
  'src/cfgfile.cpp',
  'src/string_utils.cpp',
  'src/files.cpp',
  'src/objs.cpp',
  'src/data.cpp',
  'src/errs.cpp',
  'src/atlaparser.cpp',
  'src/ahapp.cpp',
  'src/ahframe.cpp',
  'src/consts_ah.cpp',
  'src/editpane.cpp',
  'src/editsframe.cpp',
  'src/extend.cpp',
  'src/extend_no.cpp',
  'src/flagsdlg.cpp',
  'src/hexfilterdlg.cpp',
  'src/listcoledit.cpp',
  'src/listpane.cpp',
  'src/mapframe.cpp',
  'src/mappane.cpp',
  'src/msgframe.cpp',
  'src/optionsdlg.cpp',
  'src/unitfilterdlg.cpp',
  'src/unitframe.cpp',
  'src/unitframefltr.cpp',
  'src/unitpane.cpp',
  'src/unitpanefltr.cpp',
  'src/unitsplitdlg.cpp',
  'src/utildlgs.cpp',
)

# Shared with the parser-tests target below: the parser/model/property layer,
# deliberately excluding the wx UI translation units.
shared_sources = files(
  'src/string_utils.cpp',
  'src/files.cpp',
  'src/cfgfile.cpp',
  'src/objs.cpp',
  'src/data.cpp',
  'src/errs.cpp',
  'src/consts_ah.cpp',
  'src/atlaparser.cpp',
)

test_sources = files(
  'tests/parser_regression_tests.cpp',
  'tests/property_regression_tests.cpp',
  'tests/model_regression_tests.cpp',
  'tests/game_data_helper_stub.cpp',
)

# HAVE_PYTHON is passed directly as a compile define rather than through a
# generated config.h: config.h's other macros (STDC_HEADERS, HAVE_STRDUP) are
# defined but never consumed anywhere in the C++ code, so there is nothing
# else worth regenerating a header for.
py_opt = get_option('python')
ah_extra_args = []
ah_deps = [wx_dep]
if py_opt.allowed()
  cc = meson.get_compiler('cpp')
  py_fail_msg = ('python option enabled but no working Python 2 installation was found ' +
                 '(Python.h identifying itself as PY_MAJOR_VERSION 2, plus a matching ' +
                 'libpythonX.Y, are both required). This project\'s embedded Python support ' +
                 'only works against Python 2, which is not available on modern systems ' +
                 '(see README) - matches the pre-existing --with-python limitation.')

  # A bare has_header('Python.h') check is not enough: a machine with only
  # Python 3 headers installed (the common case today) would satisfy it and
  # then fail later with a raw, confusing compile/link error - defeating the
  # point of failing fast here. Confirm it is genuinely Python 2 via
  # PY_MAJOR_VERSION, and confirm a matching library actually links, before
  # ever adding -DHAVE_PYTHON.
  python2_ok = cc.has_header('Python.h')
  py_major = ''
  if python2_ok
    py_major = cc.get_define('PY_MAJOR_VERSION', prefix: '#include <Python.h>')
    python2_ok = py_major == '2'
  endif
  py2_lib = []
  if python2_ok
    py_minor = cc.get_define('PY_MINOR_VERSION', prefix: '#include <Python.h>')
    py2_lib = cc.find_library('python' + py_major + '.' + py_minor, required: false)
    python2_ok = py2_lib.found()
  endif
  if not python2_ok
    error(py_fail_msg)
  endif

  ah_extra_args += ['-DHAVE_PYTHON']
  ah_deps += [py2_lib]
endif

executable('ah', ah_sources,
  include_directories: [root_inc, src_inc],
  cpp_args: ah_extra_args,
  dependencies: ah_deps)

parser_tests_exe = executable('parser-tests', test_sources + shared_sources,
  include_directories: [root_inc, src_inc, tests_inc],
  cpp_args: ['-DALH_TESTING'],
  dependencies: [wx_dep])

# tests reference fixtures via repo-root-relative paths (e.g.
# tests/fixtures/structures.rep), so the test binary must run with the
# source root as its working directory, matching `make test`'s behaviour.
test('parser-tests', parser_tests_exe, workdir: meson.project_source_root())
