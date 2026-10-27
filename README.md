# qt-chess
Chess UI made on Qt QML and C++
Able to connect to a UCI engine to play with a computer.

[Gameplay demo 2026](https://youtu.be/hlW6xv23fN4) 

## Download
Latest installer: [Releases](https://github.com/novaua/qt-chess/releases/latest).
First release: [v0.1.0](https://github.com/novaua/qt-chess/releases/tag/v0.1.0) — see the [release notes](docs/release-notes/v0.1.0.md).

Tools required to compile:
* MS Visual Studio 2022
* Qt 6.10+ with [Qt VS Tools](https://marketplace.visualstudio.com/items?itemName=TheQtCompany.QtVisualStudioTools2022)

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
