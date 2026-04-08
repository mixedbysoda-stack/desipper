[Setup]
AppName=De-Sipper
AppVersion=1.0.0
AppVerName=De-Sipper v1.0.0
AppPublisher=Carbonated Audio
DefaultDirName={commonpf64}\Carbonated Audio\De-Sipper
DefaultGroupName=Carbonated Audio
OutputBaseFilename=De-Sipper-v1.0.0-Windows-Installer
OutputDir=..\..\dist\Windows\Installer
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
UninstallDisplayName=De-Sipper
WizardStyle=modern
DisableProgramGroupPage=yes
PrivilegesRequired=admin

[Types]
Name: "full"; Description: "Full installation (VST3 + Standalone)"
Name: "vst3only"; Description: "VST3 plugin only"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "De-Sipper VST3 Plugin"; Types: full vst3only custom; Flags: fixed
Name: "standalone"; Description: "De-Sipper Standalone App"; Types: full custom

[Files]
Source: "..\..\dist\Windows\VST3\Sipper.vst3\*"; DestDir: "{commonpf64}\Common Files\VST3\Sipper.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\dist\Windows\Standalone\Sipper.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

[Icons]
Name: "{group}\De-Sipper"; Filename: "{app}\Sipper.exe"; Components: standalone
Name: "{group}\Uninstall De-Sipper"; Filename: "{uninstallexe}"
Name: "{commondesktop}\De-Sipper"; Filename: "{app}\Sipper.exe"; Components: standalone; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Components: standalone; GroupDescription: "Additional options:"

[Run]
Filename: "{app}\Sipper.exe"; Description: "Launch De-Sipper"; Flags: nowait postinstall skipifsilent; Components: standalone

[Messages]
WelcomeLabel2=This will install De-Sipper v1.0.0 by Carbonated Audio on your computer.%n%nThe VST3 plugin will be installed to the standard VST3 folder so your DAW can find it automatically.

[Code]
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    MsgBox('De-Sipper installed successfully!' + #13#10 + #13#10 +
           'VST3 plugin installed to:' + #13#10 +
           ExpandConstant('{commonpf64}') + '\Common Files\VST3\' + #13#10 + #13#10 +
           'Open your DAW and rescan plugins to use De-Sipper.',
           mbInformation, MB_OK);
  end;
end;
