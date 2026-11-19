---
id: TSK-02
title: "Installer: install the Microsoft VC++ runtime when missing"
type: task
status: open
priority: medium
---

# TSK-02 Installer: install the Microsoft VC++ runtime when missing

<!-- migrated from TASKS.md:45 (In Progress) -->

- [ ] Installer: install the Microsoft VC++ runtime when missing (branch `fixVcRedistInstall_281026`)
  - [x] `vc_redist.x64.exe` no longer copied into `{app}`; unpacked to `{tmp}` and run (`/install /quiet /norestart`) only if the installed runtime (registry `VC\Runtimes\x64`) is missing or older than the bundled one; exit codes 0/1638 OK, 3010 → restart offered, others → warning with download link
  - [x] `Build-Installer.ps1` sets `VCINSTALLDIR` so local builds bundle it too; the build fails if it's missing
  - [ ] Install on a PC without the runtime (or older) and check the `/LOG` setup log line `VC++ runtime …: exit code`
