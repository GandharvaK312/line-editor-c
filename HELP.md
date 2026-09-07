# Simple Line Editor — Command Reference Manual

This application is a command-line line editor designed in standard C. It provides direct text manipulation via line indexing and terminal commands.

---

## Command Syntax & Parameters

| Command | Shorthand | Syntax | Description |
| :--- | :--- | :--- | :--- |
| `insert` | `ins`, `i` | `ins <line_num> <text>` | Inserts text at the designated line number. |
| `delete` | `del`, `d` | `del <line_num>` | Removes the specified line from memory. |
| `print` | `p`, `list` | `print [start] [end]` | Displays lines formatted with numbers. Defaults to all. |
| `save` | `w` | `save <filename>` | Writes current document buffer out to a disk file. |
| `load` | `e` | `load <filename>` | Clears current buffer and loads lines from a file. |
| `find` | `f`, `search`| `find <phrase>` | Scans document and prints line numbers with matches. |
| `rep` | `r`, `replace`| `rep <line\|all>/<target>/<replacement>` | Replaces matching strings on a specific line or all lines. |
| `undo` | `u` | `undo` | Reverses the most recent insert, delete, or replace action. |
| `stats` | — | `stats` | Displays total line count, word count, and character count. |
| `help` | `h` | `help` | Displays the built-in quick command help card. |
| `quit` | `q`, `exit` | `quit` | Cleans up all heap memory and exits the program. |

---

## Step-by-Step Examples

### 1. Document Creation & Insertion
To build a document from scratch, supply 1-indexed positions:
```text
ed> ins 1 The quick brown fox
OK: Inserted line at 1.

ed> ins 2 jumped over the dog.
OK: Inserted line at 2.

ed> ins 2 lazily
OK: Inserted line at 2.

ed> print
   1 | The quick brown fox
   2 | lazily
   3 | jumped over the dog.