#define MyAppName "Chess++"
#define MyAppVersion GetEnv("APP_VERSION")
#if MyAppVersion == ""
  #define MyAppVersion "1.0.0-local"
#endif
#define MyAppPublisher "Vitaly German"
#define MyAppExeName "ChessGui.exe"

; Stockfish downloads, resolved at build time by Resolve-Stockfish.ps1: the newest stable
; release and the previous one as fallback, each with the SHA-256 GitHub publishes.
; Builds that don't pass them (/DSfPrimaryUrl=... etc.) use these known-good defaults.
#ifndef SfPrimaryUrl
  #define SfPrimaryTag    "sf_19"
  #define SfPrimaryUrl    "https://github.com/official-stockfish/Stockfish/releases/download/sf_19/stockfish-windows-x86-64-universal.zip"
  #define SfPrimarySha256 "3c8bf1f9ea66a09350a40df4f632288285ac206d99f33ab5842c408fc30b48a7"
#endif
#ifndef SfFallbackUrl
  #define SfFallbackTag    "sf_18"
  #define SfFallbackUrl    "https://github.com/official-stockfish/Stockfish/releases/download/sf_18/stockfish-windows-x86-64-sse41-popcnt.zip"
  #define SfFallbackSha256 "f25830a4567e2bac843d029d3113e6e34ba0cd2f95b9c0bb49afc871fe722a10"
#endif

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
Name: "stockfish"; Description: "Download the Stockfish chess engine ({#SfPrimaryTag}, requires internet)"; GroupDescription: "Optional components:"

[Files]
Source: "..\deploy\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; Unzips and checks the downloaded engine; extracted to {tmp} on demand (ExtractTemporaryFile).
Source: "Prepare-Stockfish.ps1"; Flags: dontcopy

[UninstallDelete]
Type: filesandordirs; Name: "{app}\engines"

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent

[Code]
// Stockfish: one download in the normal case. Each candidate goes through
//   download (SHA-256 verified) -> unzip -> UCI check (Prepare-Stockfish.ps1)
// and the fallback (previous stable release) is used only if any step fails.
// Everything runs into {tmp} when the user clicks Next on the Ready page; the verified
// engine is copied to {app}\engines during installation, together with the log
// stockfish-install.log (also written to the Inno Setup log when run with /LOG).
const
  StockfishCandidates = 2;
  StockfishManualUrl = 'https://stockfishchess.org/download/windows/';

var
  DownloadPage: TDownloadWizardPage;
  StockfishReadyExe: String; // verified engine in {tmp}; empty: none (yet)
  StockfishLog: String;

procedure InitializeWizard;
begin
  DownloadPage := CreateDownloadPage(
    'Downloading Stockfish Chess Engine',
    'Fetching the Stockfish engine ({#SfPrimaryTag}) from GitHub...',
    nil);
end;

procedure SfLog(const Msg: String);
begin
  Log('Stockfish: ' + Msg);
  SaveStringToFile(StockfishLog,
    GetDateTimeString('yyyy/mm/dd hh:nn:ss', '-', ':') + '  ' + Msg + #13#10, True);
end;

procedure StockfishCandidate(Index: Integer; var Tag, Url, Sha256: String);
begin
  if Index = 0 then
  begin
    Tag := '{#SfPrimaryTag}';  Url := '{#SfPrimaryUrl}';  Sha256 := '{#SfPrimarySha256}';
  end else
  begin
    Tag := '{#SfFallbackTag}'; Url := '{#SfFallbackUrl}'; Sha256 := '{#SfFallbackSha256}';
  end;
end;

// Unzip + UCI check + copy to ReadyExe. Prepare-Stockfish.ps1 appends its own lines to the log.
function PrepareStockfish(const Zip, WorkDir, ReadyExe: String): Boolean;
var
  ResultCode: Integer;
begin
  Result := Exec('powershell.exe',
    '-NoProfile -ExecutionPolicy Bypass -File "' + ExpandConstant('{tmp}\Prepare-Stockfish.ps1') + '"' +
    ' -Zip "' + Zip + '" -WorkDir "' + WorkDir + '" -ReadyExe "' + ReadyExe + '" -Log "' + StockfishLog + '"',
    '', SW_HIDE, ewWaitUntilTerminated, ResultCode) and (ResultCode = 0);
  if not Result then
    SfLog('  result     : FAILED (exit code ' + IntToStr(ResultCode) + ')');
end;

// Try the candidates until one is downloaded and verified. Returns False if none works.
function GetStockfish(): Boolean;
var
  I: Integer;
  Tag, Url, Sha256, Zip: String;
  Downloaded: Boolean;
  LogContent: AnsiString;
begin
  Result := False;
  ExtractTemporaryFile('Prepare-Stockfish.ps1');
  SfLog('Chess++ {#MyAppVersion} setup');

  for I := 0 to StockfishCandidates - 1 do
  begin
    StockfishCandidate(I, Tag, Url, Sha256);
    if I = 0 then
      SfLog('Stockfish ' + Tag + ' (newest stable release)')
    else
      SfLog('Fallback: Stockfish ' + Tag + ' (previous stable release)');
    SfLog('  url        : ' + Url);
    SfLog('  sha256     : ' + Sha256);

    Zip := 'stockfish-' + Tag + '.zip';
    DownloadPage.Clear;
    DownloadPage.Add(Url, Zip, Sha256);
    Downloaded := False;
    try
      DownloadPage.Download;
      Downloaded := True;
      SfLog('  download   : OK, checksum verified');
    except
      SfLog('  download   : FAILED - ' + GetExceptionMessage);
      DeleteFile(ExpandConstant('{tmp}\' + Zip));
    end;
    if not Downloaded and DownloadPage.AbortedByUser then
    begin
      SfLog('Download canceled by the user');
      Exit;
    end;

    if Downloaded then
    begin
      DownloadPage.SetText('Checking the Stockfish engine...', '');
      if PrepareStockfish(ExpandConstant('{tmp}\' + Zip), ExpandConstant('{tmp}\sf_' + Tag),
                          ExpandConstant('{tmp}\stockfish.exe')) then
      begin
        StockfishReadyExe := ExpandConstant('{tmp}\stockfish.exe');
        SfLog('Ready: Stockfish ' + Tag);
        Result := True;
        Exit;
      end;
    end;
  end;

  SfLog('No working Stockfish engine');
  LogContent := '';
  LoadStringFromFile(StockfishLog, LogContent);
  MsgBox('The Stockfish engine could not be installed.' + #13#10 + #13#10 +
         'The app will still be installed. To play against the computer, download' + #13#10 +
         'Stockfish from ' + StockfishManualUrl + #13#10 +
         'and place stockfish.exe in: ' + ExpandConstant('{app}\engines\') + #13#10 + #13#10 +
         '--- Log ---' + #13#10 + LogContent,
         mbError, MB_OK);
end;

// Get Stockfish when the user clicks Next on the Ready page (native download progress bar).
// Going Back and Next again doesn't download a second time.
function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if (CurPageID = wpReady) and WizardIsTaskSelected('stockfish') and (StockfishReadyExe = '') then
  begin
    StockfishLog := ExpandConstant('{tmp}\stockfish-install.log');
    DeleteFile(StockfishLog);
    DownloadPage.Show;
    try
      GetStockfish();
    finally
      DownloadPage.Hide;
    end;
  end;
end;

// Copy the verified engine and the log into {app}\engines.
procedure CurStepChanged(CurStep: TSetupStep);
var
  EnginesDir: String;
begin
  if (CurStep = ssPostInstall) and WizardIsTaskSelected('stockfish') and (StockfishLog <> '') then
  begin
    WizardForm.StatusLabel.Caption := 'Installing Stockfish chess engine...';
    WizardForm.FilenameLabel.Caption := '';
    EnginesDir := ExpandConstant('{app}\engines');
    ForceDirectories(EnginesDir);
    if StockfishReadyExe <> '' then
    begin
      if CopyFile(StockfishReadyExe, EnginesDir + '\stockfish.exe', False) then
        SfLog('Installed: ' + EnginesDir + '\stockfish.exe')
      else
      begin
        SfLog('ERROR: could not copy the engine to ' + EnginesDir);
        MsgBox('Stockfish could not be copied to ' + EnginesDir + '.', mbError, MB_OK);
      end;
    end;
    CopyFile(StockfishLog, EnginesDir + '\stockfish-install.log', False);
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
