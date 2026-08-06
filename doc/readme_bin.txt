Atlantis Little Helper - binary release archive
===============================================

This file ships inside the release archives published at

    https://github.com/rmstdope/Atlantis-Little-Helper/releases

Archives are built automatically by the project's Release workflow whenever a
version tag is pushed, so every published build comes from a known commit and
has passed the full regression suite on all supported platforms.


Contents of the archive
-----------------------

    ah                    the Atlantis Little Helper executable
    LICENSE               GNU General Public License, version 2
    BUILD-INFO.txt        version, tag, commit, platform and build timestamp
    doc/readme.html       full user documentation - start here
    doc/history.txt       version history
    doc/readme_bin.txt    this file
    terrain_bitmaps/      optional bitmaps for terrain display


Requirements
------------

The executable is dynamically linked against wxWidgets 3.x, which must be
installed separately:

    macOS                 brew install wxwidgets
    Linux (Debian/Ubuntu) sudo apt-get install libwxgtk3.2-1t64
                          (older releases: libwxgtk3.2-1)

On macOS, an archive downloaded through a browser is quarantined by Gatekeeper.
After unpacking, clear the quarantine attribute:

    xattr -dr com.apple.quarantine <unpacked directory>


Running it
----------

ALH keeps its configuration and history in the current directory, so use a
separate directory per game and start ALH from that directory. See the
"Installation and configuration" section of doc/readme.html for details.

To use bitmap terrains, copy the .bmp files from terrain_bitmaps into your game
directory. Terrain bitmaps are not used by macOS builds.


Verifying a download
--------------------

Each release also publishes SHA256SUMS.txt covering every archive:

    shasum -a 256 -c SHA256SUMS.txt


Building from source
--------------------

See README.md in the repository root.
