# Command-Line Line Editor in C

This project is a minimalist command-line line editor written in C, developed for the 3rd Semester Portfolio Building Studio Course coding competition[cite: 1].

## Project Overview
* **Duration:** 2 hours[cite: 1]
* **Team Size:** 1–3 members[cite: 1]
* **Objective:** Design and build a working terminal-based line editor that manipulates text documents one line at a time using line-number commands, entirely from the terminal with no GUI[cite: 1].

## Implemented Features
* **Core Features:**
  * **Insert a line:** Add a new line of text at a given line number, shifting existing lines down[cite: 1].
  * **Delete a line:** Remove the line at a given number, shifting the lines below it up[cite: 1].
  * **Display the document:** Print all current lines with their line numbers to review document state[cite: 1].
  * **Save / Load a file:** Write the in-memory document to a `.txt` file and read one back in on startup[cite: 1].
* **Bonus Features:**
  * **Search:** Find and report line numbers containing a given word or phrase[cite: 1].
  * **Find & Replace:** Replace text on a specific line or across the whole document[cite: 1].
  * **Undo:** Reverse the most recent insert, delete, or edit action[cite: 1].
  * **Statistics:** Report simple document metrics like line and word counts[cite: 1].

## Compilation & Run Instructions
Compile the project using strict compiler warnings:
```bash
gcc -Wall -Wextra -std=c11 -pedantic line_editor.c -o line_editor