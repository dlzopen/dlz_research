; Inno Setup Script for dlzr - Landmine War Remake
; 地雷战复刻版安装脚本
;
; Usage: iscc dlzr_installer.iss
; Download Inno Setup: https://jrsoftware.org/isinfo.php

#define MyAppName "dlzr - 地雷战复刻版"
#define MyAppNameEn "dlzr - Landmine War Remake"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "dlzopen"
#define MyAppURL "https://github.com/dlzopen/dlz_research"
#define MyAppExeName "dlzr.exe"

[Setup]
AppId={{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppNameEn}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile=
OutputDir=installer_output
OutputBaseFilename=dlzr_setup_{#MyAppVersion}
SetupIconFile=
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

; 语言选择 / Language selection
ShowLanguageDialog=auto

[Languages]
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 6.1; Check: not IsAdminInstallMode

[Files]
; 主程序 / Main executable
Source: "build\bin\Debug\dlzr.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\bin\Debug\SDL3.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\bin\Debug\SDL3_mixer.dll"; DestDir: "{app}"; Flags: ignoreversion

; 文档 / Documentation
Source: "docs\*.md"; DestDir: "{app}\docs"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion

; 可选：背景音乐 / Optional: Background music
Source: "music\*"; DestDir: "{app}\music"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: DirExists('music')

; 注意：游戏数据文件需要用户自行提供
; Note: Game data files must be provided by the user
[Dirs]
Name: "{app}\music"; Check: not DirExists('{app}\music')

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Code]
// 检查游戏数据文件 / Check for game data files
function InitializeSetup(): Boolean;
var
  DataDir: String;
  ResultCode: Integer;
begin
  Result := True;
  
  // 检查是否已安装游戏数据 / Check if game data is installed
  DataDir := ExpandConstant('{app}');
  
  if not FileExists(DataDir + '\GAME.DAT') then
  begin
    if MsgBox('警告：未找到游戏数据文件（GAME.DAT）。' + #13#10 +
              'dlzr 需要原版游戏的数据文件才能运行。' + #13#10 + #13#10 +
              'Warning: Game data files (GAME.DAT) not found.' + #13#10 +
              'dlzr requires original game data files to run.' + #13#10 + #13#10 +
              '是否继续安装？' + #13#10 +
              'Continue with installation?',
              mbConfirmation, MB_YESNO) = IDNO then
    begin
      Result := False;
    end;
  end;
end;

// 安装后提示 / Post-installation notice
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    if not FileExists(ExpandConstant('{app}\GAME.DAT')) then
    begin
      MsgBox('安装完成！' + #13#10 + #13#10 +
             'Installation complete!' + #13#10 + #13#10 +
             '请将原版游戏的数据文件复制到安装目录：' + #13#10 +
             ExpandConstant('{app}') + #13#10 + #13#10 +
             'Please copy the original game data files to:' + #13#10 +
             ExpandConstant('{app}') + #13#10 + #13#10 +
             '需要的文件 / Required files:' + #13#10 +
             'GAME.DAT, FIGHT.DAT, FACE.DAT, TOWN.DAT, WAVE.DAT, etc.' + #13#10 + #13#10 +
             '可选 / Optional:' + #13#10 +
             '- music/ 目录放置 MP3/OGG 背景音乐' + #13#10 +
             '- music/ directory for MP3/OGG background music',
             mbInformation, MB_OK);
    end;
  end;
end;
