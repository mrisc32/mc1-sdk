# Support

This folder contains various support packages for different development tools.

## CMake toolchain

The CMake toolchain file [cmake/mc1-toolchain.cmake] can be used for building CMake projects for the MC1 computer. Just pass the option `-DCMAKE_TOOLCHAIN_FILE=mc1-toolchain.cmake` to CMake.

## Syntax highlighting for gedit / GtkSourceView

Copy or symlink [gtksourceview/mr32asm.lang] to `~/.local/share/gtksourceview-4/language-specs/` (or `~/.local/share/gtksourceview-3.0/language-specs/` for GtkSourceView 3).
