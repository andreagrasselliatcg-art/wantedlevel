; =============================================================================
;  Installer Windows di Wanted Level (Inno Setup 6)
;
;  Compilazione:
;    iscc /DAppVersion=1.0.0 /DArtefacts="..\..\build\WantedLevel_artefacts\Release" WantedLevel.iss
; =============================================================================

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef Artefacts
  #define Artefacts "..\..\build\WantedLevel_artefacts\Release"
#endif

#define AppName      "Wanted Level"
#define AppPublisher "Tune Talks"

[Setup]
AppId={{6B1E9C2A-4F7D-4E8B-9A35-7C2D1F0E5A93}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\{#AppPublisher}\{#AppName}
DefaultGroupName={#AppPublisher}
DisableProgramGroupPage=yes
OutputDir=..\..\dist
OutputBaseFilename=WantedLevel-{#AppVersion}-Windows-Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
UninstallDisplayName={#AppName} ({#AppPublisher})

[Languages]
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full";   Description: "Installazione completa"
Name: "custom"; Description: "Installazione personalizzata"; Flags: iscustom

[Components]
Name: "vst3";       Description: "Plug-in VST3";   Types: full custom; Flags: fixed
Name: "standalone"; Description: "App Standalone"; Types: full

[Files]
; Il VST3 e' una cartella-bundle: va copiata intera in Common Files\VST3
Source: "{#Artefacts}\VST3\{#AppName}.vst3\*"; DestDir: "{commoncf64}\VST3\{#AppName}.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Artefacts}\Standalone\{#AppName}.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#AppPublisher}\{#AppName}"; Filename: "{app}\{#AppName}.exe"; Components: standalone
Name: "{autoprograms}\{#AppPublisher}\Disinstalla {#AppName}"; Filename: "{uninstallexe}"

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\{#AppName}.vst3"
