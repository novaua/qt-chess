# Progress Log

Use this file to record where work stopped and what to do next. Newest entry at the top.

---

## 2026-05-06 — CMake installer pipeline (branch: feMakeInstaller_0430a)

### What was done
- Added `CMakeLists.txt` to support building with CMake + Ninja (needed by GitHub Actions runner which has no Visual Studio solution runner)
- Updated `.github/workflows/release.yml`:
  - Installs Qt 6.7, vcpkg, MSVC x64
  - Configures with CMake/Ninja, builds Release
  - Runs `windeployqt`, collects deploy folder
  - Runs Inno Setup (`installer/ChessSetup.iss`) to produce installer EXE
  - PR builds upload artifact; tag pushes publish a GitHub Release
- Several build-fix commits made (360efa3, 86d0d83, 2f6f4b1, e463983, 9ee0a48)

### Current status
Build is still failing — last commit message is "attempt to fix build" (9ee0a48). The `.vcxproj` and `.vcxproj.filters` files were also modified, suggesting there may be source file additions that need to be reflected in CMakeLists.txt.

### Where to continue
1. Open `CMakeLists.txt` and compare listed source files against what is in `ChessGui.vcxproj.filters` (which changed heavily in the last diff — +109 lines)
2. Confirm all four targets build: `ChessCore` → `UciConnector` → `ChessConnector` → `ChessGui`
3. Check that Qt6 component names match what is installed (`Qt6::Core`, `Qt6::Quick`, `Qt6::Widgets`, etc.)
4. Push and watch the GitHub Actions run on the PR
5. Once green, check that the artifact `ChessSetup-*.exe` installs and runs correctly

### Key files
| File | Purpose |
|---|---|
| `CMakeLists.txt` | Top-level CMake build — main thing to fix |
| `.github/workflows/release.yml` | CI pipeline (CMake → windeployqt → Inno Setup) |
| `installer/ChessSetup.iss` | Inno Setup script (verify it exists and paths are correct) |
| `ChessGui/ChessGui/ChessGui.vcxproj` | VS project — reference for source file list |
| `ChessGui/ChessGui/ChessGui.vcxproj.filters` | Filter list changed heavily; reflects new/moved files |
