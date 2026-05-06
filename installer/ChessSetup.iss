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
ArchitecturesInstallIn64BitMode=x64
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
  EnginesDir, Script: String;
begin
  EnginesDir := ExpandConstant('{app}\engines');
  ForceDirectories(EnginesDir);
  Script :=
    '$rel = (Invoke-RestMethod "https://api.github.com/repos/official-stockfish/Stockfish/releases/latest").tag_name; ' +
    '$url = "https://github.com/official-stockfish/Stockfish/releases/download/$rel/stockfish-windows-x86-64.zip"; ' +
    'Invoke-WebRequest $url -OutFile "$env:TEMP\sf.zip"; ' +
    'Expand-Archive "$env:TEMP\sf.zip" -DestinationPath "$env:TEMP\sf" -Force; ' +
    '$exe = Get-ChildItem "$env:TEMP\sf" -Recurse -Filter "stockfish*.exe" | Select-Object -First 1; ' +
    'Copy-Item $exe.FullName "' + EnginesDir + '\stockfish.exe"';
  if not Exec('powershell.exe',
      '-NoProfile -ExecutionPolicy Bypass -Command "' + Script + '"',
      '', SW_HIDE, ewWaitUntilTerminated, ResultCode) or (ResultCode <> 0) then
    MsgBox(
      'Stockfish could not be downloaded automatically.' + #13#10 +
      'You can get it from https://stockfishchess.org and place it in:' + #13#10 +
      EnginesDir,
      mbInformation, MB_OK);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssDone) and IsTaskSelected('stockfish') then
    DownloadStockfish();
end;
