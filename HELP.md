# Line Editor - User Manual

This editor operates strictly through terminal commands. You must specify the action, the target line number, and any relevant text[cite: 1].

## Supported Commands

| Command | Syntax | Description |
| :--- | :--- | :--- |
| **Insert** | `i <line_number> <text>` | Inserts `<text>` at the specified `<line_number>`. Existing lines are shifted down. |
| **Delete** | `d <line_number>` | Deletes the text at `<line_number>`. Lines below it are shifted up. |
| **Print** | `p` | Displays the entire document with line numbers in the terminal. |
| **Help** | `h` | Prints a quick reference cheat sheet. |
| **Quit** | `q` | Exits the editor. Memory is cleared upon exit. |

## Usage Examples

*   **To start a document:**
    `> i 1 This is my very first line of text.`
*   **To insert a line in the middle:**
    `> i 2 This text drops between line 1 and the old line 2.`
*   **To delete the 3rd line:**
    `> d 3`
*   **To view your work:**
    `> p`