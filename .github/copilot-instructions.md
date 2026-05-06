---
applyTo: "src/**.c, src/**.h"
---
Treat src/util/** as an outside dependency. Do not suggest code from there. Do not edit any files there.

When doing edits, always respect the file template in .vscode/c.code-snippets. There are separate templates for .h and .c files. The templates contain sections. Put the new code in the correct section of the file. If a section is missing, add it. Keep the sections in the order they are in the template.
When creating a file, add the appropiate template first.

Never check if malloc(), calloc() or realloc() returned NULL. Assume that they always succeed. Do not add error handling for failed memory allocations.

When creating a struct or enum, add doxygen comments to it.
When adding a function, add doxygen comments to the declaration in the header file (public function) or to the declaration in the source file, section "Private Function Declarations" (private function).

Respect the naming convention of public functions, which is `moduleName_functionName` (implemented in moduleName.c).
Do not use the ! operator for boolean conditions. Write condition == false instead.
Do not create functions, variables or defines that are not used.
Never declare multiple variables in the same line.
Make order of operations explicit with parentheses, even if they are not strictly necessary.
