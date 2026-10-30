# qt-chess
Chess++ is a free, open-source chess game for Windows with a Qt QML interface and a C++ chess core.

* Play locally against another person, or against a computer using any UCI engine (Stockfish recommended) with five difficulty levels
* Play online through your Lichess account
* Local user profiles with saved games and settings, move history, captured pieces, sound effects

[Gameplay demo 2026](https://youtu.be/hlW6xv23fN4) 

## Download
Latest installer: [download the newest release](https://github.com/novaua/qt-chess/releases/latest) (Windows 10/11, x64).
All versions: [Releases](https://github.com/novaua/qt-chess/releases). Release notes for each version are in [docs/release-notes](docs/release-notes).

Tools required to compile:
* MS Visual Studio 2022
* Qt 6.12+ (with the Qt Multimedia add-on) and [Qt VS Tools](https://marketplace.visualstudio.com/items?itemName=TheQtCompany.QtVisualStudioTools2022)

## Build the installer locally
Needs the tools above plus [Inno Setup 6](https://jrsoftware.org/isdl.php) and `windeployqt` on `PATH` (or pass `-QtBinDir`):
```powershell
.\installer\Build-Installer.ps1                 # version = latest git tag + "-local"
.\installer\Build-Installer.ps1 -Version 0.2.0  # explicit version
.\installer\Build-Installer.ps1 -SkipBuild      # repackage existing x64\Release binaries
```
The result is `installer\Output\ChessSetup-<version>.exe`. Local installers are unsigned; releases are built and signed by CI.
`installer\Deploy-Qt.ps1` collects the Qt files (only what the app uses; plugins and QML go to `qt\`, see `qt.conf`).

# Generate self-signed Code Signing certificate (run once, locally)
# 1. Create the certificate in your personal store
```powershell
$cert = New-SelfSignedCertificate `
    -Subject "CN=Chess++, O=VigeCompany" `
    -Type CodeSigning `
    -CertStoreLocation Cert:\CurrentUser\My `
    -HashAlgorithm SHA256 `
    -NotAfter (Get-Date).AddYears(5)
```
# 2. Export to a PFX file
```powershell
$password = ConvertTo-SecureString -String "YourPasswordHere" -Force -AsPlainText
Export-PfxCertificate -Cert $cert -FilePath "ChessPluPlus.pfx" -Password $password
```
# 3. Encode to base64 for GitHub Secrets
```powershell
$bytes = [System.IO.File]::ReadAllBytes("ChessPluPlus.pfx")
[System.Convert]::ToBase64String($bytes) | Out-File 'Base64Cert.txt'
```
Add the content of `Base64Cert.txt` to the GitHub repository secret `PFX_CERT_BASE64` and the password to `PFX_PASSWORD`.

> **Note:** Self-signed certificates trigger a Windows SmartScreen warning ("Windows protected your PC") on first run.
> Users click **More info → Run anyway**. This is expected for a personal project.
> A commercial release would use a CA-signed EV certificate to avoid this warning.

## License
Copyright (C) 2015-2026 Vitaly German

Chess++ is free software, licensed under the [GNU General Public License v3.0](LICENSE) (GPL-3.0-only).
It comes with NO WARRANTY, to the extent permitted by law. Derived works must be released under the same license with their source.

## Code signing policy
Free code signing provided by [SignPath.io](https://signpath.io), certificate by [SignPath Foundation](https://signpath.org).
This applies to releases once the SignPath Foundation certificate is active; earlier releases are signed with a self-signed certificate (see the note above).

* **Build:** installers are built from the source in this repository by the [GitHub Actions workflow](.github/workflows/release.yml) on `v*` tags. Only binaries built this way are signed.
* **Roles:** committer, reviewer and approver: [Vitaly German](https://github.com/novaua). Changes from other contributors are reviewed by the maintainer before merging. Every release is approved manually before signing.
* **Scope:** only Chess++'s own binaries (`ChessGui.exe`, `LiChessConnector.dll`) and the installer. Qt libraries and the optional Stockfish engine are third-party and are not signed by this project.

## Privacy
Chess++ does not collect analytics or telemetry, and has no accounts of its own. Network access happens only in these cases:

* **Lichess (only if you use online play):** when you log in or play online, the app talks to `lichess.org` (OAuth login, account name, games and moves). Your Lichess access token is stored encrypted in your local user profile and can be removed by disconnecting Lichess in the app.
* **QR code for challenges (only when you create a Lichess challenge):** the dialog shows a QR code rendered by `api.qrserver.com`; the challenge link is sent to that service to produce the image.
* **Stockfish download (optional, installer only):** if you tick the option in the installer, it downloads Stockfish from `github.com/official-stockfish/Stockfish` releases.
* **Local data:** profiles, settings and saved games stay on your computer.

Playing locally or against a local UCI engine uses no network connection.
