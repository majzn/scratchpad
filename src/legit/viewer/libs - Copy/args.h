#ifndef ARGPARSE_H
#define ARGPARSE_H

int arg_parse_init(void);

typedef struct State_t ArgParseState;

typedef int (*ArgParseFunc)(ArgParseState *state, char **argv, int argc,
                            int *i_ptr);

typedef struct {
  const char *shortName;
  const char *longName;
  ArgParseFunc handler;
  const char *helpMsg;
  int is_mandatory;
  int was_present;
} ArgOption;

void argparse_print_help(char **argv, ArgOption *table);
int argparse_process_args(ArgParseState *state, char **argv, int argc,
                          ArgOption *table);
int argparse_parse_float(char **argv, int argc, int *i_ptr, float *target,
                         const char *name);
int argparse_parse_string(char **argv, int argc, int *i_ptr,
                          const char **target, const char *name);

#ifdef ARG_PARSE_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void argparse_print_help(char **argv, ArgOption *table) {
  int i;
  printf("Usage: %s [options]\n", argv[0]);
  printf("Options:\n");
  for (i = 0; table[i].shortName != NULL; i++) {
    const char *sName = table[i].shortName;
    const char *lName = table[i].longName;
    const char *help = table[i].helpMsg;
    int needs_value = (strstr(help, "<val>") != NULL);

    if (needs_value) {
      printf("  %s <val>              %s <val>\t%s%s\n", sName, lName, help,
             table[i].is_mandatory ? " (Mandatory)" : "");
    } else {
      printf("  %s                   %s\t\t%s%s\n", sName, lName, help,
             table[i].is_mandatory ? " (Mandatory)" : "");
    }
  }
}

int argparse_process_args(ArgParseState *state, char **argv, int argc,
                          ArgOption *table) {
  int i;
  int j;
  int consumed;

  for (i = 0; table[i].shortName != NULL; i++) {
    table[i].was_present = 0;
  }

  for (i = 1; i < argc; i++) {
    consumed = 0;
    for (j = 0; table[j].shortName != NULL; j++) {
      int handlerResult = 1;
      if (strcmp(argv[i], table[j].shortName) == 0 ||
          (table[j].longName && strcmp(argv[i], table[j].longName) == 0)) {

        table[j].was_present = 1;

        if (table[j].handler) {
          handlerResult = table[j].handler(state, argv, argc, &i);

          if (handlerResult == -1) {
            argparse_print_help(argv, table);
            exit(EXIT_SUCCESS);
          }
          if (handlerResult == 0) {
            return 1;
          }
        }
        consumed = 1;
        break;
      }
    }

    if (!consumed) {
      fprintf(stderr, "ERROR: Unknown argument: %s.\n", argv[i]);
      argparse_print_help(argv, table);
      return 1;
    }
  }

  for (i = 0; table[i].shortName != NULL; i++) {
    if (table[i].is_mandatory && !table[i].was_present) {
      fprintf(stderr, "ERROR: Mandatory option %s (%s) is missing.\n",
              table[i].shortName,
              table[i].longName ? table[i].longName : "N/A");
      argparse_print_help(argv, table);
      return 1;
    }
  }

  return 0;
}

int argparse_parse_float(char **argv, int argc, int *i_ptr, float *target,
                         const char *name) {
  int i = *i_ptr;
  float val;
  char *endptr;

  if (i + 1 >= argc) {
    fprintf(stderr, "ERROR: Missing numeric value for argument %s (%s).\n",
            argv[i], name);
    return 0;
  }

  val = (float)strtod(argv[i + 1], &endptr);

  if (*endptr != '\0' || endptr == argv[i + 1]) {
    fprintf(stderr,
            "ERROR: Invalid numeric format for %s (%s). Expected a float.\n",
            argv[i], name);
    return 0;
  }

  *target = val;
  *i_ptr = i + 1;
  return 1;
}

int argparse_parse_string(char **argv, int argc, int *i_ptr,
                          const char **target, const char *name) {
  int i = *i_ptr;

  if (i + 1 >= argc) {
    fprintf(stderr,
            "ERROR: Missing value for argument %s (%s). Expected a string.\n",
            argv[i], name);
    return 0;
  }

  *target = argv[i + 1];
  *i_ptr = i + 1;
  return 1;
}

int arg_parse_init(void) { return 0; }

#endif /* ARG_PARSE_IMPLEMENTATION */

#endif
