project('atlantis-little-helper', 'cpp',
  version: '2.8.0',
  default_options: ['cpp_std=c++17', 'warning_level=0'])

wx_dep = dependency('wxwidgets')

root_inc = include_directories('.')
src_inc = include_directories('src')
tests_inc = include_directories('tests')

ah_sources = files(
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
if py_opt.allowed()
  cc = meson.get_compiler('cpp')
  if not cc.has_header('Python.h')
    error('python option enabled but Python.h was not found. This project\'s embedded ' +
          'Python support only works against Python 2 headers, which are not available ' +
          'on modern systems (see README) - matches the pre-existing --with-python limitation.')
  endif
  ah_extra_args += ['-DHAVE_PYTHON']
endif

executable('ah', ah_sources,
  include_directories: [root_inc, src_inc],
  cpp_args: ah_extra_args,
  link_args: ['-s'],
  dependencies: [wx_dep])

parser_tests_exe = executable('parser-tests', test_sources + shared_sources,
  include_directories: [root_inc, src_inc, tests_inc],
  cpp_args: ['-DALH_TESTING'],
  dependencies: [wx_dep])

# tests reference fixtures via repo-root-relative paths (e.g.
# tests/fixtures/structures.rep), so the test binary must run with the
# source root as its working directory, matching `make test`'s behaviour.
test('parser-tests', parser_tests_exe, workdir: meson.project_source_root())
