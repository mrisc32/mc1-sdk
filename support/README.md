# Support

This folder contains various support packages for different development tools.

## CMake toolchain

The CMake toolchain file [toolchain-mc1.cmake](cmake/toolchain-mc1.cmake) can be used for building CMake projects for the MC1 computer. Just pass the option `-DCMAKE_TOOLCHAIN_FILE=toolchain-mc1.cmake` to CMake.

## Syntax highlighting for gedit / GtkSourceView

Copy or symlink [mr32asm.lang](gtksourceview/mr32asm.lang) to `~/.local/share/gtksourceview-4/language-specs/` (or `~/.local/share/gtksourceview-3.0/language-specs/` for GtkSourceView 3).
