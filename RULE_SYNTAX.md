# Rule Syntax

## Prerequisites

If you are not familiar with p-code, start with Ghidra's
[p-code reference](https://ghidra.re/ghidra_docs/languages/html/pcoderef.html).

Ghidra does not provide a built-in high p-code view, so the
[HighPcodeGraphViewer](https://github.com/osogi/HighPcodeGraphViewer) project
can be useful while writing or debugging rules.

## Rule

Pcode Weaver source rules describe a p-code pattern and the rewrite actions to
apply when that pattern is found.

```text
pattern_1;
pattern_2;
--
action_1;
action_2
```

Whitespace and newlines are insignificant. Semicolons separate statements.

The scanner does not currently support comments. `#` is used for constants:
`#0`, `#-1`, `#0x10`.

---

## Identifiers

- `v...` or `V...` - varnode, for example `v_src`
- `o...` or `O...` - p-code operation, also called pnode, for example `o_add`
- `b...` or `B...` - basic block, for example `bb_entry`
- `#123`, `#-1`, `#0x10` - constant varnodes
- other alphabetic names - you could use as size variables, for example `word_size`

---

## Operations

Rules use Ghidra p-code opcode names:

```text
COPY LOAD STORE BRANCH CBRANCH BRANCHIND CALL CALLIND USERDEFINED RETURN
PIECE SUBPIECE POPCOUNT LZCOUNT
INT_EQUAL INT_NOTEQUAL INT_LESS INT_SLESS INT_LESSEQUAL INT_SLESSEQUAL
INT_ZEXT INT_SEXT INT_ADD INT_SUB INT_CARRY INT_SCARRY INT_SBORROW
INT_2COMP INT_NEGATE INT_XOR INT_AND INT_OR INT_LEFT INT_RIGHT INT_SRIGHT
INT_MULT INT_DIV INT_REM INT_SDIV INT_SREM
BOOL_NEGATE BOOL_XOR BOOL_AND BOOL_OR
FLOAT_EQUAL FLOAT_NOTEQUAL FLOAT_LESS FLOAT_LESSEQUAL FLOAT_NAN
FLOAT_ADD FLOAT_SUB FLOAT_MULT FLOAT_DIV FLOAT_NEG FLOAT_ABS FLOAT_SQRT
FLOAT_CEIL FLOAT_FLOOR FLOAT_ROUND
INT2FLOAT FLOAT2FLOAT TRUNC CPOOLREF NEW
```

---

## Pattern Terms

Varnodes:

```text
v_name
v_name(size, bb)
v_name(_, _)             // `_` is a wildcard for any size or basic block
v_name(size, bb, offset)
VEMPTY
#0
```

Pnodes:

```text
o_name
o_name(OPCODE, bb) 
o_name(OPCODE, _)        // `_` is a wildcard for any basic block
o_name(_, bb)            // ANY opcode wildcard: match any pnode in bb
OPEMPTY
```

Be careful with `o_name(_, bb)`: it only binds the matched pnode for actions;
it will not add opcode-specific input/output checks.

Basic blocks:

```text
bb_name
```

---

## Pattern Edges

### DFG
Use `->` for data-flow relationships:

```text
#0 -> (1) o_cmp(INT_NOTEQUAL, bb_cond)
o_cmp -> v_tmp
v_tmp -> (1) o_branch(CBRANCH, bb_cond)
```

Meaning:

- `v -> (N) o` - varnode is input `N` of the pnode
- `o -> v` - pnode defines the varnode

### CFG
Use `->` for control-flow relationships, and `<=` for dominance relationships:

```text
bb_cond -> (0) bb_body
bb_cond <= bb_body
```

- `bb -> (N) bb_next` - outgoing edge `N`
- `bb_a <= bb_b` - dominance relationship

---

## Actions

Use `->>` to modify or create p-code relationships.

```text
o_new(USERDEFINED BEFORE o_old)
#100 ->> (0) o_new
v_arg ->> (1) o_new
o_new ->> v_out
DELETE o_old
```

Supported forms:

- `o_new(OPCODE BEFORE o_old)` - create pnode before an existing pnode
- `o_new(OPCODE AFTER o_old)` - create pnode after an existing pnode
- `OPCODE` must be a concrete opcode when creating a pnode; `_`/ANY is only for
  pattern matching existing pnodes
- `value ->> (N) o_target` - set input `N`
- `o_target ->> v_target` - set output varnode
- `v_new(size)` - create or size a varnode action term
- `DELETE o_target` - delete a matched pnode

---

## Example

Copy Propagation
```text
v_orig ->(0) o_copy (COPY, _) -> v_copy ->(0) o_any(_, _)
--
v_orig ->>(0) o_any
```
