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
procedure DownloadStockfish();
var
  ResultCode: Integer;
  EnginesDir, ScriptFile, LogFile: String;
  LogContent: AnsiString;
begin
  EnginesDir  := ExpandConstant('{app}\engines');
  ScriptFile  := ExpandConstant('{tmp}\sf_download.ps1');
  LogFile     := ExpandConstant('{tmp}\sf_download.log');

  ForceDirectories(EnginesDir);

  // Write the PowerShell script to a temp file to avoid all quote-escaping
  // issues that arise when passing -Command "..." on the command line.
  SaveStringToFile(ScriptFile,
    '$log    = "' + LogFile + '"' + #13#10 +
    '$tmpDir = "' + ExtractFilePath(ScriptFile) + '"' + #13#10 +
    '$zipFile = "$tmpDir\sf.zip"' + #13#10 +
    '$sfDir   = "$tmpDir\sf"' + #13#10 +
    'function Log($m) { $m | Out-File $log -Append -Encoding ASCII }' + #13#10 +
    'try {' + #13#10 +
    '  "=== Stockfish download started $(Get-Date) ===" | Out-File $log -Encoding ASCII' + #13#10 +
    '  Log "Working dir: $tmpDir"' + #13#10 +
    '  $rel = (Invoke-RestMethod "https://api.github.com/repos/official-stockfish/Stockfish/releases/latest").tag_name' + #13#10 +
    '  Log "Release tag: $rel"' + #13#10 +
    '  $zip = "stockfish-windows-x86-64-avx2.zip"' + #13#10 +
    '  $url = "https://github.com/official-stockfish/Stockfish/releases/download/$rel/$zip"' + #13#10 +
    '  Log "Downloading: $url"' + #13#10 +
    '  Invoke-WebRequest $url -OutFile $zipFile -UseBasicParsing' + #13#10 +
    '  Log "Download OK. Extracting..."' + #13#10 +
    '  Expand-Archive $zipFile -DestinationPath $sfDir -Force' + #13#10 +
    '  Log "Extraction OK. Searching for stockfish*.exe..."' + #13#10 +
    '  $exe = Get-ChildItem $sfDir -Recurse -Filter "stockfish*.exe" | Select-Object -First 1' + #13#10 +
    '  if (-not $exe) { throw "No stockfish*.exe found inside the zip" }' + #13#10 +
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
    LogContent := '(log file not found)';
    LoadStringFromFile(LogFile, LogContent);
    MsgBox(
      'Stockfish could not be downloaded automatically (exit code: ' +
        IntToStr(ResultCode) + ').' + #13#10 +
      'You can get it from https://stockfishchess.org and place it in:' + #13#10 +
      EnginesDir + #13#10#13#10 +
      '--- Download log ---' + #13#10 +
      LogContent,
      mbError, MB_OK);
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssPostInstall) and WizardIsTaskSelected('stockfish') then
  begin
    WizardForm.StatusLabel.Caption := 'Downloading Stockfish chess engine...';
    WizardForm.FilenameLabel.Caption := 'https://github.com/official-stockfish/Stockfish';
    DownloadStockfish();
    WizardForm.StatusLabel.Caption := 'Done.';
    WizardForm.FilenameLabel.Caption := '';
  end;
end;
