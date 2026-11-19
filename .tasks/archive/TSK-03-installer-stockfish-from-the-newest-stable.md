---
id: TSK-03
title: "Installer: Stockfish from the newest stable release, single download"
type: task
status: done
priority: medium
---

# TSK-03 Installer: Stockfish from the newest stable release, single download

<!-- migrated from TASKS.md:49 (In Progress) -->

- [x] Installer: Stockfish from the newest **stable** release, single download (#72)
  - [x] `Resolve-Stockfish.ps1` pins newest stable + previous stable (fallback) with GitHub's SHA-256 at build time (`Build-Installer.ps1`, `release.yml`); no hard-coded release in `ChessSetup.iss`, the build fails if it can't resolve (review: option A)
  - [x] Per candidate: download (checksum) → unzip → UCI check (`Prepare-Stockfish.ps1`, `uci` → `uciok`, version logged); fallback only if a step fails
  - [x] `{app}\engines\stockfish-install.log` (+ Inno `/LOG`); `Test-StockfishDownload.ps1` mirrors the flow — normal and fallback paths verified
  - [ ] Run the built installer once (normal install) and check `engines\stockfish-install.log`
