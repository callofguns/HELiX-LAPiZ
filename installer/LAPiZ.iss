; Windows installer for LAPiZ, built with Inno Setup 6 (https://jrsoftware.org/isinfo.php).
; Build the plugin in Release first, then run from the project folder:
;   "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" installer\LAPiZ.iss
; The finished installer is written to dist\.
;
; Keep AppVersion in step with Source/Version.h and the VERSION in CMakeLists.txt.
; Never change AppId: Windows uses it to recognise an older LAPiZ when updating or uninstalling.

#define AppVersion "1.1.0"
#define Artefacts "..\build\Lapiz_artefacts\Release"

[Setup]
AppId={{8AC7DA59-70F4-4FBF-BF06-E7911CD5DCFE}
AppName=LAPiZ
AppVersion={#AppVersion}
AppVerName=LAPiZ {#AppVersion}
AppPublisher=HELiX
AppPublisherURL=https://github.com/callofguns/HELiX-LAPiZ
AppSupportURL=https://github.com/callofguns/HELiX-LAPiZ/issues
DefaultDirName={autopf}\HELiX\LAPiZ
DefaultGroupName=HELiX
DisableProgramGroupPage=yes
LicenseFile=..\LICENSE
OutputDir=..\dist
OutputBaseFilename=LAPiZ-{#AppVersion}-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
; The VST3 folder is shared by every plugin on the computer, so installing needs administrator rights.
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayName=LAPiZ (HELiX)
UninstallDisplayIcon={app}\LAPiZ.exe

[Types]
Name: "full"; Description: "Plugin and standalone app"
Name: "plugin"; Description: "Plugin only (VST3)"
Name: "custom"; Description: "Choose what to install"; Flags: iscustom

[Components]
Name: "vst3"; Description: "LAPiZ VST3 plugin (for FL Studio and other music software)"; Types: full plugin custom; Flags: fixed
Name: "standalone"; Description: "LAPiZ standalone app (plays without FL Studio)"; Types: full

[Files]
; The VST3 plugin goes in the standard VST3 folder that FL Studio scans: C:\Program Files\Common Files\VST3
Source: "{#Artefacts}\VST3\LAPiZ.vst3\*"; DestDir: "{commoncf64}\VST3\LAPiZ.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Artefacts}\Standalone\LAPiZ.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion
Source: "..\THIRD_PARTY_NOTICES.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\LAPiZ"; Filename: "{app}\LAPiZ.exe"; Components: standalone

[Messages]
FinishedLabel=LAPiZ is installed.%n%nIn FL Studio, open Options > Manage plugins, click Find installed plugins, then star LAPiZ to add it to your plugin list.
