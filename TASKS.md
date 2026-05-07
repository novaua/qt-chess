# Tasks

## Branch: feMakeInstaller_0430a — CMake Installer Pipeline

### In Progress

- [ ] Fix CMake build so `cmake --build build --config Release` produces `ChessGui.exe`
  - CMakeLists.txt added (commit e463983) but build is still failing (commits 360efa3–9ee0a48)
  - Check that all four VS projects (ChessCore, UciConnector, ChessConnector, ChessGui) are wired in CMakeLists.txt
  - Verify Qt6 find_package paths and windeployqt invocation

- [ ] Validate GitHub Actions workflow (`.github/workflows/release.yml`)
  - Workflow builds with CMake + Ninja on `windows-latest`
  - Runs `windeployqt` then packages with Inno Setup (`installer/ChessSetup.iss`)
  - Test by opening a PR against `develop` — artifact should appear under the run

- [ ] Create / verify Inno Setup script (`installer/ChessSetup.iss`)
  - Script must pick up `deploy/` folder produced by the workflow
  - Output file should match `installer/Output/ChessSetup-*.exe`

### Backlog

- [ ] Merge `feMakeInstaller_0430a` → `develop` once CI passes green
- [ ] Tag a release (`v*`) to trigger the GitHub Release publish step

---

## How to use this file

Mark tasks `[x]` when done. Add new tasks under the appropriate section. Move completed sections to the bottom under **Done**.

## Done

<!-- Move completed task blocks here -->
