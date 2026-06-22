INSTRUCTIONS for making an app package:

1) Copy loose build output files from GameSave\XDK\Durango\Layout\Image\Loose
2) From XDK command prompt in this directory, run makepkg-pack.bat
   OPTIONAL: Specify content ID as first parameter if you want to use a fixed ID for content update scenarios
3) Output is placed in GameSave\AppPackages\GameSaveXDK

If you need to test a package with two apps (e.g. debug and release builds), use AppxManifest.xml.twoApps as a guide. (Remember to use the pack command to create an updated appdata.bin after manually editing AppxManifest.xml.)
