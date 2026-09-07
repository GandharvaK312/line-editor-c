# C-Line Editor

A command-line text editor built in C for the Portfolio Building Studio Course.

### Team Members
*   Computer Science Undergrad, Reva University

### Core Features Implemented
This project fulfills the following core requirements:
1. **Insert a line:** Dynamically adds memory and links the node at any given integer index[cite: 1].
2. **Delete a line:** Bypasses and frees the specified node, shifting the remaining layout up[cite: 1].
3. **Display the document:** Traverses the entire list sequentially to output the current state with line numbers[cite: 1].

### Compilation and Execution
To run this project from a standard terminal environment, ensure `gcc` is installed[cite: 1].

1. Compile the source code:
   ```bash
   gcc -o editor editor.c