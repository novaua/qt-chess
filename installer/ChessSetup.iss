#define MyAppName "Chess++"
#define MyAppVersion GetEnv("APP_VERSION")
#if MyAppVersion == ""
  #define MyAppVersion "1.0.0-local"
#endif
#define MyAppPublisher "Vitaly German"
#define MyAppExeName "ChessGui.exe"

[Setup]
AppId={{F3A8B2C1-5D47-4E89-B123-9A0C1D2E3F4A}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL=https://github.com/vital-german/qt-chess
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir=Output
OutputBaseFilename=ChessSetup-{#MyAppVersion}
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional icons:"
Name: "stockfish"; Description: "Download latest Stockfish chess engine (requires internet)"; GroupDescription: "Optional components:"

[Files]
Source: "..\deploy\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent

[Code]
// Stable URL — GitHub's /releases/latest/download/ always redirects to
// the current release, so no API call is needed to resolve the tag.
const
  StockfishUrl = 'https://github.com/official-stockfish/Stockfish/releases/latest/download/stockfish-windows-x86-64-avx2.zip';
  StockfishZip = 'stockfish.zip';

// Download using curl.exe, which ships with every Windows 10 1803+ / Windows 11
// install at {sys}\curl.exe. curl handles TLS natively, follows redirects,
// and produces no Inno Setup-internal log spam.
function DownloadWithCurl(const Url, DestFile: String): Boolean;
var
  Curl: String;
  ResultCode: Integer;
begin
  Curl := ExpandConstant('{sys}\curl.exe');
  Result := FileExists(Curl);
  if not Result then
  begin
    Log('Stockfish: curl.exe not found in System32');
    Exit;
  end;
  Log('Stockfish: downloading ' + Url);
  Result := Exec(Curl, '-L -s --retry 2 -o "' + DestFile + '" "' + Url + '"',
                 '', SW_HIDE, ewWaitUntilTerminated, ResultCode)
            and (ResultCode = 0);
  if Result then
    Log('Stockfish: download complete')
  else
    Log(Format('Stockfish: curl failed (exit code %d)', [ResultCode]));
end;

// Extract zip and copy stockfish.exe using PowerShell (local only, no network).
procedure ExtractStockfish(const ZipFile, EnginesDir: String);
var
  ScriptFile, LogFile: String;
  ResultCode: Integer;
  LogContent: AnsiString;
begin
  ScriptFile := ExpandConstant('{tmp}\sf_extract.ps1');
  LogFile    := ExpandConstant('{tmp}\sf_extract.log');

  SaveStringToFile(ScriptFile,
    '$log     = "' + LogFile + '"' + #13#10 +
    '$zipFile = "' + ZipFile + '"' + #13#10 +
    '$sfDir   = "' + ExpandConstant('{tmp}') + '\sf"' + #13#10 +
    'function Log($m) { $m | Out-File $log -Append -Encoding ASCII }' + #13#10 +
    'try {' + #13#10 +
    '  "=== Extracting $(Get-Date) ===" | Out-File $log -Encoding ASCII' + #13#10 +
    '  Expand-Archive $zipFile -DestinationPath $sfDir -Force' + #13#10 +
    '  Log "Extraction OK"' + #13#10 +
    '  $exe = Get-ChildItem $sfDir -Recurse -Filter "stockfish*.exe" | Select-Object -First 1' + #13#10 +
    '  if (-not $exe) { throw "No stockfish*.exe found in zip" }' + #13#10 +
    '  Log "Found: $($exe.FullName)"' + #13#10 +
    '  Copy-Item $exe.FullName "' + EnginesDir + '\stockfish.exe" -Force' + #13#10 +
    '  Log "=== Done ==="' + #13#10 +
    '} catch {' + #13#10 +
    '  Log "ERROR: $_"' + #13#10 +
    '  Log "Stack: $($_.ScriptStackTrace)"' + #13#10 +
    '  exit 1' + #13#10 +
    '}',
    False);

  if not Exec('powershell.exe',
      '-NoProfile -ExecutionPolicy Bypass -File "' + ScriptFile + '"',
      '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
    ResultCode := -1;

  if ResultCode <> 0 then
  begin
    LogContent := '(no log)';
    LoadStringFromFile(LogFile, LogContent);
    MsgBox('Stockfish could not be extracted (exit code: ' + IntToStr(ResultCode) + ').' + #13#10 +
           'Place stockfish.exe manually in: ' + EnginesDir + #13#10 + #13#10 +
           '--- Log ---' + #13#10 + LogContent,
           mbError, MB_OK);
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ZipFile, EnginesDir: String;
begin
  if (CurStep = ssPostInstall) and WizardIsTaskSelected('stockfish') then
  begin
    ZipFile    := ExpandConstant('{tmp}\' + StockfishZip);
    EnginesDir := ExpandConstant('{app}\engines');
    ForceDirectories(EnginesDir);

    WizardForm.StatusLabel.Caption := 'Downloading Stockfish chess engine...';
    WizardForm.FilenameLabel.Caption := StockfishUrl;

    if DownloadWithCurl(StockfishUrl, ZipFile) then
    begin
      WizardForm.StatusLabel.Caption := 'Installing Stockfish chess engine...';
      WizardForm.FilenameLabel.Caption := '';
      ExtractStockfish(ZipFile, EnginesDir);
    end else
      MsgBox('Stockfish could not be downloaded (curl.exe required, included with Windows 10 1803+).' + #13#10 +
             'You can place stockfish.exe manually in: ' + EnginesDir,
             mbError, MB_OK);

    WizardForm.StatusLabel.Caption := 'Done.';
    WizardForm.FilenameLabel.Caption := '';
  end;
end;
