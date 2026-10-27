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
AppPublisherURL=https://github.com/novaua/qt-chess
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir=Output
LicenseFile=..\LICENSE
OutputBaseFilename=ChessSetup-{#MyAppVersion}
SetupIconFile=..\ChessGui\ChessGui\chess.ico
UninstallDisplayIcon={app}\ChessGui.exe
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
#ifndef NoSign
; CI passes /S"MySignTool=..."; local builds pass /DNoSign=1 (see Build-Installer.ps1)
SignTool=MySignTool
SignedUninstaller=yes
#endif

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional icons:"
Name: "stockfish"; Description: "Download latest Stockfish chess engine (requires internet)"; GroupDescription: "Optional components:"

[Files]
Source: "..\deploy\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
Type: filesandordirs; Name: "{app}\engines"

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent

[Code]
// Download URLs: GitHub's /releases/latest/download/ redirect always points
// to the current release — no API call needed to resolve the tag.
// Stockfish renamed its Windows assets in sf_19 (avx2 -> universal), which broke
// a single hard-coded name, so several candidates are tried in order:
//   0. latest, current naming  (stockfish-windows-x86-64-universal.zip)
//   1. latest, pre-sf_19 naming (stockfish-windows-x86-64-avx2.zip)
//   2. pinned known-good release, verified by SHA-256 (works if "latest" changes again)
const
  StockfishZip = 'stockfish.zip';
  StockfishCandidates = 3;
  StockfishLatest = 'https://github.com/official-stockfish/Stockfish/releases/latest/download/';
  StockfishPinnedUrl = 'https://github.com/official-stockfish/Stockfish/releases/download/sf_19/stockfish-windows-x86-64-universal.zip';
  StockfishPinnedSha256 = '3c8bf1f9ea66a09350a40df4f632288285ac206d99f33ab5842c408fc30b48a7';
  StockfishManualUrl = 'https://stockfishchess.org/download/windows/';

var
  DownloadPage: TDownloadWizardPage;

procedure InitializeWizard;
begin
  DownloadPage := CreateDownloadPage(
    'Downloading Stockfish Chess Engine',
    'Fetching the latest Stockfish engine from GitHub...',
    nil);
end;

procedure StockfishCandidate(Index: Integer; var Url, Sha256: String);
begin
  Sha256 := '';
  case Index of
    0: Url := StockfishLatest + 'stockfish-windows-x86-64-universal.zip';
    1: Url := StockfishLatest + 'stockfish-windows-x86-64-avx2.zip';
  else
    begin
      Url := StockfishPinnedUrl;
      Sha256 := StockfishPinnedSha256;
    end;
  end;
end;

// Try each candidate URL until one downloads. Returns False if all fail.
function DownloadStockfish(): Boolean;
var
  I: Integer;
  Url, Sha256, Errors: String;
begin
  Result := False;
  Errors := '';
  for I := 0 to StockfishCandidates - 1 do
  begin
    StockfishCandidate(I, Url, Sha256);
    DownloadPage.Clear;
    DownloadPage.Add(Url, StockfishZip, Sha256);
    try
      DownloadPage.Download;
      Result := True;
      Break;
    except
      Errors := Errors + Url + #13#10 + '    ' + GetExceptionMessage + #13#10;
      DeleteFile(ExpandConstant('{tmp}\' + StockfishZip));
    end;
  end;

  if not Result then
    MsgBox('The Stockfish engine could not be downloaded:' + #13#10 + #13#10 + Errors + #13#10 +
           'The app will still be installed. To play against the computer, download' + #13#10 +
           'Stockfish from ' + StockfishManualUrl + #13#10 +
           'and place stockfish.exe in: ' + ExpandConstant('{app}\engines\'),
           mbError, MB_OK);
end;

// Download Stockfish when the user clicks Next on the Ready page.
// Uses Inno Setup''s built-in WinHTTP download (no PowerShell, native progress bar).
function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if (CurPageID = wpReady) and WizardIsTaskSelected('stockfish') then
  begin
    DownloadPage.Show;
    try
      DownloadStockfish();
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

// Offer to remove per-user app data (logs, saved games, user profiles, settings)
// after uninstall. The question defaults to Yes. Silent uninstalls keep the data
// unless /DELETEDATA=1 is passed, so scripted reinstalls don't wipe saved games.
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  DataDir: String;
begin
  if CurUninstallStep <> usPostUninstall then
    Exit;

  DataDir := ExpandConstant('{localappdata}\ChessPlusPlus_Inc');
  if not DirExists(DataDir) then
    Exit;

  if (ExpandConstant('{param:DELETEDATA|0}') = '1') or
     (SuppressibleMsgBox(
        'Also delete Chess++ user data (logs, saved games, user profiles and settings)?' + #13#10 + #13#10 +
        DataDir,
        mbConfirmation, MB_YESNO or MB_DEFBUTTON1, IDNO) = IDYES) then
    DelTree(DataDir, True, True, True);
end;
