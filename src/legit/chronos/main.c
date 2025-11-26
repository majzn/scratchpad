#define _CRT_SECURE_NO_WARNINGS
#define CHRONOS_IMPLEMENTATION
#include "chronos.h"
#define SS_ENGINE_IMPLEMENTATION
#include "ss.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static cr_context* ctx = NULL;

void app_audio_play(short *buf, int n_frames, int s_rate, int ch, void *udata) {
    int i;
    int c;

    cr_lock(ctx);
    for (i = 0; i < n_frames; i++) {
        ctx->global_time++;
        for (c = 0; c < ch; c++) {
            double val;
            
            val = cr_process(ctx, c);
            
            val = tanh(val); 
            
            if (val > 1.0) val = 1.0;
            else if (val < -1.0) val = -1.0;
            
            buf[i * ch + c] = (short)(val * 32000.0);
        }
    }
    cr_unlock(ctx);
}

int main(int argc, char **argv) {
    char line[1024];
    ss_audio_t* audio;
    int log_lvl;
    char* file_to_load = NULL;
    char* script_content = NULL;
    int i;
    FILE* f;
    long sz;
    
    log_lvl = CR_LOG_DEBUG; 
    
    for(i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-v") == 0) log_lvl = CR_LOG_INFO;
        else if(strcmp(argv[i], "-d") == 0) log_lvl = CR_LOG_DEBUG;
        else if(strcmp(argv[i], "-q") == 0) log_lvl = CR_LOG_NONE;
        else file_to_load = argv[i];
    }
    
    printf("Chronos v8.3 (Soft Clip Edition)\n");
    ctx = cr_create_context();
    cr_set_log_level(ctx, log_lvl);

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
            }
            fclose(f);
        } else {
            printf("Could not open file '%s'\n", file_to_load);
        }
    }

    printf("Attempting ss_open_audio...\n"); 
    audio = ss_open_audio(app_audio_play, NULL, 48000, 2, 1024, NULL);
    if (!audio) { 
        printf("Audio failed.\n"); 
        if (script_content) free(script_content);
        return 1; 
    }
    
    /* FIX: The audio->s_rate access is now valid because ss_audio_t exposes s_rate */
    cr_lock(ctx);
    ctx->sample_rate = (double)audio->s_rate;
    cr_unlock(ctx);
    
    printf("Audio initialized successfully. System Sample Rate: %d Hz\n", audio->s_rate);

    if (script_content) {
        cr_lock(ctx);
        if(cr_run(ctx, script_content)) printf("Script loaded successfully.\n");
        cr_unlock(ctx);
        free(script_content);
        script_content = NULL;
    }
    
    printf("REPL Ready. Type 'quit' to exit, 'inspect(var)' to debug.\n");
    while (1) {
        char* fname;
        int ok;
        char* buf;

        printf("> ");
        if (!fgets(line, 1024, stdin)) break;
        line[strcspn(line, "\n")] = 0;
        if (strcmp(line, "quit") == 0) break;
        if (strlen(line) == 0) continue;
        if (strncmp(line, ":load ", 6) == 0) {
            fname = line + 6;
            f = fopen(fname, "rb");
            if (f) {
                fseek(f, 0, SEEK_END);
                sz = ftell(f);
                fseek(f, 0, SEEK_SET);
                buf = (char*)malloc(sz + 1);
                if (buf) {
                    fread(buf, 1, sz, f);
                    buf[sz] = 0;
                    cr_lock(ctx);
                    ok = cr_run(ctx, buf);
                    cr_unlock(ctx);
                    free(buf);
                    if(ok) printf("Loaded.\n");
                }
                fclose(f);
            } else {
                printf("File not found.\n");
            }
            continue;
        }

        cr_lock(ctx);
        ok = cr_run(ctx, line);
        cr_unlock(ctx);
        if (ok) printf("OK\n");
    }
    ss_close_audio(audio);
    cr_destroy_context(ctx);
    return 0;
}