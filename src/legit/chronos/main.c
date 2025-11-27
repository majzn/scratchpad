#define _CRT_SECURE_NO_WARNINGS
#define CHRONOS_IMPLEMENTATION
#include "chronos.h"
#define SS_ENGINE_IMPLEMENTATION
#include "ss.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#ifndef _WIN32
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>

static struct termios oldt, newt;

void set_non_blocking_input() {
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

void restore_input() {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

int check_for_input() {
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}

int get_char_non_blocking() {
    if (check_for_input()) {
        char c;
        if (read(STDIN_FILENO, &c, 1) == 1) return c;
    }
    return -1;
}

#else
#include <conio.h>

void set_non_blocking_input() {}
void restore_input() {}

int check_for_input() {
    return _kbhit();
}

int get_char_non_blocking() {
    if (_kbhit()) return _getch();
    return -1;
}
#endif

#define MAX_HISTORY 32
#define MAX_LINE_LEN 1024
static char history[MAX_HISTORY][MAX_LINE_LEN];
static int history_count = 0;
static int history_index = 0;

static void add_to_history(const char* line) {
    if (strlen(line) == 0) return;
    if (history_count > 0 && strcmp(history[history_count - 1], line) == 0) return;

    if (history_count < MAX_HISTORY) {
        strncpy(history[history_count], line, MAX_LINE_LEN - 1);
        history[history_count][MAX_LINE_LEN - 1] = '\0';
        history_count++;
    } else {
        int i;
        for (i = 0; i < MAX_HISTORY - 1; i++) {
            strncpy(history[i], history[i+1], MAX_LINE_LEN - 1);
        }
        strncpy(history[MAX_HISTORY - 1], line, MAX_LINE_LEN - 1);
        history[MAX_HISTORY - 1][MAX_LINE_LEN - 1] = '\0';
    }
    history_index = history_count;
}

static void clear_line(int line_pos) {
    int i;
    for (i = 0; i < line_pos; i++) printf("\b \b");
    fflush(stdout);
}

static void load_history_command(char* line_buffer, int* line_pos) {
    if (history_count == 0 || history_index < 0 || history_index >= history_count) return;

    clear_line(*line_pos);

    strncpy(line_buffer, history[history_index], MAX_LINE_LEN - 1);
    line_buffer[MAX_LINE_LEN - 1] = '\0';
    *line_pos = (int)strlen(line_buffer);

    printf("%s", line_buffer);
    fflush(stdout);
}

static cr_engine* engine = NULL;
static int repl_mode = 0;
#define MAX_CUSTOM_KEYS 64
static char custom_note_map[MAX_CUSTOM_KEYS + 1] = "1234567890qwertyuiopasdfghjklzxcvbnm"; 
static int custom_map_len = 36;
static int custom_map_idx = 0;

static double mono_mode_val = 1.0; 
static double retrigger_mode_val = 1.0; 

void app_audio_play(short *buf, int n_frames, int s_rate, int ch, void *udata) {
    int i;
    for (i = 0; i < n_frames; i++) {
        cr_tick(engine);
        double l = cr_process(engine, 0);
        double r = cr_process(engine, 1);
        l = tanh(l); if(l>1) l=1; if(l<-1) l=-1;
        r = tanh(r); if(r>1) r=1; if(r<-1) r=-1;
        buf[i*2] = (short)(l * 32000.0);
        buf[i*2+1] = (short)(r * 32000.0);
    }
}

int main(int argc, char **argv) {
    char* script_content = NULL;
    ss_audio_t* audio;
    char* file_to_load = NULL;
    int i;
    FILE* f;
    long sz;
    
    for(i = 1; i < argc; i++) {
        if(argv[i][0] != '-') file_to_load = argv[i];
    }
    
    printf("Chronos v9.5 (Modular OP System with New Primitives)\n");
    
    engine = cr_create_engine(48000);
    cr_set_log_level(engine, CR_LOG_INFO);

    if (file_to_load) {
        f = fopen(file_to_load, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            sz = ftell(f);
            fseek(f, 0, SEEK_SET);
            script_content = (char*)malloc(sz + 1);
            if (script_content) {
                fread(script_content, 1, sz, f);
                script_content[sz] = 0;
                printf("Loading '%s'...\n", file_to_load);
                if(cr_eval(engine, script_content, 1)) {
                    printf("Successfully loaded.\n");
                }
                free(script_content);
            }
            fclose(f);
        } else {
            printf("Could not open file '%s'\n", file_to_load);
        }
    }

    {
        int gate_node = cr_get_variable_node(engine->active, "NOTE_GATE");
        int pitch_node = cr_get_variable_node(engine->active, "NOTE_PITCH");
        int mono_node = cr_get_variable_node(engine->active, "MONO_MODE");
        int retrigger_node = cr_get_variable_node(engine->active, "RETRIGGER_MODE");
        int adsr_node = -1;

        if (gate_node != -1) adsr_node = cr_find_adsr_node_for_gate(engine->active, gate_node);

        audio = ss_open_audio(app_audio_play, NULL, 48000, 2, 1024, NULL);
        if (!audio) return 1;

        set_non_blocking_input();
        
        printf("Audio running. Type 'mode' or press ENTER to switch to Note-Entry mode.\n");
        printf("> ");
        fflush(stdout);

        static char line_buffer[MAX_LINE_LEN];
        static int line_pos = 0;
        
        static int in_escape = 0;
        static int in_bracket = 0;

        while (1) {
            int c = get_char_non_blocking();
            
            #ifdef _WIN32
            if (repl_mode == 0 && c != -1 && (c == 0 || c == 0xE0)) { 
                int key_code = _getch();
                c = -1; 
                
                if (key_code == 72 && history_count > 0) {
                    if (history_index > 0) history_index--;
                    load_history_command(line_buffer, &line_pos);
                    continue;
                } else if (key_code == 80 && history_count > 0) { 
                    if (history_index < history_count - 1) {
                        history_index++;
                        load_history_command(line_buffer, &line_pos);
                    } else if (history_index == history_count - 1) {
                        history_index = history_count;
                        clear_line(line_pos);
                        line_pos = 0;
                        line_buffer[0] = 0;
                    }
                    continue;
                }
            }
            #else
            if (repl_mode == 0 && c != -1 && c == 27) {
                in_escape = 1;
                in_bracket = 0;
                continue;
            }
            if (in_escape && c != -1) {
                if (c == '[') {
                    in_bracket = 1;
                    continue;
                }
                
                if (in_bracket) {
                    in_escape = 0;
                    in_bracket = 0;
                    if (c == 'A' && history_count > 0) {
                        if (history_index > 0) history_index--;
                        load_history_command(line_buffer, &line_pos);
                        continue;
                    } else if (c == 'B' && history_count > 0) {
                        if (history_index < history_count - 1) {
                            history_index++;
                            load_history_command(line_buffer, &line_pos);
                        } else if (history_index == history_count - 1) {
                            history_index = history_count;
                            clear_line(line_pos);
                            line_pos = 0;
                            line_buffer[0] = 0;
                        }
                        continue;
                    } else {
                    }
                }
                
                if (c == 27) { } else {
                    in_escape = 0;
                }
            }
            #endif

            if (c == 27) {
                printf("\n");
                if (repl_mode == 1 || repl_mode == 2) {
                    repl_mode = 0;
                    printf("Switched to **Code-Entry** mode.\n> ");
                    if (gate_node != -1) cr_set_node_value(engine->active, gate_node, 0.0);
                } else {
                    
                }
                fflush(stdout);
                continue;
            }

            if (repl_mode == 2) { 
                if (c != -1) {
                    if (c == 13 || c == 10) { 
                        repl_mode = 1; 
                        custom_note_map[custom_map_idx] = 0;
                        custom_map_len = custom_map_idx;
                        printf("\nMapping finalized: %s\nSwitched to **Note-Entry** mode (ESC to exit).\n> ", custom_note_map);
                        if (gate_node != -1) cr_set_node_value(engine->active, gate_node, 0.0);
                        custom_map_idx = 0;
                        fflush(stdout);
                    } else if (c == 127 || c == 8) { 
                        if (custom_map_idx > 0) {
                            custom_map_idx--;
                            printf("\b \b"); 
                            fflush(stdout);
                        }
                    } else if (c > 31 && custom_map_idx < MAX_CUSTOM_KEYS) { 
                        char key = tolower(c);
                        if (strchr(custom_note_map, key) == NULL || custom_map_idx == 0) { 
                            custom_note_map[custom_map_idx++] = key;
                            
                            if (pitch_node != -1) cr_set_node_value(engine->active, pitch_node, (double)(60 + custom_map_idx));
                            if (adsr_node != -1) engine->active->node_pool[adsr_node].dsp.s[4] = 0.0;
                            if (gate_node != -1) cr_set_node_value(engine->active, gate_node, 1.0);

                            printf("%c", key); 
                            fflush(stdout);
                        } else {
                            printf("\a"); 
                            fflush(stdout);
                        }
                    }
                }
            } else if (repl_mode == 1) {
                
                const int base_midi = 48; 
                int note_index = -1;
                char key_char;

                if (c != -1) {
                    key_char = tolower(c);
                    if (key_char == ' ') {
                        if (gate_node != -1) cr_set_node_value(engine->active, gate_node, 0.0);
                        printf("\r[NOTE: OFF]  ");
                        fflush(stdout);
                    } else {
                        char* found = strchr(custom_note_map, key_char);
                        if (found) note_index = found - custom_note_map;
                    }
                }

                if (note_index != -1 && pitch_node != -1) {
                    if (adsr_node != -1) engine->active->node_pool[adsr_node].dsp.s[4] = 0.0;
                    cr_set_node_value(engine->active, pitch_node, (double)(base_midi + note_index));
                    if (gate_node != -1) cr_set_node_value(engine->active, gate_node, 1.0);
                    printf("\r[NOTE: %d] ", base_midi + note_index);
                    fflush(stdout);
                } else if (c == 13 || c == 10) {
                    repl_mode = 0;
                    printf("\nSwitched to **Code-Entry** mode.\n> ");
                    if (gate_node != -1) cr_set_node_value(engine->active, gate_node, 0.0);
                    fflush(stdout);
                } else if (c != -1 && c != ' ') {
                    printf("\r[NOTE: Unknown key. ESC to exit]  ");
                    fflush(stdout);
                }

            } else {
                if (c != -1) {
                    if (c == 13 || c == 10) {
                        line_buffer[line_pos] = 0;
                        line_pos = 0;
                        
                        add_to_history(line_buffer);

                        if (strcmp(line_buffer, "quit") == 0) break;
                        if (strcmp(line_buffer, "") == 0) {
                            repl_mode = 1;
                            printf("\nSwitched to **Note-Entry** mode (ESC to exit).\n> ");
                            fflush(stdout);
                            continue;
                        }
                        if (strcmp(line_buffer, "mode") == 0) {
                            repl_mode = 1;
                            printf("\nSwitched to **Note-Entry** mode (ESC to exit).\n> ");
                            fflush(stdout);
                            continue;
                        }
                        if (strcmp(line_buffer, "map") == 0) {
                            repl_mode = 2;
                            custom_map_idx = 0;
                            memset(custom_note_map, 0, sizeof(custom_note_map));
                            printf("\nSwitched to **Mapping** mode. Press keys (max %d, ENTER to finish):\n", MAX_CUSTOM_KEYS);
                            if (pitch_node != -1) cr_set_node_value(engine->active, pitch_node, 60.0);
                            if (gate_node != -1) cr_set_node_value(engine->active, gate_node, 0.0); 
                            printf("Key Map: ");
                            fflush(stdout);
                            continue;
                        }
                        
                        if (strcmp(line_buffer, "mono") == 0) {
                            mono_mode_val = 1.0;
                            if (mono_node != -1) cr_set_node_value(engine->active, mono_node, 1.0);
                            printf("Set to Monophonic Mode.\n> "); fflush(stdout); continue;
                        }
                        if (strcmp(line_buffer, "poly") == 0) {
                            mono_mode_val = 0.0;
                            if (mono_node != -1) cr_set_node_value(engine->active, mono_node, 0.0);
                            printf("Set to Polyphonic Mode (Note: Requires multiple ADSR instances in script).\n> "); fflush(stdout); continue;
                        }
                        if (strcmp(line_buffer, "retrigger") == 0) {
                            retrigger_mode_val = 1.0;
                            if (retrigger_node != -1) cr_set_node_value(engine->active, retrigger_node, 1.0);
                            printf("Set to Retrigger Mode.\n> "); fflush(stdout); continue;
                        }
                        if (strcmp(line_buffer, "legato") == 0) {
                            retrigger_mode_val = 0.0;
                            if (retrigger_node != -1) cr_set_node_value(engine->active, retrigger_node, 0.0);
                            printf("Set to Legato Mode.\n> "); fflush(stdout); continue;
                        }
                        
                        if (strncmp(line_buffer, "load ", 5) == 0) {
                            char* fname = line_buffer + 5;
                            f = fopen(fname, "rb");
                            if (f) {
                                fseek(f, 0, SEEK_END);
                                sz = ftell(f);
                                fseek(f, 0, SEEK_SET);
                                script_content = (char*)malloc(sz + 1);
                                if (script_content) {
                                    fread(script_content, 1, sz, f);
                                    script_content[sz] = 0;
                                    if(cr_eval(engine, script_content, 1)) {
                                        printf("Loaded '%s'.\n", fname);
                                    }
                                    free(script_content);
                                }
                                fclose(f);
                            } else {
                                printf("File not found.\n");
                            }
                        } else {
                            if(cr_eval(engine, line_buffer, 0)) {
                                printf("OK\n");
                            }
                        }
                        
                        gate_node = cr_get_variable_node(engine->active, "NOTE_GATE");
                        pitch_node = cr_get_variable_node(engine->active, "NOTE_PITCH");
                        mono_node = cr_get_variable_node(engine->active, "MONO_MODE");
                        retrigger_node = cr_get_variable_node(engine->active, "RETRIGGER_MODE");
                        if (gate_node != -1) adsr_node = cr_find_adsr_node_for_gate(engine->active, gate_node);
                        
                        printf("> ");
                        fflush(stdout);

                    } else if (c == 127 || c == 8) {
                        if (line_pos > 0) {
                            line_pos--;
                            printf("\b \b");
                            fflush(stdout);
                        }
                    } else if (c > 31 && line_pos < MAX_LINE_LEN - 1) {
                        line_buffer[line_pos++] = (char)c;
                        printf("%c", (char)c);
                        fflush(stdout);
                    }
                }
            }
        }

        restore_input();
        ss_close_audio(audio);
        cr_destroy_engine(engine);
    }
    return 0;
}
