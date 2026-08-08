# lib/

`src/build.bat` passes `/LIBPATH:"..\..\lib"` to the linker, but as of this commit every
library actually linked (`gdi32.lib`, `user32.lib`, `Opengl32.lib`) is a standard Windows SDK
library the linker finds on its own — nothing here is currently required to build.

This folder exists as a placeholder in case a future third-party `.lib` gets added (the map
viewer sibling project uses this same layout for `libcurl.lib`, for reference).
