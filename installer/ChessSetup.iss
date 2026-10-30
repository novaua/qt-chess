#define MyAppName "Chess++"
#define MyAppVersion GetEnv("APP_VERSION")
#if MyAppVersion == ""
  #define MyAppVersion "1.0.0-local"
#endif
#define MyAppPublisher "Vitaly German"
#define MyAppExeName "ChessGui.exe"

; Stockfish downloads, resolved at build time by Resolve-Stockfish.ps1: the newest stable
; release and the previous one as fallback, each with the SHA-256 GitHub publishes.
; Nothing is hard-coded here: build with installer\Build-Installer.ps1 (or release.yml),
; which pass /DSfPrimaryTag=... /DSfPrimaryUrl=... /DSfPrimarySha256=... and the same
; for SfFallback*. A future asset rename then fails the build, not users' installs.
#if !Defined(SfPrimaryTag) || !Defined(SfPrimaryUrl) || !Defined(SfPrimarySha256) || \
    !Defined(SfFallbackTag) || !Defined(SfFallbackUrl) || !Defined(SfFallbackSha256)
  #error Stockfish download not set: build with installer\Build-Installer.ps1 (runs Resolve-Stockfish.ps1)
#endif

; Microsoft Visual C++ runtime the app was built with. windeployqt puts vc_redist.x64.exe
; into deploy\ (only when VCINSTALLDIR is set; Build-Installer.ps1 and release.yml do that).
; Setup runs it only if the installed runtime is missing or older than this version.
#define VCRedistFile AddBackslash(SourcePath) + "..\deploy\vc_redist.x64.exe"
#if !FileExists(VCRedistFile)
  #error deploy\vc_redist.x64.exe missing: build with installer\Build-Installer.ps1 (sets VCINSTALLDIR for windeployqt)
#endif
#define VCRedistMajor
#define VCRedistMinor
#define VCRedistBld
#define VCRedistRbld
#expr GetVersionComponents(VCRedistFile, VCRedistMajor, VCRedistMinor, VCRedistBld, VCRedistRbld)
#define VCRedistVersion Str(VCRedistMajor) + "." + Str(VCRedistMinor) + "." + Str(VCRedistBld) + "." + Str(VCRedistRbld)

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
; Windows 11 look; the images come from scripts\make_icons.py (Setup picks the size for the DPI)
WizardStyle=modern
WizardImageFile=art\wizard-100.bmp,art\wizard-125.bmp,art\wizard-150.bmp,art\wizard-175.bmp,art\wizard-200.bmp,art\wizard-250.bmp
WizardSmallImageFile=art\wizard-small-100.bmp,art\wizard-small-125.bmp,art\wizard-small-150.bmp,art\wizard-small-175.bmp,art\wizard-small-200.bmp,art\wizard-small-250.bmp
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
Source: "..\deploy\*"; Excludes: "vc_redist.x64.exe"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; Unpacked to {tmp} only when needed (VCRedistNeeded); run in CurStepChanged(ssPostInstall).
Source: "..\deploy\vc_redist.x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall; Check: VCRedistNeeded
; Unzips and checks the downloaded engine; extracted to {tmp} on demand (ExtractTemporaryFile).
Source: "Prepare-Stockfish.ps1"; Flags: dontcopy

; Qt files of an earlier version, which this one may not ship: Qt also looks for plugins next to the
; exe, so leftovers could still be loaded. Up to 0.1.3 the plugins and QML were there, not in qt\.
[InstallDelete]
Type: files; Name: "{app}\*.dll"
Type: filesandordirs; Name: "{app}\qt"
Type: filesandordirs; Name: "{app}\generic"
Type: filesandordirs; Name: "{app}\iconengines"
Type: filesandordirs; Name: "{app}\imageformats"
Type: filesandordirs; Name: "{app}\multimedia"
Type: filesandordirs; Name: "{app}\networkinformation"
Type: filesandordirs; Name: "{app}\platforms"
Type: filesandordirs; Name: "{app}\qml"
Type: filesandordirs; Name: "{app}\qmltooling"
Type: filesandordirs; Name: "{app}\styles"
Type: filesandordirs; Name: "{app}\tls"
Type: filesandordirs; Name: "{app}\translations"

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

// --- Microsoft Visual C++ runtime -----------------------------------------------------
// ChessGui.exe needs the runtime it was built with ({#VCRedistVersion}) or newer. The
// bundled vc_redist.x64.exe is unpacked (see [Files] Check) and run only if the installed
// runtime is missing or older; it is never copied into {app}.
const
  VCRuntimeKey = 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64';
  VCRedistUrl = 'https://aka.ms/vc14/vc_redist.x64.exe';

var
  VCRedistRestart: Boolean;

function InstalledVCRuntime(var Version: Int64): Boolean;
var
  Installed, Major, Minor, Bld, Rbld: Cardinal;
begin
  Result := RegQueryDWordValue(HKLM64, VCRuntimeKey, 'Installed', Installed) and (Installed = 1) and
            RegQueryDWordValue(HKLM64, VCRuntimeKey, 'Major', Major) and
            RegQueryDWordValue(HKLM64, VCRuntimeKey, 'Minor', Minor) and
            RegQueryDWordValue(HKLM64, VCRuntimeKey, 'Bld', Bld);
  if not RegQueryDWordValue(HKLM64, VCRuntimeKey, 'Rbld', Rbld) then
    Rbld := 0;
  if Result then
    Version := PackVersionComponents(Major, Minor, Bld, Rbld);
end;

function VCRedistNeeded(): Boolean;
var
  Installed: Int64;
begin
  Result := not InstalledVCRuntime(Installed) or
            (ComparePackedVersion(Installed,
               PackVersionComponents({#VCRedistMajor}, {#VCRedistMinor}, {#VCRedistBld}, {#VCRedistRbld})) < 0);
end;

procedure InstallVCRedist();
var
  Exe: String;
  ResultCode: Integer;
begin
  Exe := ExpandConstant('{tmp}\vc_redist.x64.exe');
  if not FileExists(Exe) then
  begin
    Log('VC++ runtime: {#VCRedistVersion} or newer already installed');
    Exit;
  end;

  WizardForm.StatusLabel.Caption := 'Installing Microsoft Visual C++ Runtime...';
  WizardForm.FilenameLabel.Caption := '';
  if not Exec(Exe, '/install /quiet /norestart', '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
    ResultCode := -1;
  Log('VC++ runtime {#VCRedistVersion}: vc_redist.x64.exe exit code ' + IntToStr(ResultCode));

  case ResultCode of
    0, 1638: ;                      // installed, or a newer version is already there
    3010: VCRedistRestart := True;  // installed, Windows needs a restart
  else
    SuppressibleMsgBox('The Microsoft Visual C++ Runtime could not be installed (exit code ' +
      IntToStr(ResultCode) + ').' + #13#10 + #13#10 +
      'If Chess++ does not start, install it from:' + #13#10 + VCRedistUrl,
      mbError, MB_OK, IDOK);
  end;
end;

function NeedRestart(): Boolean;
begin
  Result := VCRedistRestart;
end;

// --- Stockfish: copy the verified engine and the log into {app}\engines ----------------
procedure InstallStockfish();
var
  EnginesDir: String;
begin
  if WizardIsTaskSelected('stockfish') and (StockfishLog <> '') then
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

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    InstallVCRedist();
    InstallStockfish();
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
