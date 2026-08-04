alh
===

Atlantis Little Helper

Building on macOS (Apple Silicon)
---------------------------------

Requirements:

- Xcode Command Line Tools (`xcode-select --install`)
- wxWidgets 3.x (`brew install wxwidgets`)

Build:

    ./configure --with-python=no
    mkdir -p obj bin
    make

The resulting binary is `bin/ah` (native arm64).

Note: the embedded Python extension is disabled (`--with-python=no`) because
the configure check requires Python 2, which is no longer available on modern
systems.
