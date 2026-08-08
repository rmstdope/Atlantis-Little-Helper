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

Releases
--------

Ready-built archives are published on the
[Releases page](https://github.com/rmstdope/Atlantis-Little-Helper/releases).
They are produced automatically by the `Release` GitHub Actions workflow
whenever a version tag is pushed - no manual build or upload is involved.

### Tag format

Only these tag shapes trigger a release:

    v<MAJOR>.<MINOR>.<PATCH>            e.g. v2.8.0      -> normal release
    v<MAJOR>.<MINOR>.<PATCH>-<SUFFIX>   e.g. v2.8.0-rc1  -> GitHub pre-release

A suffixed tag is marked as a pre-release and does not become "Latest", which
makes it the safe way to rehearse the whole pipeline. Anything else (`v2.8`,
`v2.7.0.apple`, a typo like `v2.8.O`) is rejected with an explanatory error
rather than silently doing nothing.

### Creating a release tag

1. Bump the version in **both** places, in a normal reviewed commit on `master`:
   - `meson.build` - the `version:` field
   - `src/version.h` - `AH_VERSION`

   The workflow compares both against the tag and refuses to publish if they
   disagree, so this has to happen before tagging.

2. Tag that commit and push the tag:

        git checkout master && git pull origin master
        git tag -a v2.8.0 -m "Atlantis Little Helper 2.8.0"
        git push origin v2.8.0

3. Follow the run under Actions -> Release. It builds and runs the full test
   suite on all three platforms first; the release is only published once every
   platform has succeeded, so a failure never leaves a partial release visible.

### What gets published

One archive per platform, plus a combined `SHA256SUMS.txt`:

    alh-<version>-macos-arm64.tar.gz
    alh-<version>-macos-intel.tar.gz
    alh-<version>-linux-x86_64.tar.gz
    SHA256SUMS.txt

Each archive unpacks to a single `alh-<version>-<platform>/` directory
containing the `ah` executable, `LICENSE`, `BUILD-INFO.txt` (version, tag,
commit, platform, build timestamp), the `doc/` files, and `terrain_bitmaps/`.

Verify a download with:

    shasum -a 256 -c SHA256SUMS.txt

### Running a downloaded build

The binaries are **dynamically linked against wxWidgets 3.x**, which must be
installed separately:

- macOS: `brew install wxwidgets`
- Linux (Debian/Ubuntu): `sudo apt-get install libwxgtk3.2-1t64`
  (older releases: `libwxgtk3.2-1`)

On macOS, archives downloaded through a browser are quarantined by Gatekeeper.
Clear it after unpacking:

    xattr -dr com.apple.quarantine alh-<version>-macos-arm64

ALH stores its configuration and history in the current directory, so run it
from a separate directory per game - see `doc/readme.html`.

### Recovering from a failed release run

Fix whatever failed, then re-drive the same tag from Actions -> Release ->
Run workflow, entering the tag name as the input. There is no need to delete
and re-push the tag. Re-runs are idempotent: the existing release is reused,
assets are replaced in place rather than duplicated, and release notes you have
edited by hand are left untouched. A release left as a draft by an interrupted
run is published by the next successful run.

If the failure was the version-consistency check, the fix is a new commit that
corrects `meson.build`/`src/version.h`, and a tag moved to point at it.
