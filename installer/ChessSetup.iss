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
// Download URL: GitHub's /releases/latest/download/ redirect always points
// to the current release — no API call needed to resolve the tag.
const
  StockfishUrl = 'https://github.com/official-stockfish/Stockfish/releases/latest/download/stockfish-windows-x86-64-avx2.zip';
  StockfishZip = 'stockfish.zip';

function GetTickCount: Cardinal;
  external 'GetTickCount@kernel32.dll stdcall';

var
  DownloadPage: TDownloadWizardPage;
  LastProgressPct:  Integer;
  LastProgressTick: Cardinal;

// Called by the download engine on every received chunk.
// Logs to the setup log only when >= 1% progress or >= 10 s have elapsed.
function OnStockfishProgress(const Url: String; Progress, ProgressMax: Int64): Boolean;
var
  Pct: Integer;
begin
  Result := True;
  if ProgressMax <= 0 then Exit;

  Pct := Integer((Progress * 100) div ProgressMax);

  if (Pct >= LastProgressPct + 1) or
     (GetTickCount - LastProgressTick >= 10000) then
  begin
    Log(Format('Stockfish download: %d%%  (%d KB / %d KB)', [Pct, Integer(Progress div 1024), Integer(ProgressMax div 1024)]));
    LastProgressPct  := Pct;
    LastProgressTick := GetTickCount;
  end;
end;

procedure InitializeWizard;
begin
  LastProgressPct  := -1;
  LastProgressTick := 0;
  DownloadPage := CreateDownloadPage(
    'Downloading Stockfish Chess Engine',
    'Fetching the latest Stockfish engine from GitHub...',
    @OnStockfishProgress);
end;

// Queue the Stockfish download when the user clicks Next on the Ready page.
// Uses Inno Setup''s built-in WinHTTP download (no PowerShell, native progress bar).
function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if (CurPageID = wpReady) and WizardIsTaskSelected('stockfish') then
  begin
    DownloadPage.Clear;
    DownloadPage.Add(StockfishUrl, StockfishZip, '');
    DownloadPage.Show;
    try
      try
        DownloadPage.Download;
      except
        MsgBox('Stockfish download failed:' + #13#10 + GetExceptionMessage + #13#10 + #13#10 +
               'The app will still be installed. You can place stockfish.exe' + #13#10 +
               'manually in: ' + ExpandConstant('{app}\engines\'),
               mbError, MB_OK);
      end;
    finally
      DownloadPage.Hide;
    end;
  end;
end;

// Extract the already-downloaded zip into {app}\engines\ using PowerShell.
// Only the extraction runs here — no network access, no hanging risk.
procedure ExtractStockfish();
var
  EnginesDir, ZipFile, ScriptFile, LogFile: String;
  ResultCode: Integer;
  LogContent: AnsiString;
begin
  ZipFile    := ExpandConstant('{tmp}\' + StockfishZip);
  EnginesDir := ExpandConstant('{app}\engines');
  ScriptFile := ExpandConstant('{tmp}\sf_extract.ps1');
  LogFile    := ExpandConstant('{tmp}\sf_extract.log');

  if not FileExists(ZipFile) then Exit;
  ForceDirectories(EnginesDir);

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
    LogContent := '(no log)';
    LoadStringFromFile(LogFile, LogContent);
    MsgBox('Stockfish could not be extracted (exit code: ' + IntToStr(ResultCode) + ').' + #13#10 +
           'Place stockfish.exe manually in: ' + EnginesDir + #13#10#13#10 +
           '--- Log ---' + #13#10 + LogContent,
           mbError, MB_OK);
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssPostInstall) and WizardIsTaskSelected('stockfish') then
  begin
    WizardForm.StatusLabel.Caption := 'Installing Stockfish chess engine...';
    WizardForm.FilenameLabel.Caption := '';
    ExtractStockfish();
    WizardForm.StatusLabel.Caption := 'Done.';
  end;
end;
