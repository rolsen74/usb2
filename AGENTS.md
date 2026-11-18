# Working in this repository

Use `Stack/` as the main reference for coding style. Match the surrounding code in the file you edit; older files vary in spacing and copyright headers. Keep changes focused and do not reformat unrelated code.

## File layout

- Put the implementation in the file named for its operation. `Stack/` commonly uses `Alloc.c`, `Free.c`, `Lock.c`, `Valid.c`, and similar small files. For host controllers, put transfer construction in `*_Build.c`, queue insertion in `*_Add.c`, unlinking in `*_Remove.c`, and result accounting in `*_Length.c`. Do not collect these implementations in a central `Core.c` or leave operation files as forwarding wrappers.
- Keep shared helpers in their own files only when more than one operation uses them. Check the local `.h` file and `usb2_all.h` for existing declarations and macros before adding new ones.
- Some source files deliberately `#include` nearby `.c` files. Follow the existing build arrangement and avoid defining the same function in two compilation units.

## C style

- Preserve the file's existing copyright and license header. New files should use the current `Stack/` header with the GPL SPDX line, unless copied or derived code requires its existing notice. Keep the `// --` section markers used around includes and functions.
- Use tabs for indentation. Put each opening and closing brace on its own line, with the opening brace at the same indentation as its `if`, `while`, or other control statement. Follow local alignment rather than applying a formatter to the whole file.
- For a null or false check, prefer `if ( ! xx )` over `if ( xx == NULL )`. Put parentheses around each part of a compound `if` or `while` condition, as in `if ( ( xx ) && ( yy ) )` or `while( ( xx ) && ( yy ) || ( zz ) )`.
- Add a blank line between successive `if` or `while` blocks, and between a completed control-flow block and the next logical step. Use these blank lines as natural separators while keeping related statements together.
- Use a blank line around a function call when it starts or ends a distinct step. In a controller queue operation, separate `UHCI_Chip_Stop ( hn );` from the descriptor edits and separate those edits from `UHCI_Chip_Start ( hn );`. Keep consecutive assignments that form one step together.
- Write value returns with an outer pair of parentheses and spaces: `return ( TRUE );`, `return ( value );`, or `return ( GetValue() );`. Keep plain `return;` in `void` functions.
- Leave one blank line before a `return` when it follows another statement or a completed block. A `return` that is the first statement inside `{` does not need a blank line.
- Declare one variable per line. For example, write `U32 status;`, `U32 wanted;`, and `U32 actual;` on separate lines instead of `U32 status, wanted, actual;`.
- Group local `struct` declarations together and leave one blank line after the last `struct` declaration before `U8`, `U32`, `S32`, or other local declarations.
- Keep declarations separate from initialization. Declare locals together at the start of the block, then assign initial values immediately after that declaration section. Use `struct UHCI_TD * last;` followed by `last = NULL;` after the declarations, rather than `struct UHCI_TD * last = NULL;`.

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
