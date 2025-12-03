#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ROWS 10
#define MAX_COLS 5
#define MAX_CELL_LEN 30

#define TYPE_LEAF 0
#define TYPE_TABLE 1

/* Foreground and Background Colors (White text on Black default) */
#define FG_DEFAULT (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define BG_DEFAULT 0
#define ATTRIB_DEFAULT (FG_DEFAULT | BG_DEFAULT)

/* Highlight Color (Black text on White background) */
#define FG_HIGHLIGHT 0
#define BG_HIGHLIGHT (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE)
#define ATTRIB_HIGHLIGHT (FG_HIGHLIGHT | BACKGROUND_INTENSITY | BG_HIGHLIGHT)

typedef struct TABLE TABLE;

typedef struct CELL {
    int type;
    char content[MAX_CELL_LEN + 1];
    TABLE *sub_table;
} CELL;

struct TABLE {
    CELL cells[MAX_ROWS][MAX_COLS];
    int rows;
    int cols;
    int cursor_row;
    int cursor_col;
};

#define MAX_DEPTH 5
TABLE *g_current_table;
TABLE *g_table_root;
TABLE *g_table_stack[MAX_DEPTH];
int g_stack_depth = 0;

HANDLE g_hConsoleOutput;
HANDLE g_hConsoleInput;
DWORD g_dwOriginalMode;

CHAR_INFO *g_buffer;
int g_buffer_size = 0;
int g_screen_width = 0;
int g_screen_height = 0;

void set_cursor(short x, short y)
{
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(g_hConsoleOutput, coord);
}

void hide_cursor()
{
    CONSOLE_CURSOR_INFO cursor_info;
    cursor_info.dwSize = 1;
    cursor_info.bVisible = FALSE;
    SetConsoleCursorInfo(g_hConsoleOutput, &cursor_info);
}

void show_cursor()
{
    CONSOLE_CURSOR_INFO cursor_info;
    cursor_info.dwSize = 1;
    cursor_info.bVisible = TRUE;
    SetConsoleCursorInfo(g_hConsoleOutput, &cursor_info);
}

void free_console_buffer()
{
    if (g_buffer) {
        free(g_buffer);
        g_buffer = NULL;
    }
    g_buffer_size = 0;
    g_screen_width = 0;
    g_screen_height = 0;
}

void clear_screen()
{
    DWORD dwDummy;
    COORD coordScreen = {0, 0};
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    if (!GetConsoleScreenBufferInfo(g_hConsoleOutput, &csbi)) {
        return;
    }
    FillConsoleOutputCharacterA(g_hConsoleOutput, ' ', csbi.dwSize.X * csbi.dwSize.Y, coordScreen, &dwDummy);
    FillConsoleOutputAttribute(g_hConsoleOutput, ATTRIB_DEFAULT, csbi.dwSize.X * csbi.dwSize.Y, coordScreen, &dwDummy);
    set_cursor(0, 0);
}

int initialize_console_buffer()
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD new_size;
    
    free_console_buffer();
    
    if (!GetConsoleScreenBufferInfo(g_hConsoleOutput, &csbi)) {
        return 0;
    }
    
    /* Calculate current visible window dimensions */
    g_screen_width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    g_screen_height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    
    /* Set the screen buffer size to match the window size to prevent scrolling */
    new_size.X = (short)g_screen_width;
    new_size.Y = (short)g_screen_height;
    /* We must check if this fails, which it often does if the size is too small or large */
    if (!SetConsoleScreenBufferSize(g_hConsoleOutput, new_size)) {
        /* If setting fails, just proceed with the current buffer size and hope the malloc works */
        /* Re-get the actual size after the attempted set, in case OS adjusted it */
        if (!GetConsoleScreenBufferInfo(g_hConsoleOutput, &csbi)) {
            return 0;
        }
        g_screen_width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        g_screen_height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }

    g_buffer_size = g_screen_width * g_screen_height;
    
    g_buffer = (CHAR_INFO *)malloc(sizeof(CHAR_INFO) * g_buffer_size);
    if (!g_buffer) {
        g_buffer_size = 0;
        return 0;
    }
    
    return 1;
}

void buffer_clear()
{
    int i;
    for (i = 0; i < g_buffer_size; i++) {
        g_buffer[i].Char.AsciiChar = ' ';
        g_buffer[i].Attributes = ATTRIB_DEFAULT;
    }
}

void buffer_set_char(int x, int y, char c, WORD color)
{
    int index;
    if (x >= 0 && x < g_screen_width && y >= 0 && y < g_screen_height) {
        index = y * g_screen_width + x;
        g_buffer[index].Char.AsciiChar = c;
        g_buffer[index].Attributes = color;
    }
}

void buffer_print_string(int x, int y, const char *str, int max_len, WORD color)
{
    int i;
    int len;
    
    len = strlen(str);
    if (len > max_len) {
        len = max_len;
    }
    
    for (i = 0; i < len; i++) {
        buffer_set_char(x + i, y, str[i], color);
    }
    
    for (i = len; i < max_len; i++) {
        buffer_set_char(x + i, y, ' ', color);
    }
}

void blit_buffer()
{
    COORD buffer_size_coord;
    COORD buffer_coord;
    SMALL_RECT write_region;

    buffer_size_coord.X = (short)g_screen_width;
    buffer_size_coord.Y = (short)g_screen_height;
    
    buffer_coord.X = 0;
    buffer_coord.Y = 0;
    
    write_region.Left = 0;
    write_region.Top = 0;
    write_region.Right = (short)(g_screen_width - 1);
    write_region.Bottom = (short)(g_screen_height - 1);
    
    WriteConsoleOutput(g_hConsoleOutput, g_buffer, buffer_size_coord, buffer_coord, &write_region);
}

TABLE *create_table(int rows, int cols)
{
    TABLE *new_table;
    int r, c;

    new_table = (TABLE *)malloc(sizeof(TABLE));
    if (!new_table) {
        return NULL;
    }

    new_table->rows = rows > MAX_ROWS ? MAX_ROWS : rows;
    new_table->cols = cols > MAX_COLS ? MAX_COLS : cols;
    new_table->cursor_row = 0;
    new_table->cursor_col = 0;

    for (r = 0; r < new_table->rows; r++) {
        for (c = 0; c < new_table->cols; c++) {
            new_table->cells[r][c].type = TYPE_LEAF;
            strcpy(new_table->cells[r][c].content, "");
            new_table->cells[r][c].sub_table = NULL;
        }
    }
    return new_table;
}

void destroy_table(TABLE *t)
{
    int r, c;
    if (!t) return;

    for (r = 0; r < t->rows; r++) {
        for (c = 0; c < t->cols; c++) {
            if (t->cells[r][c].type == TYPE_TABLE && t->cells[r][c].sub_table) {
                destroy_table(t->cells[r][c].sub_table);
            }
        }
    }
    free(t);
}

void draw_table(TABLE *t)
{
    int r, c;
    int start_x, start_y;
    int cell_width = MAX_CELL_LEN + 3; /* Border + space + content */
    char buffer[MAX_CELL_LEN + 10];
    int i;
    WORD color;
    
    buffer_clear();

    /* Header row */
    start_x = 3;
    start_y = 1;

    /* Column labels (A, B, C...) */
    for (c = 0; c < t->cols; c++) {
        if (start_x + c * cell_width + 1 < g_screen_width) {
            buffer_set_char(start_x + c * cell_width + 1, 0, 'A' + c, ATTRIB_DEFAULT);
        }
    }

    for (r = 0; r < t->rows; r++) {
        /* Check if row fits on screen */
        if (start_y + r * 2 >= g_screen_height - 3) break; 
        
        /* Horizontal border lines (Row separator) */
        for (c = 0; c < t->cols; c++) {
            if (start_x + c * cell_width < g_screen_width) {
                buffer_set_char(start_x + c * cell_width, start_y + r * 2, '+', ATTRIB_DEFAULT);
            }
            for (i = 1; i < cell_width; i++) {
                 if (start_x + c * cell_width + i < g_screen_width) {
                    buffer_set_char(start_x + c * cell_width + i, start_y + r * 2, '-', ATTRIB_DEFAULT);
                }
            }
        }

        /* Row index (1, 2, 3...) */
        sprintf(buffer, "%d", r + 1);
        buffer_print_string(0, start_y + r * 2 + 1, buffer, 2, ATTRIB_DEFAULT);

        for (c = 0; c < t->cols; c++) {
            if (start_x + c * cell_width + cell_width < g_screen_width) {
                
                /* Vertical border line */
                buffer_set_char(start_x + c * cell_width, start_y + r * 2 + 1, '|', ATTRIB_DEFAULT);
                
                /* Cell Content */
                if (t->cells[r][c].type == TYPE_TABLE) {
                    sprintf(buffer, "[TABLE %dx%d]", t->cells[r][c].sub_table->rows, t->cells[r][c].sub_table->cols);
                } else {
                    strncpy(buffer, t->cells[r][c].content, MAX_CELL_LEN);
                    buffer[MAX_CELL_LEN] = '\0';
                }

                color = ATTRIB_DEFAULT;
                if (r == t->cursor_row && c == t->cursor_col) {
                    color = ATTRIB_HIGHLIGHT;
                }

                buffer_print_string(start_x + c * cell_width + 2, start_y + r * 2 + 1, buffer, cell_width - 3, color);
            }
        }
    }

    /* Bottom horizontal border line */
    if (start_y + t->rows * 2 < g_screen_height - 3) {
        for (c = 0; c < t->cols; c++) {
            if (start_x + c * cell_width < g_screen_width) {
                buffer_set_char(start_x + c * cell_width, start_y + t->rows * 2, '+', ATTRIB_DEFAULT);
            }
            for (i = 1; i < cell_width; i++) {
                if (start_x + c * cell_width + i < g_screen_width) {
                    buffer_set_char(start_x + c * cell_width + i, start_y + t->rows * 2, '-', ATTRIB_DEFAULT);
                }
            }
        }
    }


    /* Status line 1: Cursor, Content, Help */
    sprintf(buffer, "%c%d: %.*s [ESC: Quit, ENTER: Edit/Drill, BACKSPACE: Up, CTRL+T: Make Table, CTRL+L: Make Leaf]", 
           'A' + t->cursor_col, 
           t->cursor_row + 1, 
           MAX_CELL_LEN, 
           t->cells[t->cursor_row][t->cursor_col].content);
    buffer_print_string(0, g_screen_height - 2, buffer, g_screen_width, ATTRIB_DEFAULT);

    /* Status line 2: Depth */
    sprintf(buffer, "Depth: %d", g_stack_depth);
    buffer_print_string(0, g_screen_height - 1, buffer, g_screen_width, ATTRIB_DEFAULT);
}

void edit_cell(TABLE *t)
{
    CELL *cell;
    char input_buffer[MAX_CELL_LEN + 1];
    DWORD read_count;
    int input_y;
    DWORD dwOldMode;
    
    cell = &t->cells[t->cursor_row][t->cursor_col];

    if (cell->type == TYPE_TABLE) {
        return;
    }

    input_y = g_screen_height - 3;
    
    /* Show cursor only for input mode */
    show_cursor();

    /* Clear line and print edit prompt directly to console before mode switch */
    set_cursor(0, (short)input_y);
    printf("EDIT %c%d: > %-*s", 'A' + t->cursor_col, t->cursor_row + 1, MAX_CELL_LEN, cell->content);
    
    /* Set cursor for input */
    set_cursor(12, (short)input_y); 
    
    /* Temporarily enable line input mode for ReadConsoleA */
    GetConsoleMode(g_hConsoleInput, &dwOldMode);
    SetConsoleMode(g_hConsoleInput, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
    
    if (ReadConsoleA(g_hConsoleInput, input_buffer, MAX_CELL_LEN, &read_count, NULL)) {
        input_buffer[read_count] = '\0';
        
        /* Remove newline and carriage return */
        if (read_count > 0 && input_buffer[read_count - 1] == '\n') {
            input_buffer[--read_count] = '\0';
        }
        if (read_count > 0 && input_buffer[read_count - 1] == '\r') {
            input_buffer[--read_count] = '\0';
        }
        
        strncpy(cell->content, input_buffer, MAX_CELL_LEN);
        cell->content[MAX_CELL_LEN] = '\0';
    }

    /* Restore console mode for TUI navigation and hide cursor again */
    SetConsoleMode(g_hConsoleInput, dwOldMode);
    hide_cursor();
    
    /* Re-clear the temporary input line by redrawing the buffer later */
}

void handle_navigation(INPUT_RECORD ir)
{
    WORD vk_code;
    TABLE *t;
    
    if (ir.EventType != KEY_EVENT || !ir.Event.KeyEvent.bKeyDown) {
        return;
    }
    
    t = g_current_table;
    vk_code = ir.Event.KeyEvent.wVirtualKeyCode;

    if (vk_code == VK_LEFT) {
        if (t->cursor_col > 0) t->cursor_col--;
    } else if (vk_code == VK_RIGHT) {
        if (t->cursor_col < t->cols - 1) t->cursor_col++;
    } else if (vk_code == VK_UP) {
        if (t->cursor_row > 0) t->cursor_row--;
    } else if (vk_code == VK_DOWN) {
        if (t->cursor_row < t->rows - 1) t->cursor_row++;
    } else if (vk_code == VK_RETURN) {
        if (t->cells[t->cursor_row][t->cursor_col].type == TYPE_TABLE) {
            if (g_stack_depth < MAX_DEPTH - 1) {
                g_table_stack[g_stack_depth++] = t;
                g_current_table = t->cells[t->cursor_row][t->cursor_col].sub_table;
            }
        } else {
            edit_cell(t);
        }
    } else if (vk_code == VK_BACK) {
        if (g_stack_depth > 0) {
            g_current_table = g_table_stack[--g_stack_depth];
        }
    } else if (vk_code == 'T' && ir.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) {
        CELL *cell = &t->cells[t->cursor_row][t->cursor_col];
        if (cell->type == TYPE_LEAF) {
            TABLE *new_sub;
            new_sub = create_table(3, 3);
            if (new_sub) {
                cell->type = TYPE_TABLE;
                cell->sub_table = new_sub;
                strcpy(new_sub->cells[0][0].content, "New Sub-Table");
            }
        }
    } else if (vk_code == 'L' && ir.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) {
        CELL *cell = &t->cells[t->cursor_row][t->cursor_col];
        if (cell->type == TYPE_TABLE) {
            destroy_table(cell->sub_table);
            cell->sub_table = NULL;
            cell->type = TYPE_LEAF;
            strcpy(cell->content, "");
        }
    }
}

int main()
{
    DWORD cNumRead;
    INPUT_RECORD irInBuf[128];
    int run_loop = 1;
    int i;
    int table_row = 1;
    int table_col = 2;
    DWORD dwOldMode;

    g_hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    g_hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);

    if (g_hConsoleOutput == INVALID_HANDLE_VALUE || g_hConsoleInput == INVALID_HANDLE_VALUE) {
        return 1;
    }

    /* Save and set console mode for non-blocking TUI input */
    GetConsoleMode(g_hConsoleInput, &dwOldMode);
    
    /* Set mode to raw input (ENABLE_WINDOW_INPUT for resize/key events) */
    if (!SetConsoleMode(g_hConsoleInput, ENABLE_WINDOW_INPUT)) {
        /* If setting TUI mode fails, we cannot proceed */
        return 1;
    }
    
    /* Initialize environment and buffer: FIX: This order is critical for stability. */
    if (!initialize_console_buffer()) {
        SetConsoleMode(g_hConsoleInput, dwOldMode);
        return 1;
    }

    /* Clear screen and hide cursor after the buffer size is guaranteed to be set */
    clear_screen();
    hide_cursor();


    g_table_root = create_table(MAX_ROWS, MAX_COLS);
    if (!g_table_root) {
        free_console_buffer();
        SetConsoleMode(g_hConsoleInput, dwOldMode);
        return 1;
    }
    g_current_table = g_table_root;

    strcpy(g_table_root->cells[0][0].content, "Root Cell A1");
    strcpy(g_table_root->cells[table_row][table_col].content, "Go Here ->");
    g_table_root->cells[table_row][table_col].type = TYPE_TABLE;
    g_table_root->cells[table_row][table_col].sub_table = create_table(5, 3);
    if (g_table_root->cells[table_row][table_col].sub_table) {
        strcpy(g_table_root->cells[table_row][table_col].sub_table->cells[0][0].content, "Sub Table 1: Title");
        strcpy(g_table_root->cells[table_row][table_col].sub_table->cells[1][0].content, "Sub Table 1: Data");
    }

    while (run_loop) {
        
        /* 1. Read input events (handles resize and navigation) */
        if (WaitForSingleObject(g_hConsoleInput, 10) == WAIT_OBJECT_0) {
            if (ReadConsoleInput(g_hConsoleInput, irInBuf, 128, &cNumRead)) {
                for (i = 0; i < (int)cNumRead; i++) {
                    if (irInBuf[i].EventType == KEY_EVENT) {
                        if (irInBuf[i].Event.KeyEvent.bKeyDown) {
                            if (irInBuf[i].Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) {
                                run_loop = 0;
                                break;
                            }
                            handle_navigation(irInBuf[i]);
                        }
                    } else if (irInBuf[i].EventType == WINDOW_BUFFER_SIZE_EVENT) {
                        /* 2. Handle Resize Event */
                        initialize_console_buffer();
                    }
                }
            }
        }
        
        if (!run_loop) break;

        /* 3. Draw entire state to buffer */
        draw_table(g_current_table);

        /* 4. Blit buffer to screen (single call update) */
        blit_buffer();
    }

    /* Cleanup */
    free_console_buffer();
    destroy_table(g_table_root);
    show_cursor();
    SetConsoleMode(g_hConsoleInput, dwOldMode);
    
    return 0;
}