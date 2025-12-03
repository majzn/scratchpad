#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <io.h>
#define PATH_SEP '\\'
#define mkdir_portable(path) _mkdir(path)
#define access_portable(path, mode) _access(path, mode)
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#define PATH_SEP '/'
#define mkdir_portable(path) mkdir(path, 0755)
#define access_portable(path, mode) access(path, mode)
#endif

#define MAX_LINE 16384
#define MAX_PATH_LEN 512
#define BUFFER_SIZE 65536

typedef enum {
    STATE_NORMAL,
    STATE_STRING,
    STATE_CHAR,
    STATE_C89_COMMENT,
    STATE_CPP_COMMENT,
    STATE_ESCAPE
} ParserState;

typedef struct {
    ParserState state;
    ParserState prev_state;
    char string_delimiter;
    int line_number;
    int column;
} ParserContext;

typedef struct {
    int remove_c89_comments;
    int remove_cpp_comments;
    int in_place;
    int create_backup;
    int verbose;
    int run_clang_format;
    char* output_dir;
    char* clang_format_style;
    char** input_patterns;
    int pattern_count;
} Options;

typedef struct {
    char** files;
    int count;
    int capacity;
} FileList;

void print_usage(const char* prog_name) {
    printf("C Source Code Formatter v1.0\n");
    printf("Usage: %s [options] <input_files...>\n\n", prog_name);
    printf("Options:\n");
    printf("  -c89          Remove C89 style comments (/* */)\n");
    printf("  -cpp          Remove C++ style comments (//)\n");
    printf("  -all          Remove all comments\n");
    printf("  -i            In-place modification\n");
    printf("  -backup       Create .bak backup files (with -i)\n");
    printf("  -o <dir>      Output directory\n");
    printf("  -format       Run clang-format after processing\n");
    printf("  -style <name> clang-format style (default: LLVM)\n");
    printf("  -v            Verbose output\n");
    printf("  -h, --help    Show this help message\n\n");
    printf("Supports wildcards: *.c, *.h, etc.\n\n");
    printf("Examples:\n");
    printf("  %s -all *.c *.h\n", prog_name);
    printf("  %s -c89 -i -backup file.c\n", prog_name);
    printf("  %s -all -o output src/*.c\n", prog_name);
    printf("  %s -all -format -style Google *.c\n", prog_name);
}

void init_options(Options* opts) {
    opts->remove_c89_comments = 0;
    opts->remove_cpp_comments = 0;
    opts->in_place = 0;
    opts->create_backup = 0;
    opts->verbose = 0;
    opts->run_clang_format = 0;
    opts->output_dir = NULL;
    opts->clang_format_style = "LLVM";
    opts->input_patterns = NULL;
    opts->pattern_count = 0;
}

void init_parser_context(ParserContext* ctx) {
    ctx->state = STATE_NORMAL;
    ctx->prev_state = STATE_NORMAL;
    ctx->string_delimiter = 0;
    ctx->line_number = 1;
    ctx->column = 0;
}

void init_file_list(FileList* list) {
    list->files = NULL;
    list->count = 0;
    list->capacity = 0;
}

void free_file_list(FileList* list) {
    int i;
    if (list->files) {
        for (i = 0; i < list->count; i++) {
            free(list->files[i]);
        }
        free(list->files);
    }
}

int add_file(FileList* list, const char* filename) {
    char* copy;
    int i;
    
    for (i = 0; i < list->count; i++) {
        if (strcmp(list->files[i], filename) == 0) {
            return 1;
        }
    }
    
    if (list->count >= list->capacity) {
        int new_cap = list->capacity == 0 ? 32 : list->capacity * 2;
        char** new_files = (char**)realloc(list->files, new_cap * sizeof(char*));
        if (!new_files) {
            return 0;
        }
        list->files = new_files;
        list->capacity = new_cap;
    }
    
    copy = (char*)malloc(strlen(filename) + 1);
    if (!copy) {
        return 0;
    }
    strcpy(copy, filename);
    list->files[list->count++] = copy;
    return 1;
}

int matches_pattern(const char* filename, const char* pattern) {
    const char* f = filename;
    const char* p = pattern;
    const char* star_f = NULL;
    const char* star_p = NULL;
    
    while (*f) {
        if (*p == '*') {
            star_p = p++;
            star_f = f;
        } else if (*p == '?' || tolower((unsigned char)*f) == tolower((unsigned char)*p)) {
            f++;
            p++;
        } else if (star_p) {
            p = star_p + 1;
            f = ++star_f;
        } else {
            return 0;
        }
    }
    
    while (*p == '*') {
        p++;
    }
    
    return *p == '\0';
}

#ifdef _WIN32
void expand_pattern(const char* pattern, FileList* list) {
    WIN32_FIND_DATAA find_data;
    HANDLE hFind;
    char dir_path[MAX_PATH_LEN];
    char full_path[MAX_PATH_LEN];
    char* last_sep;
    
    strncpy(dir_path, pattern, MAX_PATH_LEN - 1);
    dir_path[MAX_PATH_LEN - 1] = '\0';
    
    last_sep = strrchr(dir_path, PATH_SEP);
    if (last_sep) {
        *last_sep = '\0';
    } else {
        strcpy(dir_path, ".");
    }
    
    hFind = FindFirstFileA(pattern, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        if (strchr(pattern, '*') == NULL && strchr(pattern, '?') == NULL) {
            add_file(list, pattern);
        }
        return;
    }
    
    do {
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            if (strcmp(dir_path, ".") == 0) {
                add_file(list, find_data.cFileName);
            } else {
                snprintf(full_path, MAX_PATH_LEN, "%s%c%s", dir_path, PATH_SEP, find_data.cFileName);
                add_file(list, full_path);
            }
        }
    } while (FindNextFileA(hFind, &find_data));
    
    FindClose(hFind);
}
#else
void expand_pattern(const char* pattern, FileList* list) {
    DIR* dir;
    struct dirent* entry;
    struct stat st;
    char dir_path[MAX_PATH_LEN];
    char full_path[MAX_PATH_LEN];
    char* base_pattern;
    char* last_sep;
    int has_wildcard;
    
    has_wildcard = (strchr(pattern, '*') != NULL || strchr(pattern, '?') != NULL);
    
    if (!has_wildcard) {
        add_file(list, pattern);
        return;
    }
    
    strncpy(dir_path, pattern, MAX_PATH_LEN - 1);
    dir_path[MAX_PATH_LEN - 1] = '\0';
    
    last_sep = strrchr(dir_path, PATH_SEP);
    if (last_sep) {
        *last_sep = '\0';
        base_pattern = last_sep + 1;
    } else {
        strcpy(dir_path, ".");
        base_pattern = (char*)pattern;
    }
    
    dir = opendir(dir_path);
    if (!dir) {
        if (!has_wildcard) {
            add_file(list, pattern);
        }
        return;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }
        
        if (matches_pattern(entry->d_name, base_pattern)) {
            if (strcmp(dir_path, ".") == 0) {
                snprintf(full_path, MAX_PATH_LEN, "%s", entry->d_name);
            } else {
                snprintf(full_path, MAX_PATH_LEN, "%s%c%s", dir_path, PATH_SEP, entry->d_name);
            }
            
            if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
                add_file(list, full_path);
            }
        }
    }
    
    closedir(dir);
}
#endif

int parse_args(int argc, char** argv, Options* opts) {
    int i;
    int pattern_start = -1;
    
    init_options(opts);
    
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c89") == 0) {
            opts->remove_c89_comments = 1;
        } else if (strcmp(argv[i], "-cpp") == 0) {
            opts->remove_cpp_comments = 1;
        } else if (strcmp(argv[i], "-all") == 0) {
            opts->remove_c89_comments = 1;
            opts->remove_cpp_comments = 1;
        } else if (strcmp(argv[i], "-i") == 0) {
            opts->in_place = 1;
        } else if (strcmp(argv[i], "-backup") == 0) {
            opts->create_backup = 1;
        } else if (strcmp(argv[i], "-v") == 0) {
            opts->verbose = 1;
        } else if (strcmp(argv[i], "-format") == 0) {
            opts->run_clang_format = 1;
        } else if (strcmp(argv[i], "-style") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -style requires style argument\n");
                return -1;
            }
            opts->clang_format_style = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -o requires directory argument\n");
                return -1;
            }
            opts->output_dir = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            return -1;
        } else {
            if (pattern_start == -1) {
                pattern_start = i;
            }
        }
    }
    
    if (pattern_start == -1) {
        fprintf(stderr, "Error: No input files specified\n");
        return -1;
    }
    
    opts->input_patterns = &argv[pattern_start];
    opts->pattern_count = argc - pattern_start;
    
    if (opts->in_place && opts->output_dir) {
        fprintf(stderr, "Error: Cannot use -i and -o together\n");
        return -1;
    }
    
    return 0;
}



void process_char(char c, char** out_ptr, ParserContext* ctx, const Options* opts) {
    char* out = *out_ptr;
    
    if (c == '\n') {
        ctx->line_number++;
        ctx->column = 0;
    } else {
        ctx->column++;
    }
    
    switch (ctx->state) {
        case STATE_ESCAPE:
            *out++ = c;
            ctx->state = ctx->prev_state;
            break;
            
        case STATE_STRING:
            if (c == '\\') {
                *out++ = c;
                ctx->prev_state = STATE_STRING;
                ctx->state = STATE_ESCAPE;
            } else if (c == ctx->string_delimiter) {
                *out++ = c;
                ctx->state = STATE_NORMAL;
                ctx->string_delimiter = 0;
            } else {
                *out++ = c;
            }
            break;
            
        case STATE_CHAR:
            if (c == '\\') {
                *out++ = c;
                ctx->prev_state = STATE_CHAR;
                ctx->state = STATE_ESCAPE;
            } else if (c == '\'') {
                *out++ = c;
                ctx->state = STATE_NORMAL;
            } else {
                *out++ = c;
            }
            break;
            
        case STATE_C89_COMMENT:
            if (!opts->remove_c89_comments) {
                *out++ = c;
            }
            break;
            
        case STATE_CPP_COMMENT:
            if (!opts->remove_cpp_comments) {
                *out++ = c;
            }
            if (c == '\n') {
                ctx->state = STATE_NORMAL;
            }
            break;
            
        case STATE_NORMAL:
            if (c == '"') {
                *out++ = c;
                ctx->state = STATE_STRING;
                ctx->string_delimiter = '"';
            } else if (c == '\'') {
                *out++ = c;
                ctx->state = STATE_CHAR;
            } else {
                *out++ = c;
            }
            break;
    }
    
    *out_ptr = out;
}

int process_buffer(const char* input, size_t input_len, char* output, 
                   size_t* output_len, ParserContext* ctx, const Options* opts) {
    const char* in = input;
    const char* in_end = input + input_len;
    char* out = output;
    
    while (in < in_end) {
        char c = *in;
        char next = (in + 1 < in_end) ? *(in + 1) : '\0';
        
        if (ctx->state == STATE_NORMAL) {
            if (c == '/' && next == '*' && opts->remove_c89_comments) {
                ctx->state = STATE_C89_COMMENT;
                in += 2;
                continue;
            } else if (c == '/' && next == '/' && opts->remove_cpp_comments) {
                ctx->state = STATE_CPP_COMMENT;
                in += 2;
                continue;
            }
        } else if (ctx->state == STATE_C89_COMMENT) {
            if (c == '*' && next == '/') {
                ctx->state = STATE_NORMAL;
                in += 2;
                continue;
            }
        }
        
        process_char(c, &out, ctx, opts);
        in++;
    }
    
    *output_len = out - output;
    return 0;
}

int create_backup(const char* filename) {
    char backup_path[MAX_PATH_LEN];
    FILE* src;
    FILE* dst;
    char buffer[BUFFER_SIZE];
    size_t bytes;
    
    snprintf(backup_path, MAX_PATH_LEN, "%s.bak", filename);
    
    src = fopen(filename, "rb");
    if (!src) {
        return -1;
    }
    
    dst = fopen(backup_path, "wb");
    if (!dst) {
        fclose(src);
        return -1;
    }
    
    while ((bytes = fread(buffer, 1, BUFFER_SIZE, src)) > 0) {
        if (fwrite(buffer, 1, bytes, dst) != bytes) {
            fclose(src);
            fclose(dst);
            return -1;
        }
    }
    
    fclose(src);
    fclose(dst);
    return 0;
}

void get_output_path(const char* input, const Options* opts, char* output, size_t output_size) {
    const char* basename;
    const char* last_sep;
    const char* extension;
    char name_without_ext[MAX_PATH_LEN];
    size_t name_len;
    
    if (opts->in_place) {
        strncpy(output, input, output_size - 1);
        output[output_size - 1] = '\0';
        return;
    }
    
    last_sep = strrchr(input, PATH_SEP);
    basename = last_sep ? last_sep + 1 : input;
    
    extension = strrchr(basename, '.');
    
    if (extension && extension != basename) {
        name_len = extension - basename;
        if (name_len >= MAX_PATH_LEN) {
            name_len = MAX_PATH_LEN - 1;
        }
        strncpy(name_without_ext, basename, name_len);
        name_without_ext[name_len] = '\0';
    } else {
        strncpy(name_without_ext, basename, MAX_PATH_LEN - 1);
        name_without_ext[MAX_PATH_LEN - 1] = '\0';
        extension = "";
    }
    
    if (opts->output_dir) {
        snprintf(output, output_size, "%s%c%s.fmt%s", 
                opts->output_dir, PATH_SEP, name_without_ext, extension);
    } else {
        if (last_sep) {
            char dir_path[MAX_PATH_LEN];
            size_t dir_len = last_sep - input + 1;
            if (dir_len >= MAX_PATH_LEN) {
                dir_len = MAX_PATH_LEN - 1;
            }
            strncpy(dir_path, input, dir_len);
            dir_path[dir_len] = '\0';
            snprintf(output, output_size, "%s%s.fmt%s", 
                    dir_path, name_without_ext, extension);
        } else {
            snprintf(output, output_size, "%s.fmt%s", name_without_ext, extension);
        }
    }
}

int format_file(const char* input_file, const Options* opts) {
    FILE* input;
    FILE* output;
    char* input_buffer;
    char* output_buffer;
    char* final_output;
    char output_path[MAX_PATH_LEN];
    char temp_path[MAX_PATH_LEN];
    size_t input_size;
    size_t output_size;
    size_t bytes_read;
    size_t final_size;
    ParserContext ctx;
    const char* write_path;
    
    input = fopen(input_file, "rb");
    if (!input) {
        fprintf(stderr, "Error: Cannot open input file '%s': %s\n", 
                input_file, strerror(errno));
        return 1;
    }
    
    fseek(input, 0, SEEK_END);
    input_size = ftell(input);
    fseek(input, 0, SEEK_SET);
    
    if (input_size == 0) {
        fclose(input);
        if (opts->verbose) {
            printf("Skipped (empty): %s\n", input_file);
        }
        return 0;
    }
    
    input_buffer = (char*)malloc(input_size + 1);
    output_buffer = (char*)malloc(input_size * 2 + 1);
    
    if (!input_buffer || !output_buffer) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        if (input_buffer) free(input_buffer);
        if (output_buffer) free(output_buffer);
        fclose(input);
        return 1;
    }
    
    bytes_read = fread(input_buffer, 1, input_size, input);
    fclose(input);
    
    if (bytes_read != input_size) {
        fprintf(stderr, "Error: Failed to read file '%s'\n", input_file);
        free(input_buffer);
        free(output_buffer);
        return 1;
    }
    
    init_parser_context(&ctx);
    
    if (process_buffer(input_buffer, input_size, output_buffer, 
                       &output_size, &ctx, opts) != 0) {
        fprintf(stderr, "Error: Processing failed for '%s'\n", input_file);
        free(input_buffer);
        free(output_buffer);
        return 1;
    }
    
    free(input_buffer);
    output_buffer[output_size] = '\0';
    
    final_output = output_buffer;
    final_size = output_size;
    
    get_output_path(input_file, opts, output_path, MAX_PATH_LEN);
    
    if (opts->in_place) {
        if (opts->create_backup) {
            if (create_backup(input_file) != 0) {
                fprintf(stderr, "Warning: Could not create backup for '%s'\n", input_file);
            }
        }
        snprintf(temp_path, MAX_PATH_LEN, "%s.tmp", input_file);
        write_path = temp_path;
    } else {
        if (opts->output_dir && access_portable(opts->output_dir, 0) != 0) {
            if (mkdir_portable(opts->output_dir) != 0 && errno != EEXIST) {
                fprintf(stderr, "Error: Cannot create output directory '%s'\n", opts->output_dir);
                free(final_output);
                return 1;
            }
        }
        write_path = output_path;
    }
    
    output = fopen(write_path, "wb");
    if (!output) {
        fprintf(stderr, "Error: Cannot open output file '%s': %s\n", 
                write_path, strerror(errno));
        free(final_output);
        return 1;
    }
    
    if (fwrite(final_output, 1, final_size, output) != final_size) {
        fprintf(stderr, "Error: Failed to write to '%s'\n", write_path);
        fclose(output);
        free(final_output);
        return 1;
    }
    
    fclose(output);
    free(final_output);
    
    if (opts->in_place) {
        if (remove(input_file) != 0) {
            fprintf(stderr, "Error: Cannot remove original file '%s'\n", input_file);
            return 1;
        }
        if (rename(temp_path, input_file) != 0) {
            fprintf(stderr, "Error: Cannot rename temp file to '%s'\n", input_file);
            return 1;
        }
    }
    
    if (opts->verbose) {
        printf("Formatted: %s -> %s\n", input_file, output_path);
    }
    
    return 0;
}

int run_clang_format(const char* filename, const char* style, int verbose) {
    char command[MAX_PATH_LEN * 2];
    int result;
    
#ifdef _WIN32
    snprintf(command, sizeof(command), "clang-format.exe -i -style=%s \"%s\" 2>nul", 
             style, filename);
#else
    snprintf(command, sizeof(command), "clang-format -i -style=%s \"%s\" 2>/dev/null", 
             style, filename);
#endif
    
    if (verbose) {
        printf("Running: %s\n", command);
    }
    
    result = system(command);
    
    if (result != 0) {
        if (verbose) {
            fprintf(stderr, "Warning: clang-format failed for '%s' (exit code: %d)\n", 
                    filename, result);
        }
        return 1;
    }
    
    return 0;
}

int check_clang_format_available(void) {
    int result;
    
#ifdef _WIN32
    result = system("clang-format.exe --version >nul 2>&1");
#else
    result = system("clang-format --version >/dev/null 2>&1");
#endif
    
    return result == 0;
}
int main(int argc, char** argv) {
    Options opts;
    FileList file_list;
    int i;
    int errors = 0;
    int processed = 0;
    int format_errors = 0;
    
    if (parse_args(argc, argv, &opts) != 0) {
        print_usage(argv[0]);
        return 1;
    }
    
    if (opts.run_clang_format) {
        if (!check_clang_format_available()) {
            fprintf(stderr, "Error: clang-format is not available in PATH\n");
            fprintf(stderr, "Please install clang-format or remove the -format option\n");
            return 1;
        }
        if (opts.verbose) {
            printf("clang-format is available\n");
        }
    }
    
    init_file_list(&file_list);
    
    for (i = 0; i < opts.pattern_count; i++) {
        expand_pattern(opts.input_patterns[i], &file_list);
    }
    
    if (file_list.count == 0) {
        fprintf(stderr, "Error: No files matched the specified patterns\n");
        free_file_list(&file_list);
        return 1;
    }
    
    printf("Processing %d file(s)...\n", file_list.count);
    
    for (i = 0; i < file_list.count; i++) {
        if (format_file(file_list.files[i], &opts) == 0) {
            processed++;
            
            if (opts.run_clang_format) {
                char format_target[MAX_PATH_LEN];
                
                if (opts.in_place) {
                    strncpy(format_target, file_list.files[i], MAX_PATH_LEN - 1);
                    format_target[MAX_PATH_LEN - 1] = '\0';
                } else {
                    get_output_path(file_list.files[i], &opts, format_target, MAX_PATH_LEN);
                }
                
                if (run_clang_format(format_target, opts.clang_format_style, opts.verbose) != 0) {
                    format_errors++;
                }
            }
        } else {
            errors++;
        }
    }
    
    free_file_list(&file_list);
    
    printf("\nCompleted: %d processed, %d errors", processed, errors);
    if (opts.run_clang_format) {
        printf(", %d clang-format errors", format_errors);
    }
    printf("\n");
    
    return errors > 0 ? 1 : 0;
}
