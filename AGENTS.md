# Working in this repository

Use `Stack/` as the main reference for coding style. Match the surrounding code in the file you edit; older files vary in spacing and copyright headers. Keep changes focused and do not reformat unrelated code.

## File layout

- Put the implementation in the file named for its operation. `Stack/` commonly uses `Alloc.c`, `Free.c`, `Lock.c`, `Valid.c`, and similar small files. For host controllers, put transfer construction in `*_Build.c`, queue insertion in `*_Add.c`, unlinking in `*_Remove.c`, and result accounting in `*_Length.c`. Do not collect these implementations in a central `Core.c` or leave operation files as forwarding wrappers.
- Keep shared helpers in their own files only when more than one operation uses them. Check the local `.h` file and `usb2_all.h` for existing declarations and macros before adding new ones.
- Some source files deliberately `#include` nearby `.c` files. Follow the existing build arrangement and avoid defining the same function in two compilation units.

## C style

- Start every new C source file with the exact layout below: one leading blank line, the copyright and SPDX header, a blank line, `// --`, a blank line, the includes, a blank line, and another `// --` followed by a blank line. Replace `[INCLUDES]` with the file's actual `#include` directives. Preserve existing notices when copied or derived code requires them. Do not reformat unrelated older files solely to apply this rule.

```c

/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

[INCLUDES]

// --

```

- Keep the `// --` section markers used around functions.
- Put a function's return type, including `static` or section qualifiers, on its own line. Put the function name and parameter list on the next line, as in `S32` followed by `Dot4_Write( ... )`. Apply this to new functions without reformatting unrelated older code.
- Use tabs for indentation. Put each opening and closing brace on its own line, with the opening brace at the same indentation as its `if`, `while`, or other control statement. Follow local alignment rather than applying a formatter to the whole file.
- For a null or false check, prefer `if ( ! xx )` over `if ( xx == NULL )`. Put parentheses around each part of a compound `if` or `while` condition, as in `if ( ( xx ) && ( yy ) )` or `while( ( xx ) && ( yy ) || ( zz ) )`.
- Leave one blank line before an `if` when it follows another statement, including an assignment. An `if` at the start of a block can follow the opening brace directly. Leave one blank line after the completed `if` block before the next statement. Keep an attached `else` or `else if` directly after its `if` block. Also separate successive `while` blocks and other completed control-flow blocks from the next logical step.
- Put a blank line before and after every statement that calls a function when it is next to another statement, including a call assigned to a variable or used in an `if` condition. No blank line is needed immediately inside an opening or closing brace. Keep consecutive assignments without calls together.
- Write value returns with an outer pair of parentheses and spaces: `return ( TRUE );`, `return ( value );`, or `return ( GetValue() );`. Keep plain `return;` in `void` functions.
- Leave one blank line before a `return` when it follows another statement or a completed block. A `return` that is the first statement inside `{` does not need a blank line.
- Declare one variable per line. For example, write `U32 status;`, `U32 wanted;`, and `U32 actual;` on separate lines instead of `U32 status, wanted, actual;`.
- At the start of each function or block, declare local `struct` and `enum` variables first, including pointers to those types. Leave one blank line before the remaining declarations, such as `U8`, `U32`, and `S32`.
- Declare locals without initializers. After all declarations, leave one blank line before assigning initial values. Leave one more blank line between those assignments and the rest of the code. Skip groups that have no entries.

```c
if ( ! xx )
{
	return ( FALSE );
}

if ( ( xx ) && ( yy ) )
{
	DoSomething();
}

while( ( xx ) && ( yy ) || ( zz ) )
{
	DoMore();
}
```
- Use the project's types (`U8`, `U16`, `U32`, `S32`, `PTR`, and its structs), `TRUE`/`FALSE`, and its existing names. Functions normally use `SEC_CODE`; section data uses the corresponding section macro.
- Declare local variables at the start of the function or block, as the surrounding code does. Use descriptive names already established in that subsystem (`usbbase`, `hn`, `ioreq`, `ep`, `fn`).
- Keep comments short and explain hardware constraints, ownership, or unusual control flow. Preserve existing diagnostic calls such as `USBDEBUG`, `USBERROR`, `USBPANIC`, and `TASK_NAME_*` where applicable.

## Behavior and verification

- Follow the established allocation and cleanup pattern: check each allocation, release owned resources on failure, and use the local `bailout:` pattern where appropriate. Preserve lock, semaphore, message, and request lifecycle ordering.
- For controller code, keep hardware descriptor fields, physical addresses, endian conversions (`LE_SWAP16`/`LE_SWAP32`), and DMA buffer ownership explicit. Use OHCI/EHCI and the relevant `Stack/HCD/` code as references, while respecting controller-specific hardware rules.
- Build with `make -j4` after C changes when the AmigaOS cross compiler is available. Report build results separately from runtime validation; a successful link does not establish hardware behavior.
