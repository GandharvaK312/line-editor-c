#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_INPUT_BUFFER 4096

/* --- Data Structures --- */

typedef struct LineNode {
    char *text;
    struct LineNode *prev;
    struct LineNode *next;
} LineNode;

typedef struct {
    LineNode *head;
    LineNode *tail;
    int line_count;
} Document;

typedef enum {
    ACTION_INSERT,
    ACTION_DELETE,
    ACTION_REPLACE
} ActionType;

typedef struct UndoNode {
    ActionType type;
    int line_num;
    char *text_before;
    char *text_after;
    struct UndoNode *next;
} UndoNode;

typedef struct {
    UndoNode *top;
    int count;
} UndoStack;

/* --- Safe Memory & String Utilities --- */

static char *safe_strdup(const char *src) {
    if (!src) return NULL;
    size_t len = strlen(src);
    char *dest = (char *)malloc(len + 1);
    if (!dest) {
        fprintf(stderr, "Error: Memory allocation failed in safe_strdup.\n");
        exit(EXIT_FAILURE);
    }
    memcpy(dest, src, len + 1);
    return dest;
}

static void trim_newline(char *str) {
    if (!str) return;
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

static char *skip_whitespace(char *str) {
    while (*str && isspace((unsigned char)*str)) {
        str++;
    }
    return str;
}

/* --- Document Initialization & Cleanup --- */

static void init_document(Document *doc) {
    doc->head = NULL;
    doc->tail = NULL;
    doc->line_count = 0;
}

static void free_document(Document *doc) {
    LineNode *curr = doc->head;
    while (curr) {
        LineNode *next = curr->next;
        free(curr->text);
        free(curr);
        curr = next;
    }
    doc->head = NULL;
    doc->tail = NULL;
    doc->line_count = 0;
}

/* --- Undo Subsystem --- */

static void init_undo_stack(UndoStack *stack) {
    stack->top = NULL;
    stack->count = 0;
}

static void free_undo_node(UndoNode *node) {
    if (!node) return;
    free(node->text_before);
    free(node->text_after);
    free(node);
}

static void clear_undo_stack(UndoStack *stack) {
    UndoNode *curr = stack->top;
    while (curr) {
        UndoNode *next = curr->next;
        free_undo_node(curr);
        curr = next;
    }
    stack->top = NULL;
    stack->count = 0;
}

static void push_undo(UndoStack *stack, ActionType type, int line_num, const char *before, const char *after) {
    UndoNode *node = (UndoNode *)malloc(sizeof(UndoNode));
    if (!node) {
        fprintf(stderr, "Error: Memory allocation failed for undo record.\n");
        return;
    }
    node->type = type;
    node->line_num = line_num;
    node->text_before = before ? safe_strdup(before) : NULL;
    node->text_after = after ? safe_strdup(after) : NULL;
    node->next = stack->top;
    stack->top = node;
    stack->count++;
}

/* --- Core Document Operations --- */

static LineNode *get_node_at(const Document *doc, int line_num) {
    if (line_num < 1 || line_num > doc->line_count) return NULL;
    
    LineNode *curr;
    if (line_num <= doc->line_count / 2) {
        curr = doc->head;
        for (int i = 1; i < line_num; i++) {
            curr = curr->next;
        }
    } else {
        curr = doc->tail;
        for (int i = doc->line_count; i > line_num; i--) {
            curr = curr->prev;
        }
    }
    return curr;
}

static bool insert_line_internal(Document *doc, int line_num, const char *text) {
    if (line_num < 1 || line_num > doc->line_count + 1) {
        return false;
    }

    LineNode *new_node = (LineNode *)malloc(sizeof(LineNode));
    if (!new_node) {
        fprintf(stderr, "Error: Memory allocation failed for LineNode.\n");
        return false;
    }
    new_node->text = safe_strdup(text);
    new_node->prev = NULL;
    new_node->next = NULL;

    if (doc->line_count == 0) {
        doc->head = new_node;
        doc->tail = new_node;
    } else if (line_num == 1) {
        new_node->next = doc->head;
        doc->head->prev = new_node;
        doc->head = new_node;
    } else if (line_num == doc->line_count + 1) {
        new_node->prev = doc->tail;
        doc->tail->next = new_node;
        doc->tail = new_node;
    } else {
        LineNode *curr = get_node_at(doc, line_num);
        new_node->prev = curr->prev;
        new_node->next = curr;
        curr->prev->next = new_node;
        curr->prev = new_node;
    }

    doc->line_count++;
    return true;
}

static bool delete_line_internal(Document *doc, int line_num, char **deleted_text_out) {
    if (line_num < 1 || line_num > doc->line_count || !doc->head) {
        return false;
    }

    LineNode *curr = get_node_at(doc, line_num);
    if (!curr) return false;

    if (deleted_text_out) {
        *deleted_text_out = safe_strdup(curr->text);
    }

    if (curr == doc->head && curr == doc->tail) {
        doc->head = NULL;
        doc->tail = NULL;
    } else if (curr == doc->head) {
        doc->head = curr->next;
        doc->head->prev = NULL;
    } else if (curr == doc->tail) {
        doc->tail = curr->prev;
        doc->tail->next = NULL;
    } else {
        curr->prev->next = curr->next;
        curr->next->prev = curr->prev;
    }

    free(curr->text);
    free(curr);
    doc->line_count--;
    return true;
}

static bool replace_line_internal(Document *doc, int line_num, const char *new_text, char **old_text_out) {
    LineNode *curr = get_node_at(doc, line_num);
    if (!curr) return false;

    if (old_text_out) {
        *old_text_out = safe_strdup(curr->text);
    }

    free(curr->text);
    curr->text = safe_strdup(new_text);
    return true;
}

/* --- Public User-Invoked Commands --- */

static void cmd_insert(Document *doc, UndoStack *stack, int line_num, const char *text) {
    if (line_num < 1 || line_num > doc->line_count + 1) {
        printf("Error: Invalid line number %d. Valid range: [1, %d].\n", line_num, doc->line_count + 1);
        return;
    }
    if (insert_line_internal(doc, line_num, text)) {
        push_undo(stack, ACTION_INSERT, line_num, NULL, text);
        printf("OK: Inserted line at %d.\n", line_num);
    }
}

static void cmd_delete(Document *doc, UndoStack *stack, int line_num) {
    if (line_num < 1 || line_num > doc->line_count) {
        printf("Error: Invalid line number %d. Valid range: [1, %d].\n", line_num, doc->line_count);
        return;
    }
    char *removed_text = NULL;
    if (delete_line_internal(doc, line_num, &removed_text)) {
        push_undo(stack, ACTION_DELETE, line_num, removed_text, NULL);
        free(removed_text);
        printf("OK: Deleted line %d.\n", line_num);
    }
}

static void cmd_display(const Document *doc, int start_line, int end_line) {
    if (doc->line_count == 0) {
        printf("[Document is empty]\n");
        return;
    }
    if (start_line < 1) start_line = 1;
    if (end_line > doc->line_count) end_line = doc->line_count;
    if (start_line > end_line) {
        printf("Error: Start line (%d) cannot exceed end line (%d).\n", start_line, end_line);
        return;
    }

    LineNode *curr = get_node_at(doc, start_line);
    for (int i = start_line; i <= end_line && curr != NULL; i++) {
        printf("%4d | %s\n", i, curr->text);
        curr = curr->next;
    }
}

static void cmd_save(const Document *doc, const char *filename) {
    if (!filename || strlen(filename) == 0) {
        printf("Error: Missing filename for save.\n");
        return;
    }
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        printf("Error: Could not open '%s' for writing.\n", filename);
        return;
    }
    LineNode *curr = doc->head;
    while (curr) {
        fprintf(fp, "%s\n", curr->text);
        curr = curr->next;
    }
    fclose(fp);
    printf("OK: Successfully saved %d line(s) to '%s'.\n", doc->line_count, filename);
}

static void cmd_load(Document *doc, UndoStack *stack, const char *filename) {
    if (!filename || strlen(filename) == 0) {
        printf("Error: Missing filename for load.\n");
        return;
    }
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Error: Could not open file '%s' for reading.\n", filename);
        return;
    }

    free_document(doc);
    clear_undo_stack(stack);

    char buffer[MAX_INPUT_BUFFER];
    int loaded = 0;
    while (fgets(buffer, sizeof(buffer), fp)) {
        trim_newline(buffer);
        insert_line_internal(doc, doc->line_count + 1, buffer);
        loaded++;
    }
    fclose(fp);
    printf("OK: Loaded %d line(s) from '%s'.\n", loaded, filename);
}

/* --- Bonus Features --- */

static void cmd_search(const Document *doc, const char *pattern) {
    if (!pattern || strlen(pattern) == 0) {
        printf("Error: Missing search pattern.\n");
        return;
    }
    if (doc->line_count == 0) {
        printf("[Document is empty]\n");
        return;
    }

    int match_count = 0;
    LineNode *curr = doc->head;
    int line_num = 1;
    while (curr) {
        if (strstr(curr->text, pattern) != NULL) {
            printf("Line %d: %s\n", line_num, curr->text);
            match_count++;
        }
        curr = curr->next;
        line_num++;
    }

    if (match_count == 0) {
        printf("No occurrences of '%s' found.\n", pattern);
    } else {
        printf("Found %d matching line(s).\n", match_count);
    }
}

static char *replace_string(const char *orig, const char *rep, const char *with) {
    if (!orig || !rep || !with) return NULL;
    size_t rep_len = strlen(rep);
    if (rep_len == 0) return safe_strdup(orig);
    size_t with_len = strlen(with);

    int count = 0;
    const char *ins = orig;
    const char *p;
    while ((p = strstr(ins, rep)) != NULL) {
        count++;
        ins = p + rep_len;
    }

    size_t total_len = strlen(orig) + (with_len - rep_len) * count + 1;
    char *result = (char *)malloc(total_len);
    if (!result) {
        fprintf(stderr, "Error: Memory allocation failed during string replace.\n");
        exit(EXIT_FAILURE);
    }

    char *dst = result;
    ins = orig;
    while ((p = strstr(ins, rep)) != NULL) {
        size_t bytes = (size_t)(p - ins);
        memcpy(dst, ins, bytes);
        dst += bytes;
        memcpy(dst, with, with_len);
        dst += with_len;
        ins = p + rep_len;
    }
    strcpy(dst, ins);
    return result;
}

static void cmd_replace(Document *doc, UndoStack *stack, const char *target_scope, const char *find_str, const char *rep_str) {
    if (doc->line_count == 0) {
        printf("[Document is empty]\n");
        return;
    }
    if (!find_str || strlen(find_str) == 0) {
        printf("Error: Search phrase cannot be empty.\n");
        return;
    }
    const char *safe_rep = rep_str ? rep_str : "";

    if (strcmp(target_scope, "all") == 0) {
        int replaced_lines = 0;
        LineNode *curr = doc->head;
        int idx = 1;
        while (curr) {
            if (strstr(curr->text, find_str) != NULL) {
                char *new_text = replace_string(curr->text, find_str, safe_rep);
                push_undo(stack, ACTION_REPLACE, idx, curr->text, new_text);
                char *old_out = NULL;
                replace_line_internal(doc, idx, new_text, &old_out);
                free(old_out);
                free(new_text);
                replaced_lines++;
            }
            curr = curr->next;
            idx++;
        }
        printf("OK: Replaced occurrences across %d line(s).\n", replaced_lines);
    } else {
        int line_num = atoi(target_scope);
        if (line_num < 1 || line_num > doc->line_count) {
            printf("Error: Invalid line number '%s'. Valid range: [1, %d] or 'all'.\n", target_scope, doc->line_count);
            return;
        }
        LineNode *curr = get_node_at(doc, line_num);
        if (strstr(curr->text, find_str) == NULL) {
            printf("Phrase '%s' not found on line %d.\n", find_str, line_num);
            return;
        }
        char *new_text = replace_string(curr->text, find_str, safe_rep);
        push_undo(stack, ACTION_REPLACE, line_num, curr->text, new_text);
        char *old_out = NULL;
        replace_line_internal(doc, line_num, new_text, &old_out);
        free(old_out);
        free(new_text);
        printf("OK: Line %d updated.\n", line_num);
    }
}

static void cmd_undo(Document *doc, UndoStack *stack) {
    if (!stack->top) {
        printf("Undo stack is empty. No actions to reverse.\n");
        return;
    }

    UndoNode *act = stack->top;
    stack->top = act->next;
    stack->count--;

    switch (act->type) {
        case ACTION_INSERT: {
            char *dummy = NULL;
            delete_line_internal(doc, act->line_num, &dummy);
            free(dummy);
            printf("Undo: Reverted insertion at line %d.\n", act->line_num);
            break;
        }
        case ACTION_DELETE: {
            insert_line_internal(doc, act->line_num, act->text_before);
            printf("Undo: Restored deleted line %d.\n", act->line_num);
            break;
        }
        case ACTION_REPLACE: {
            char *dummy = NULL;
            replace_line_internal(doc, act->line_num, act->text_before, &dummy);
            free(dummy);
            printf("Undo: Reverted modification at line %d.\n", act->line_num);
            break;
        }
    }
    free_undo_node(act);
}

static void cmd_stats(const Document *doc) {
    int total_chars = 0;
    int total_words = 0;

    LineNode *curr = doc->head;
    while (curr) {
        const char *p = curr->text;
        total_chars += (int)strlen(p);
        bool in_word = false;
        while (*p) {
            if (isspace((unsigned char)*p)) {
                in_word = false;
            } else if (!in_word) {
                in_word = true;
                total_words++;
            }
            p++;
        }
        curr = curr->next;
    }

    printf("--- Document Statistics ---\n");
    printf("Total Lines      : %d\n", doc->line_count);
    printf("Total Words      : %d\n", total_words);
    printf("Total Characters : %d (excluding newlines)\n", total_chars);
    printf("---------------------------\n");
}

static void cmd_help(void) {
    printf("================ Line Editor Commands ================\n");
    printf("  ins <line> <text>         : Insert text at line number\n");
    printf("  del <line>                : Delete line at line number\n");
    printf("  print [start] [end]       : Display lines (defaults to all)\n");
    printf("  save <filename>           : Save document to text file\n");
    printf("  load <filename>           : Load text file into document\n");
    printf("  find <phrase>             : Search text across lines\n");
    printf("  rep <line|all>/<src>/<dst>: Find and replace text\n");
    printf("  undo                      : Undo the last action\n");
    printf("  stats                     : Show line, word, and char count\n");
    printf("  help                      : Display this reference\n");
    printf("  quit                      : Exit the line editor\n");
    printf("======================================================\n");
}

/* --- Interactive Command Dispatcher --- */

int main(int argc, char *argv[]) {
    Document doc;
    UndoStack undo_stack;
    init_document(&doc);
    init_undo_stack(&undo_stack);

    printf("Minimalist Line Editor in C (Studio 3rd Semester)\n");
    printf("Type 'help' for available commands or 'quit' to exit.\n\n");

    if (argc > 1) {
        cmd_load(&doc, &undo_stack, argv[1]);
    }

    char input[MAX_INPUT_BUFFER];
    while (1) {
        printf("ed> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            printf("\nExiting...\n");
            break;
        }

        trim_newline(input);
        char *line = skip_whitespace(input);
        if (*line == '\0') continue;

        char *token = strtok(line, " \t");
        if (!token) continue;

        if (strcmp(token, "quit") == 0 || strcmp(token, "exit") == 0 || strcmp(token, "q") == 0) {
            break;
        } else if (strcmp(token, "help") == 0 || strcmp(token, "h") == 0) {
            cmd_help();
        } else if (strcmp(token, "stats") == 0) {
            cmd_stats(&doc);
        } else if (strcmp(token, "undo") == 0 || strcmp(token, "u") == 0) {
            cmd_undo(&doc, &undo_stack);
        } else if (strcmp(token, "print") == 0 || strcmp(token, "p") == 0 || strcmp(token, "list") == 0) {
            char *arg1 = strtok(NULL, " \t");
            char *arg2 = strtok(NULL, " \t");
            int start = arg1 ? atoi(arg1) : 1;
            int end = arg2 ? atoi(arg2) : doc.line_count;
            cmd_display(&doc, start, end);
        } else if (strcmp(token, "ins") == 0 || strcmp(token, "insert") == 0 || strcmp(token, "i") == 0) {
            char *line_str = strtok(NULL, " \t");
            if (!line_str) {
                printf("Usage: ins <line_number> <text>\n");
                continue;
            }
            int line_num = atoi(line_str);
            char *text = line_str + strlen(line_str) + 1;
            while (*text && (*text == ' ' || *text == '\t')) text++;
            cmd_insert(&doc, &undo_stack, line_num, text);
        } else if (strcmp(token, "del") == 0 || strcmp(token, "delete") == 0 || strcmp(token, "d") == 0) {
            char *line_str = strtok(NULL, " \t");
            if (!line_str) {
                printf("Usage: del <line_number>\n");
                continue;
            }
            int line_num = atoi(line_str);
            cmd_delete(&doc, &undo_stack, line_num);
        } else if (strcmp(token, "save") == 0 || strcmp(token, "w") == 0) {
            char *fname = strtok(NULL, " \t");
            if (!fname) {
                printf("Usage: save <filename>\n");
                continue;
            }
            cmd_save(&doc, fname);
        } else if (strcmp(token, "load") == 0 || strcmp(token, "e") == 0) {
            char *fname = strtok(NULL, " \t");
            if (!fname) {
                printf("Usage: load <filename>\n");
                continue;
            }
            cmd_load(&doc, &undo_stack, fname);
        } else if (strcmp(token, "find") == 0 || strcmp(token, "search") == 0 || strcmp(token, "f") == 0) {
            char *pattern = strtok(NULL, "");
            if (!pattern) {
                printf("Usage: find <phrase>\n");
                continue;
            }
            pattern = skip_whitespace(pattern);
            cmd_search(&doc, pattern);
        } else if (strcmp(token, "rep") == 0 || strcmp(token, "replace") == 0 || strcmp(token, "r") == 0) {
            char *args = strtok(NULL, "");
            if (!args) {
                printf("Usage: rep <line_number|all>/<target>/<replacement>\n");
                continue;
            }
            args = skip_whitespace(args);
            char *sep1 = strchr(args, '/');
            if (!sep1) {
                printf("Invalid format. Use: rep <line|all>/<target>/<replacement>\n");
                continue;
            }
            *sep1 = '\0';
            char *target = sep1 + 1;
            char *sep2 = strchr(target, '/');
            if (!sep2) {
                printf("Invalid format. Use: rep <line|all>/<target>/<replacement>\n");
                continue;
            }
            *sep2 = '\0';
            char *replacement = sep2 + 1;
            cmd_replace(&doc, &undo_stack, args, target, replacement);
        } else {
            printf("Unknown command '%s'. Type 'help' to see syntax.\n", token);
        }
    }

    free_document(&doc);
    clear_undo_stack(&undo_stack);
    printf("Session terminated cleanly. Memory freed.\n");
    return 0;
    
}