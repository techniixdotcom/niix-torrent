; NiiX Torrent - Windows installer and uninstaller (Inno Setup 7.1 or newer).
;
; Build it with build.cmd (or .\build.ps1 -Target Installer). By hand, after
; the application has been staged to dist\x64:
;
;   iscc /DSourceDir=..\..\dist\x64 /DAppVersion=2.0.0 packaging\windows\NiiXTorrent.iss
;
; Command line switches understood by the generated setup (in addition to the
; standard Inno Setup ones such as /SILENT, /VERYSILENT, /ALLUSERS, /CURRENTUSER,
; /DIR=... and /TASKS=...):
;
;   Uninstaller:  /REMOVEDATA   also delete the user's settings, torrent list and logs
;                 /KEEPDATA     never ask, keep them (default for silent uninstalls)

#ifndef AppVersion
  #define VersionFile FileOpen(AddBackslash(SourcePath) + "..\..\VERSION")
  #define AppVersion Trim(FileRead(VersionFile))
  #expr FileClose(VersionFile)
#endif

#ifndef SourceDir
  #define SourceDir "..\..\dist\x64"
#endif

#ifndef OutputDir
  #define OutputDir "..\..\dist"
#endif

#ifndef Arch
  #define Arch "x64"
#endif

#define AppName      "NiiX Torrent"
#define AppExeName   "NiiXTorrent.exe"
#define AppPublisher "techniix"
#define AppUrl       "https://torrent.niiix.net"
#define AppProgId    "NiiXTorrent"
; Named mutex held by the running application (wxSingleInstanceChecker in
; src/niixtorrent/application.cpp). Lets Setup detect a running instance.
#define AppMutexName "584c8e47-d8a5-4e52-9165-c0650a85723a"

[Setup]
; Identifies the product for upgrades and in "Installed apps". Never change it.
AppId={{6B1F0C59-52F3-4C0B-9E55-2A9D1E7B4D10}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppUrl}
AppSupportURL={#AppUrl}
AppUpdatesURL={#AppUrl}
AppCopyright=Copyright (C) 2026 {#AppPublisher}
VersionInfoVersion={#AppVersion}
VersionInfoProductName={#AppName}
VersionInfoDescription={#AppName} Setup

DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
DisableWelcomePage=no
UsePreviousAppDir=yes
UsePreviousTasks=yes
AllowNoIcons=yes

; Installs for all users by default (elevates), but lets the user pick
; "just me" - on the first page or with /CURRENTUSER - which needs no admin rights.
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog commandline
; The "start when I sign in" task and the optional removal of settings on
; uninstall deliberately act on the profile of the user running Setup.
UsedUserAreasWarning=no

#if Arch == "x64"
SetupArchitecture=x64
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
#else
ArchitecturesAllowed=x86compatible
#endif
MinVersion=10.0.17763

LicenseFile={#SourceDir}\LICENSE
SetupIconFile=..\..\res\app.ico
UninstallDisplayIcon={app}\{#AppExeName}
UninstallDisplayName={#AppName}
WizardStyle=modern dynamic
WizardImageFile=wizard-large.bmp
WizardSmallImageFile=wizard-small.bmp
WizardSizePercent=110

Compression=lzma2/max
SolidCompression=yes
OutputDir={#OutputDir}
OutputBaseFilename=NiiXTorrent-{#AppVersion}-{#Arch}-setup

; Offer to close a running NiiX Torrent, and start it again afterwards when
; the user is upgrading.
AppMutex={#AppMutexName}
CloseApplications=yes
RestartApplications=yes
; Associations are registered below.
ChangesAssociations=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "dutch"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "portuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "turkish"; MessagesFile: "compiler:Languages\Turkish.isl"
Name: "ukrainian"; MessagesFile: "compiler:Languages\Ukrainian.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"

[CustomMessages]
TaskAssociations=File associations:
TaskAssocTorrent=Open .torrent files with {#AppName}
TaskAssocMagnet=Open magnet links with {#AppName}
TaskOther=Other:
TaskFirewall=Allow {#AppName} through Windows Firewall
TaskAutoStart=Start {#AppName} when I sign in to Windows
FirewallRuleName={#AppName}
RemoveDataPrompt=Do you also want to remove your {#AppName} settings, torrent list and logs?%n%nDownloaded files are never touched. Choose No if you plan to install {#AppName} again.
RemovingOldVersion=Removing the previous version of {#AppName}...

german.TaskAssociations=Dateizuordnungen:
german.TaskAssocTorrent=.torrent-Dateien mit {#AppName} öffnen
german.TaskAssocMagnet=Magnet-Links mit {#AppName} öffnen
german.TaskOther=Sonstiges:
german.TaskFirewall={#AppName} in der Windows-Firewall zulassen
german.TaskAutoStart={#AppName} bei der Windows-Anmeldung starten
german.RemoveDataPrompt=Sollen auch Ihre {#AppName}-Einstellungen, die Torrent-Liste und die Protokolle entfernt werden?%n%nHeruntergeladene Dateien bleiben immer erhalten. Wählen Sie Nein, wenn Sie {#AppName} erneut installieren möchten.
german.RemovingOldVersion=Die vorherige Version von {#AppName} wird entfernt...

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "assoctorrent"; Description: "{cm:TaskAssocTorrent}"; GroupDescription: "{cm:TaskAssociations}"
Name: "assocmagnet"; Description: "{cm:TaskAssocMagnet}"; GroupDescription: "{cm:TaskAssociations}"
Name: "firewall"; Description: "{cm:TaskFirewall}"; GroupDescription: "{cm:TaskOther}"; Check: IsAdminInstallMode
Name: "autostart"; Description: "{cm:TaskAutoStart}"; GroupDescription: "{cm:TaskOther}"; Flags: unchecked

[Files]
; Everything produced by "cmake --install": the application, its translation
; database, plugins, the Visual C++ runtime DLLs and (when built with crash
; reporting) crashpad_handler.exe.
Source: "{#SourceDir}\{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\*"; Excludes: "{#AppExeName},*.pdb,*.lib,*.exp,*.ilk"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExeName}"; AppUserModelID: "techniix.NiiXTorrent"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Registry]
; --- Installation marker read by the application (installed vs. portable mode)
Root: HKA; Subkey: "Software\NiiXTorrent"; ValueType: string; ValueName: "InstallDirectory"; ValueData: "{app}"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\NiiXTorrent"; ValueType: string; ValueName: "Version"; ValueData: "{#AppVersion}"

; --- ProgId shared by the .torrent association and the magnet handler
Root: HKA; Subkey: "Software\Classes\{#AppProgId}"; ValueType: string; ValueName: ""; ValueData: "{#AppName} BitTorrent file"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\{#AppProgId}"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "BitTorrent file"
Root: HKA; Subkey: "Software\Classes\{#AppProgId}\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"",1"
Root: HKA; Subkey: "Software\Classes\{#AppProgId}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"" ""%1"""

; --- "Open with" entry
Root: HKA; Subkey: "Software\Classes\Applications\{#AppExeName}"; ValueType: string; ValueName: "FriendlyAppName"; ValueData: "{#AppName}"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\Applications\{#AppExeName}\SupportedTypes"; ValueType: string; ValueName: ".torrent"; ValueData: ""
Root: HKA; Subkey: "Software\Classes\Applications\{#AppExeName}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"" ""%1"""

; --- .torrent files
Root: HKA; Subkey: "Software\Classes\.torrent\OpenWithProgids"; ValueType: string; ValueName: "{#AppProgId}"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\.torrent"; ValueType: string; ValueName: ""; ValueData: "{#AppProgId}"; Flags: uninsdeletevalue; Tasks: assoctorrent
Root: HKA; Subkey: "Software\Classes\.torrent"; ValueType: string; ValueName: "Content Type"; ValueData: "application/x-bittorrent"; Tasks: assoctorrent

; --- magnet: links
Root: HKA; Subkey: "Software\Classes\magnet"; ValueType: string; ValueName: ""; ValueData: "URL:BitTorrent magnet link"; Tasks: assocmagnet
Root: HKA; Subkey: "Software\Classes\magnet"; ValueType: string; ValueName: "URL Protocol"; ValueData: ""; Tasks: assocmagnet
Root: HKA; Subkey: "Software\Classes\magnet\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"",0"; Tasks: assocmagnet
Root: HKA; Subkey: "Software\Classes\magnet\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"" ""%1"""; Tasks: assocmagnet

; --- Windows "Default apps" registration
Root: HKA; Subkey: "Software\NiiXTorrent\Capabilities"; ValueType: string; ValueName: "ApplicationName"; ValueData: "{#AppName}"
Root: HKA; Subkey: "Software\NiiXTorrent\Capabilities"; ValueType: string; ValueName: "ApplicationDescription"; ValueData: "A fast, lightweight BitTorrent client"
Root: HKA; Subkey: "Software\NiiXTorrent\Capabilities"; ValueType: string; ValueName: "ApplicationIcon"; ValueData: """{app}\{#AppExeName}"",0"
Root: HKA; Subkey: "Software\NiiXTorrent\Capabilities\FileAssociations"; ValueType: string; ValueName: ".torrent"; ValueData: "{#AppProgId}"
Root: HKA; Subkey: "Software\NiiXTorrent\Capabilities\MIMEAssociations"; ValueType: string; ValueName: "application/x-bittorrent"; ValueData: "{#AppProgId}"
Root: HKA; Subkey: "Software\NiiXTorrent\Capabilities\UrlAssociations"; ValueType: string; ValueName: "magnet"; ValueData: "{#AppProgId}"
Root: HKA; Subkey: "Software\RegisteredApplications"; ValueType: string; ValueName: "{#AppName}"; ValueData: "Software\NiiXTorrent\Capabilities"; Flags: uninsdeletevalue

; --- App Paths: lets "NiiXTorrent" be started from Win+R
Root: HKA; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\{#AppExeName}"; ValueType: string; ValueName: ""; ValueData: "{app}\{#AppExeName}"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\{#AppExeName}"; ValueType: string; ValueName: "SupportedProtocols"; ValueData: "magnet"

; --- Start with Windows (the same value the application's own setting uses)
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "NiiXTorrent"; ValueData: """{app}\{#AppExeName}"""; Flags: uninsdeletevalue; Tasks: autostart

[Run]
; Inbound rule so peers can connect. Removed again by the uninstaller.
Filename: "{sys}\netsh.exe"; Parameters: "advfirewall firewall delete rule name=""{cm:FirewallRuleName}"" program=""{app}\{#AppExeName}"""; Flags: runhidden; Tasks: firewall
Filename: "{sys}\netsh.exe"; Parameters: "advfirewall firewall add rule name=""{cm:FirewallRuleName}"" dir=in action=allow program=""{app}\{#AppExeName}"" enable=yes profile=any"; Flags: runhidden; Tasks: firewall
Filename: "{app}\{#AppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(AppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{sys}\netsh.exe"; Parameters: "advfirewall firewall delete rule name=""{cm:FirewallRuleName}"" program=""{app}\{#AppExeName}"""; Flags: runhidden; RunOnceId: "RemoveFirewallRule"; Check: IsAdminInstallMode

[UninstallDelete]
; Files the application creates next to itself.
Type: files; Name: "{app}\*.log"
Type: dirifempty; Name: "{app}"

[Code]
const
  // Product name shown by the previous Windows Installer (MSI) based setup.
  LegacyMsiDisplayName = 'NiiX Torrent';

// ---------------------------------------------------------------------------
// Upgrade from the old WiX/MSI installer: find its product code and remove it
// so the two installations do not end up side by side.
// ---------------------------------------------------------------------------
function FindLegacyMsiProductCode(const RootKey: Integer; var ProductCode: String): Boolean;
var
  Names: TArrayOfString;
  I: Integer;
  Key, DisplayName: String;
  IsMsi: Cardinal;
begin
  Result := False;
  Key := 'Software\Microsoft\Windows\CurrentVersion\Uninstall';

  if not RegGetSubkeyNames(RootKey, Key, Names) then
    Exit;

  for I := 0 to GetArrayLength(Names) - 1 do
  begin
    // MSI products are registered under their {GUID} product code.
    if (Length(Names[I]) = 38) and (Names[I][1] = '{') then
    begin
      if RegQueryStringValue(RootKey, Key + '\' + Names[I], 'DisplayName', DisplayName)
        and (CompareText(DisplayName, LegacyMsiDisplayName) = 0)
        and RegQueryDWordValue(RootKey, Key + '\' + Names[I], 'WindowsInstaller', IsMsi)
        and (IsMsi = 1) then
      begin
        ProductCode := Names[I];
        Result := True;
        Exit;
      end;
    end;
  end;
end;

procedure RemoveLegacyMsiInstallation;
var
  ProductCode: String;
  ResultCode: Integer;
  Found: Boolean;
begin
  Found := False;

  if IsWin64 then
    Found := FindLegacyMsiProductCode(HKLM64, ProductCode);
  if not Found then
    Found := FindLegacyMsiProductCode(HKLM32, ProductCode);

  if not Found then
    Exit;

  // Removing a per-machine MSI needs administrator rights.
  if not IsAdminInstallMode then
  begin
    Log('A previous MSI installation was found but cannot be removed without administrator rights: ' + ProductCode);
    Exit;
  end;

  Log('Removing previous MSI installation ' + ProductCode);
  WizardForm.StatusLabel.Caption := CustomMessage('RemovingOldVersion');

  // SKIPUSERDATA is not a property of the old package; its uninstall removed
  // the per-user data folder, so keep a copy and put it back afterwards.
  if DirExists(ExpandConstant('{localappdata}\NiiX Torrent')) then
    RenameFile(ExpandConstant('{localappdata}\NiiX Torrent'), ExpandConstant('{localappdata}\NiiX Torrent.setup-backup'));

  if not Exec(ExpandConstant('{sys}\msiexec.exe'), '/x ' + ProductCode + ' /qn /norestart', '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
    Log('msiexec could not be started: ' + SysErrorMessage(ResultCode))
  else
    Log('msiexec finished with exit code ' + IntToStr(ResultCode));

  if DirExists(ExpandConstant('{localappdata}\NiiX Torrent.setup-backup')) then
  begin
    if DirExists(ExpandConstant('{localappdata}\NiiX Torrent')) then
      DelTree(ExpandConstant('{localappdata}\NiiX Torrent'), True, True, True);
    RenameFile(ExpandConstant('{localappdata}\NiiX Torrent.setup-backup'), ExpandConstant('{localappdata}\NiiX Torrent'));
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssInstall then
    RemoveLegacyMsiInstallation;
end;

// ---------------------------------------------------------------------------
// Uninstall: the program files, shortcuts, associations, firewall rule and
// start-up entry always go. The user's own data is only removed on request.
// ---------------------------------------------------------------------------
function CommandLineHasSwitch(const Name: String): Boolean;
var
  I: Integer;
begin
  Result := False;
  for I := 1 to ParamCount do
    if CompareText(ParamStr(I), '/' + Name) = 0 then
    begin
      Result := True;
      Exit;
    end;
end;

function ShouldRemoveUserData: Boolean;
begin
  if CommandLineHasSwitch('REMOVEDATA') then
    Result := True
  else if CommandLineHasSwitch('KEEPDATA') or UninstallSilent then
    Result := False
  else
    Result := MsgBox(CustomMessage('RemoveDataPrompt'), mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES;
end;

// The magnet: handler is shared between BitTorrent clients. Only remove it if
// it still points at this installation - another client may have taken it over.
procedure RemoveMagnetHandlerIfOurs;
var
  Command: String;
begin
  if RegQueryStringValue(HKA, 'Software\Classes\magnet\shell\open\command', '', Command) then
  begin
    if Pos(Lowercase(ExpandConstant('{app}\{#AppExeName}')), Lowercase(Command)) > 0 then
    begin
      Log('Removing the magnet link handler');
      RegDeleteKeyIncludingSubkeys(HKA, 'Software\Classes\magnet');
    end
    else
      Log('The magnet link handler belongs to another program - left untouched');
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  DataDir: String;
begin
  if CurUninstallStep = usUninstall then
  begin
    RemoveMagnetHandlerIfOurs;

    // Decide before anything is removed, while the dialog still has context.
    if ShouldRemoveUserData then
    begin
      // %LOCALAPPDATA%\NiiX Torrent: database, logs and crash reports.
      DataDir := ExpandConstant('{localappdata}\NiiX Torrent');
      if DirExists(DataDir) then
      begin
        Log('Removing user data in ' + DataDir);
        DelTree(DataDir, True, True, True);
      end;

      RegDeleteKeyIncludingSubkeys(HKCU, 'Software\NiiXTorrent');
    end;

    // Set from Preferences > General > "Start with Windows".
    RegDeleteValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run', 'NiiXTorrent');
  end;
end;
