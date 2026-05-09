# qt-chess
Chess UI made on Qt QML and C++
Able to connect to a UCI engine to play with a computer.

[Gameplay demo](https://youtu.be/pBiuGpj8seQ)

Tools required to compile:
* MS Visual Studio 2022
* Qt 6.10+ with [Qt VS Tools](https://marketplace.visualstudio.com/items?itemName=TheQtCompany.QtVisualStudioTools2022)

# Generate self-signed Code Signing certificate (run once, locally)

```powershell
# 1. Create the certificate in your personal store
$cert = New-SelfSignedCertificate `
    -Subject "CN=Chess++, O=VigeCompany" `
    -Type CodeSigning `
    -CertStoreLocation Cert:\CurrentUser\My `
    -HashAlgorithm SHA256 `
    -NotAfter (Get-Date).AddYears(5)

# 2. Export to a PFX file
$password = ConvertTo-SecureString -String "YourPasswordHere" -Force -AsPlainText
Export-PfxCertificate -Cert $cert -FilePath "ChessPluPlus.pfx" -Password $password

# 3. Encode to base64 for GitHub Secrets
$bytes = [System.IO.File]::ReadAllBytes("ChessPluPlus.pfx")
[System.Convert]::ToBase64String($bytes) | Out-File 'Base64Cert.txt'
```

Add the content of `Base64Cert.txt` to the GitHub repository secret `PFX_CERT_BASE64` and the password to `PFX_PASSWORD`.

> **Note:** Self-signed certificates trigger a Windows SmartScreen warning ("Windows protected your PC") on first run.
> Users click **More info → Run anyway**. This is expected for a personal project.
> A commercial release would use a CA-signed EV certificate to avoid this warning.