alh
===

Atlantis Little Helper

Building
--------

Requirements:

- A C++17 compiler
- [Meson](https://mesonbuild.com/) and [Ninja](https://ninja-build.org/)
  - macOS: `brew install meson ninja`
  - Linux (Debian/Ubuntu): `sudo apt-get install meson ninja-build`
- wxWidgets 3.x
  - macOS: `brew install wxwidgets`
  - Linux (Debian/Ubuntu): `sudo apt-get install libwxgtk3.2-dev`

Build:

    meson setup build
    meson compile -C build

The resulting binary is `build/ah`.

Run the regression test suite:

    meson test -C build

Note: the embedded Python extension is disabled by default (`-Dpython=disabled`),
because it only ever worked against Python 2, which is not available on modern
systems. Passing `-Dpython=enabled` fails fast with a clear error unless a
genuine Python 2 installation is found.
