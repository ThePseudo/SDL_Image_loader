#define AppName "Image viewer"

[Setup]
AppName = {#AppName}
AppVersion = 1.0
DefaultDirName = C:/Program Files (x86)/Image loader
ChangesAssociations = yes

[Dirs]
Name:"{app}/licenses";

[Files]
Source: "./Release/SDL_Image_loader.exe"; DestDir: "{app}";
Source: "./Release/*"; DestDir: "{app}"; Excludes: "*.pdb,*.ipdb,*.iobj";
Source: "./Release/licenses/*"; DestDir: "{app}/licenses";