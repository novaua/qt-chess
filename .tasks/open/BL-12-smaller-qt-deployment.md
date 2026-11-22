---
id: BL-12
title: "Smaller Qt deployment"
type: task
status: open
priority: medium
---

# BL-12 Smaller Qt deployment

> **Note (2026-11-22, LI-11):** the app now also needs Qt Image Formats' `imageformats\qwebp.dll` (Lichess flairs are WebP). Keep it when trimming the deployment; `installer\Test-Installer.ps1` checks it.

<!-- migrated from TASKS.md:95 (In Progress / Backlog) -->

- [ ] BL-12 Smaller Qt deployment — PR #91, **frozen** until the owner picks it up again (more testing and evaluation first) [Medium]: ship only the Qt files the app uses (`installer\Deploy-Qt.ps1`; plugins and QML in `deploy\qt\`; no translations, `opengl32sw`, D3D12 compilers, FFmpeg; `QT_MEDIA_BACKEND=windows`; `QGuiApplication` instead of `QApplication`, no Qt Widgets). Installed size 146 → 67 MB, installer 50.8 → 36 MB. Branch `pruneQtDeploy_301026`: on GitHub still on the old base; a local copy rebased onto `develop` after #93 (e616f08, built and tested, not pushed). To resume: rebase onto the then-current `develop` (expect `PROGRESS.md` / `TASKS.md` conflicts; skip the "spell fix" commit, already in `develop`), Rebuild ChessGui (the Widgets module goes away), then install over the last release and check that the old plugin folders are gone, sounds play, and every page opens.
