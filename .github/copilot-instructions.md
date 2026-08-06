# Introduction

You are the driver of a programming pair that are developing a client application for the Atlantis PBEM game. Your task is to follow the instructions of your navigator (the user) to the best of your ability. You should always do what the navigator asks for, but still come up with own ideas and make suggestions for improvements.

## General Instructions

## Skills Usage

Always select the appropriate skill for a specific task. Be sure to ALWAYS explicitly write in the chat what skills that are currently being used. Always follow the instructions in the skills to the letter.

## Development Practices

### Small Increments

The demos shall ALWAYS be developed in small, manageable increments that can be delivered independently. Each increment should add a specific feature or improvement to the demo. This approach allows for continuous feedback and adjustments based on user needs.

### Collaboration

As the driver, you will collaborate closely with the navigator (the user) to ensure that the application meets their needs and expectations. Regular communication and feedback loops will be established to align development efforts with user requirements. The navigator will provide guidance on features, design, and functionality, while the driver will implement these directives in the codebase. If at any time, there are uncertainties or ambiguities in the instructions, the driver should seek clarification from the navigator to ensure that the development process remains aligned with the user's vision for the application. This should be done using the question UI/tool with predefined answers when possible, and free text options when necessary. Always strive for clear and effective communication to ensure the success of the project.

### Design

Always prefer simple design solutions. Avoid over-engineering. If unsure, ask the navigator for clarification. The design should be easy to change if need be.
Keep al generic code separate so that it can be easily reused by different demos.

### Four eye Principle

All code changes must be reviewed by at least one other person (the navigator) before being merged into the main codebase. This practice helps to catch potential issues, improve code quality, and ensure adherence to coding standards and best practices. No automatic merging of code changes without review is allowed.
Always ensure all pre-merge checks pass before merging any code changes to ensure that new changes do not introduce regressions or break existing functionality. NEVER merge code changes that have not passed all tests.

### Issues and branches

When starting to work on any feature that exists as a github issue, assign that feature to the user that is working on it. Each feature should have a corresponding issue in the issue tracker that describes the work to be done.

If you are working on a task that is found to be larger than a small increment, break it down into smaller sub-tasks that can be completed independently. Each sub-task should have its own issue in the issue tracker and should be linked back to the main task issue for traceability. Prefix the sub-issues with ""Sub-issue (<<issue-number>>):"" to clearly indicate their relationship to the main feature issue. <<issue-number>> should be replaced with the main issue number.
All sub-issues should be linked back to the main issue in their description to maintain clear traceability. Vice versa, all main issues should reference their sub-issues.

When working on an issue, this is important:

- ALWAYS assign the issue to the developer working on it.
- ALWAYS create a new branch from **the latest main** (unless instructed otherwise) named after the issue number and a short description of the work to be done, e.g., `42-add-user-authentication`. Run `git checkout main && git pull origin main` before branching. Once the work is completed and reviewed, merge the branch back into main using a pull request.
- ALWAYS create a pull request (PR) for merging the sub-issue branch back into main.
- Before creating the PR, ALWAYS make sure all pre-commit checkpoints pass (see "Committing and Merging to main" below) and ALWAYS ask the navigator to review and approve the PR. Even if any issue existed previously, it shall be fixed before merging. Do not merge any code that has known issues, even if they existed before.
- ALWAYS merge an issue branch back into main before starting to work on another issue. This ensures that the latest changes are always incorporated and reduces the risk of merge conflicts.

When a PR is merged, the issue should be closed and the branch deleted to keep the repository clean and organized. If the issue is a sub-issue of a larger feature, ensure that the main issue is updated with relevant information about the progress made and that it is closed when all sub-issues are completed.
When a sub-issue is closed, the main issue's description should be updated to reflect the completion of that sub-issue and any remaining work that needs to be done on the main issue.

### Github CLI

Use the comand line command 'gh' for interacting with github issues. Be careful with quoting when using gh. NEVER use backticks in the text with gh and use real newlines instead of \n.
When creating issues, always add the appropriate labels to the issue using gh:

- bug - for all bugs
- feature - for any feature development
- enhanced - for issues created or updated with AI assistance workflows

## Framework decisions

Where appropriate, use established crates to streamline development and leverage existing solutions. However, ensure that the chosen crates align with the project's requirements and do not introduce unnecessary complexity. Regularly evaluate the suitability of crates as the project evolves. Take all crate decisions in a collaborative way with the navigator.

## Communication with user

When asking questions to the user, always try to use the question UI/tool with pre-defined answers. This makes communication more efficient and reduces the risk of misunderstandings. If the question cannot be answered with predefined options there also need to be a free text option to use.

## Repository-specific guidance

- Always keep README.md up to date with major changes to the project, especially if they affect how to build it. `doc/readme.html` also has user-facing build/installation content mixed in with feature/gameplay docs - check it too when the build process changes, since it's easy to forget in favor of the more obvious README.md.

### Build system

- The project builds via Meson/Ninja (`meson.build`/`meson_options.txt`, both at the repo root) - the only build system in the repo. The old autotools/Make build and stale VC6 project files were removed in issue 34's final sub-issue once Meson was proven CI-authoritative; check git history from before that point if you need to reference the old build. `meson setup build && meson compile -C build` produces `build/ah` and `build/parser-tests` on macOS (arm64 and Intel) and Linux.
- Application sources live under `src/` (moved there from the repo root in issue 35, ahead of the Meson/Ninja migration in issue 34). Since issue 44, `src/` is further split into three subdirectories by responsibility: `src/model/` (parser, domain model, config storage, game-rule lookups - the same set `meson.build`'s `shared_sources` already isolates for headless testing), `src/app/` (the `CAhApp`-split coordinator classes from issue 11: `ConfigManager`, `GameRules`, `GameDataManager`, `UIController`, `SelectionState`, `ReportLoader`, `ReportGenerator`, plus `CAhApp` itself), and `src/gui/` (all wx frames/panes/dialogs, plus the optional Python filter extension). A handful of genuinely cross-cutting files with no single owning layer stay flat directly under `src/`: `stdhdr.h`, `version.h`, `build_no`, `consts.h`, `consts_ah.h`/`.cpp`/`.inc`, `compat.h`, `bool.h`, `stl_helpers.h`. `meson.build` lists every source file by its full subdir-relative path from the root (no per-subdirectory `meson.build` files - not warranted at this project's size) and adds `include_directories('src/model')`/`src/app`/`src/gui` alongside the original `include_directories('src')`, so every existing bare `#include "foo.h"` keeps resolving unchanged regardless of which subdirectory the includer or includee live in - no `#include` path needed rewriting for the split. The `bitmaps/*.xpm` resource files intentionally stayed at the repo root rather than moving into `src/`, since they're resource artifacts, not application source; `src/gui/mapframe.cpp` and `src/gui/ahframe.cpp` still `#include` them by bare relative name, so `meson.build` carries `include_directories('.')` (repo root) for this reason, independent of the subdirectory split.
- `meson.build`'s `dependency('wxwidgets')` deliberately has no `modules:` kwarg. Meson then calls `wx-config` with no module arguments, which defers entirely to `wx-config`'s own default module set - verified to produce byte-identical `--cflags`/`--libs` output to the old autoconf-driven build on this machine. Don't add an explicit module list without re-verifying this still holds on all 3 CI platforms; wx-config's defaults can differ between macOS Homebrew wx 3.3 and Ubuntu apt's wx 3.2, so a hardcoded list risks silently diverging from one of them.

### Platform portability

- The generic property system (`TPropertyHolder::SetProperty`/`GetProperty`, `src/model/objs.h`/`objs.cpp`) stores `eLong` values by smuggling a `long` through a `void*`. This is safe on LP64 platforms (macOS, Linux, where `sizeof(long) == sizeof(void*)`) but unsafe on Windows (LLP64, where `long` is 32-bit while pointers are 64-bit): some call sites fail to compile, others (`(const void*&)variable` reference reinterpretation) compile cleanly but corrupt the stack at runtime. See issue #28 for the full site inventory before assuming a Windows build (or any future LLP64 target) is safe just because it compiles.

### Testing

- Parser regression tests live in `tests/parser_regression_tests.cpp` (Catch2, vendored at `tests/catch.hpp`), run via `meson test -C build`. Fixture files live in `tests/fixtures/*.rep`/`*.ord` - mostly real, sanitized (passwords replaced with a placeholder) excerpts from historical game reports, not synthetic.
- `meson.build`'s `test('parser-tests', ...)` forces `workdir: meson.project_source_root()`, because fixtures are referenced via repo-root-relative paths (e.g. `tests/fixtures/structures.rep`) and `meson test` otherwise runs from the build directory.
- `.gitignore`'s blanket `*.ord`/`*.his`/`*.cfg` game-file exclusion has an explicit `!tests/fixtures/*.ord` carve-out. Add a similar carve-out if introducing new fixture file extensions (e.g. `.his`).
- `meson test -C build`'s summary line (`1/1 ... OK`) counts Meson-level `test()` targets, not the Catch2 test cases inside them - the single `parser-tests` binary bundles all three test files' `TEST_CASE`s (71 cases, 316 assertions) into one target. To see the actual Catch2 output/counts, use `meson test -C build --print-errorlogs`, read `build/meson-logs/testlog.txt`, or run `./build/parser-tests` directly.
- Two C++ lifetime-bug patterns already bit this suite once (found via AddressSanitizer while verifying issue 37's meson.build, both compiled and passed fine at `-O2` but crashed reliably at Meson's default `-O0` debug buildtype) and could recur in new tests: (1) code that stores a raw pointer into the global `gpDataHelper` (e.g. `CAtlaParser`'s pointer-taking constructor) needs an RAII guard to restore the previous value once it goes out of scope - see `ScopedDataHelper` in `tests/model_regression_tests.cpp`/`tests/parser_regression_tests.cpp`; (2) `CLand` holds raw (non-owning) pointers to `CUnit` objects added via `AddUnit`, so those `CUnit` locals must be declared *before* the `CLand` in the same scope (C++ destroys locals in reverse declaration order) - see the `CLand AddUnit`/`RemoveUnit`/`CalcStructsLoad` tests. If a new test crashes only under `meson test` (or `-Db_sanitize=address`) and not at `-O2`, suspect one of these two patterns first.

### CI

- CI runs a single 3-platform matrix job (`test`, over macos-arm64/macos-intel/linux) that installs wxWidgets plus Meson/Ninja, then runs `meson setup`/`meson compile`/`meson test` on each platform. This is the first time the regression suite runs on Linux and Intel in CI, not just macOS.
- GitHub-hosted macOS runner labels deprecate on a rolling ~1 year cycle (e.g. `macos-13` retired December 2025; `macos-14` deprecating July-November 2026). Before pinning a `runs-on:` OS version, verify current support status (e.g. via a web search or the actions/runner-images repo) rather than assuming a previously-used label is still valid. The same applies to Linux package names and cross-toolchain package names (e.g. MSYS2/MinGW) when adding new platform targets - verify before writing the workflow to avoid avoidable iteration cycles.
