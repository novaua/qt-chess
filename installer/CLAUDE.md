# installer/

The Windows installer: Inno Setup (`ChessSetup.iss`) over the windeployqt'd Release build. No CMake.

- **CI**: `.github/workflows/release.yml` runs `tests.yml`, then builds and signs the installer on every PR into
  `develop` / `main` (uploaded as an artifact, 14 days) and on a `v*` tag (published as a release whose body is
  `docs/release-notes/<tag>.md` when there is one, else GitHub's generated notes).
- **Locally**: `.\installer\Build-Installer.ps1` (`-Version`, `-SkipBuild`, `-QtBinDir`) does what
  `release.yml` does: MSBuild Release|x64 → `deploy\` (windeployqt) → `Resolve-Stockfish.ps1` → ISCC →
  `installer\Output\ChessSetup-<version>.exe`, unsigned. `ChessSetup.iss` doesn't compile without the
  Stockfish defines: build through this script, not ISCC alone.
- **Stockfish isn't bundled**: the installer downloads it (an optional task): the newest stable release,
  the previous one as fallback, each with GitHub's SHA-256, pinned at build time by `Resolve-Stockfish.ps1`
  (`GITHUB_TOKEN` avoids the API rate limit). `Prepare-Stockfish.ps1`, run by the installer, unzips it and
  checks the UCI handshake; it must run on Windows PowerShell 5.1. Log: `{app}\engines\stockfish-install.log`.
  `Test-StockfishDownload.ps1` dry-runs those steps into `test-engines\`.
- **Smoke test** (BL-05 step 6): `Test-Installer.ps1`, run by `release.yml` after the installer is built:
  installs it silently, runs the installed app on five screenshot scenes with Qt off `PATH` (only installed
  files count; FakeUci as the engine), checks exit codes, pictures, logs and the TLS backend, uninstalls.
  Results in `test-results\installer\`; the exit code is the number of failed checks. Locally only
  `-AppDir <folder>` (e.g. `deploy`): with an installer it refuses to run where Chess++ is installed.
- **Art**: `art\wizard-*.bmp`, the app icon `ChessGui/ChessGui/chess.ico` and `pics/app-icon.png` are drawn by
  `python scripts\make_icons.py` (Pillow), from the board's white knight. Change the script, not the pictures.
