# GWD coding style

These guidelines describe the conventions used in GWD's own C sources and
headers. Follow them for new code and code being changed. Preserve the style of
bundled third-party libraries; do not reformat unrelated code.

## Blank lines and logical grouping

Use blank lines deliberately to make the sequence of operations easy to read.
GWD generally uses more vertical spacing than compact C code.

- Separate successive steps: obtain a value, validate it, calculate a result,
  update state, and clean up. A step can be a single statement.
- Keep closely related declarations or assignments together, such as dimensions,
  paired counters, or initialization of one object's related fields.
- Declare variables near their first use. Normally separate a declaration or a
  group of declarations from the following operation with a blank line.
- Give checks their own space. Related `HARDBUG` checks may form a group, with a
  blank line before and after the group.
- Separate cleanup operations and the final return so that the exit path is
  easy to follow.
- Group struct members, constants, and function prototypes by purpose, with
  blank lines between groups.
- Normally use one blank line, not multiple consecutive blank lines. Do not add
  a blank line mechanically after every statement; follow the logical grouping
  and the surrounding code.

For example:

```c
int my_breadline(bstring arg_line, FILE *arg_file)
{
  int result = bassigngets(arg_line, my_bgetc, arg_file, '\n');

  HARDBUG(result == BSTR_ERR)
  HARDBUG(ferror(arg_file))

  return (result == 0);
}
```

## Layout

- Use two-space indentation and Allman braces: opening braces go on their own
  lines.
- Follow `.clang-format` for wrapping and alignment, including its 80-column
  limit and two-space continuation indentation.
- When an argument or parameter list wraps, put arguments or parameters on
  separate lines rather than packing several onto a continuation line.
- Simple single-statement conditionals and loops may omit braces, as in the
  surrounding code.
- Preserve intentional include ordering.
- Clang-format handles mechanical layout. The author must supply the blank
  lines that express logical grouping; clang-format cannot infer those groups.

## Names

- Use descriptive names consistent with the surrounding code.
- Prefix function parameters with `arg_`, for example `arg_nrows`, `arg_file`,
  and `arg_with_search`.
- Prefer descriptive loop indices such as `irow`, `jcolumn`, `ioutput`,
  `imove`, and `isort` over generic `i`, `j`, or `k`.
- Follow the existing `n...` convention for counts, such as `nrows` and `nmoves`.
- Use `with_...` for local pointers to the object currently being handled where
  that matches the surrounding code.
- Preserve type-specific struct-member prefixes, such as `S_`, `B_`, `TC_`,
  and `MP_`. Follow the existing prefix when extending a struct.
- Use the established `construct_`, `configure_`, `return_`, and `destroy_`
  naming conventions when extending those interfaces.

## Expressions, checks, and control flow

- Use explicit parentheses around individual comparisons in compound
  conditions. Do not rely on operator precedence to explain the expression.
- Follow GWD's use of `and`, `or`, `TRUE`, `FALSE`, and `INVALID` where
  appropriate. Use a named sentinel rather than a literal value that happens to
  represent it.
- Prefer parenthesized return expressions, such as `return (result);`.
- Use `HARDBUG` for violated invariants and failed operations that must succeed.
  Its condition describes the error, not success.
- Use `my_sscanf` for parsing in the established style, checking the conversion
  count with `HARDBUG` when the input is required to parse successfully.
- Follow the existing statement-macro syntax: `HARDBUG`, `BSTRING`, and
  `BDESTROY` are written without a trailing semicolon.

```c
HARDBUG((arg_nrows < 1) or (arg_ncols < 1))

if ((arg_type == TYPE_FIRST) or (arg_type == TYPE_SECOND))
{
  process_type(arg_type);
}
```

## Bstrings and ownership

- Use bstrings for text that can grow rather than fixed-size general-purpose
  character buffers.
- Use `BSTRING(name)` and `BDESTROY(name)` for temporary bstrings. These macros
  participate in the DEBUG name-stack checks.
- Destroy temporary bstrings in reverse construction order. Check every early
  return, `break`, `continue`, and `goto` for skipped cleanup or a name-stack
  mismatch.
- Create semi-permanent or object-owned bstrings using a checked allocation,
  commonly `HARDBUG((object->member = bfromcstr("")) == NULL)`. Release them
  with a checked `bdestroy` when their owner's lifetime ends. Do not use the
  temporary-string macros for these fields.
- Check bstring operation results using the documented return convention.
  Typically, failure is `BSTR_ERR`; constructors generally return `NULL` on
  failure.
- Prefer bstring-to-bstring operations such as `bassign` when both operands are
  bstrings. Use `bdata` when calling an interface that expects a C string.
- A pointer returned by `bdata` is borrowed. Do not free it, and do not retain
  it across an operation that may reallocate or destroy its bstring.
- If an external API writes directly into a bstring's data, reserve enough
  capacity first and maintain both its logical length and trailing NUL.
- Distinguish replacing from appending. For example, `breada` appends the whole
  stream, so a destination used to load a fresh file must start empty. Check
  file errors separately where the bstring API does not distinguish them from
  EOF.

```c
BSTRING(bname)

HARDBUG(bassigncstr(bname, arg_name) == BSTR_ERR)

process_name(bdata(bname));

BDESTROY(bname)
```

## Comments and changes

- Use short comments to explain intent, invariants, ownership, or a non-obvious
  choice. Avoid comments that merely restate the next statement.
- Match the surrounding code when several layouts are reasonable.
- Keep functional changes and unrelated formatting changes separate. Do not
  rewrite older code solely to make every historical variation uniform.
