[Setup]
AppName=Image viewer
AppVersion=1.0
DefaultDirName=C:/Program Files/Image loader

[Dirs]
Name:"{app}/licenses";

[Files]
Source: "./x64/Release/SDL_Image_loader.exe"; DestDir: "{app}";
Source: "./x64/Release/*"; DestDir: "{app}"; Excludes: "*.pdb,*.ipdb,*.iobj";
Source: "./x64/Release/licenses/*"; DestDir: "{app}/licenses";