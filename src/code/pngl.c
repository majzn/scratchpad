/*
 * test_harness.c: Example program demonstrating the use of the single-header
 * libraries (plat.h, args.h, and pngw.h content).
 */

// --- Required C standard library headers for the implementations ---
// Note: We include these standard headers first, as the library implementations rely on them.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h> 
#include <assert.h> 
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#ifdef _WIN32
#include <direct.h>
#include <process.h>
#include <windows.h>
#else
#include <dirent.h>
#include <dlfcn.h>
#include <limits.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#endif
#include <ctype.h> // Required for tolower in utility functions (mzn_rect_pack.c context)


// --- 1. Define Opaque Type ArgParseState (Fix for incomplete type) ---
// This must be defined before the ArgParseState is used as a member in AppState.
typedef struct State_t {
    // args.h uses ArgParseState internally, often as a pointer. 
    // This minimal definition makes it a complete type.
    int dummy_placeholder; 
} ArgParseState;


// --- 2. Define Implementation Macros ---
// These macros instruct the included header files to output their implementation code.
#define PLATFORM_IMPLEMENTATION
#define ARG_PARSE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION


// --- 3. Include Single-Header Libraries (This is the clean way) ---
// The preprocessor will substitute the entire content of these files, 
// including both the interface (declarations) and the implementation (definitions)
// due to the macros defined above.
#include "plat.h"
#include "args.h"
#include "pngw.h"


// --- 4. Main Application Logic ---

// Custom State Structure for args.h
typedef struct {
  ArgParseState base; 
  const char *output_file;
  int width;
  float scale;
} AppState;

// Required Handler from the original context (mzn_rect_pack.c)
int utility_flag_handler(ArgParseState *argState, char **argv, int argc, int *i_ptr) {
    if (strcmp(argv[*i_ptr], "-h") == 0 ||
             strcmp(argv[*i_ptr], "--help") == 0) {
        return -1; // Signals argparse_process_args to print help and exit
    }
    return 1;
}

// Argument Handlers
int handle_output(ArgParseState *argState, char **argv, int argc, int *i_ptr) {
  AppState *state = (AppState *)argState;
  return argparse_parse_string(argv, argc, i_ptr, &state->output_file, "Output File");
}

int handle_scale(ArgParseState *argState, char **argv, int argc, int *i_ptr) {
  AppState *state = (AppState *)argState;
  if (argparse_parse_float(argv, argc, i_ptr, &state->scale, "Scale Factor")) {
    return 1;
  }
  return 0;
}

// Option Table
ArgOption global_option_table[] = {
    {"-h", "--help", utility_flag_handler, "Show this help message.", 0, 0},
    {"-o", "--output", handle_output, "Output PNG filename <val> (Mandatory).", 1, 0},
    {"-s", "--scale", handle_scale, "Scaling factor for demonstration <val> (float, default: 1.0).", 0, 0},
    {NULL, NULL, NULL, NULL, 0, 0}};

// Function to generate and save a simple PNG
int generate_and_save_png(const char *filename, int width, int height, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    int channels = 4;
    int stride = width * channels;
    // Note: platform_malloc/free are wrappers around platform_safe_malloc/free in plat.h
    unsigned char *image_data = (unsigned char *)platform_malloc((size_t)width * height * channels);

    if (!image_data) {
        fprintf(stderr, "Failed to allocate memory for image data.\n");
        return 0;
    }

    // Fill the image with the specified color (Teal)
    for (int i = 0; i < width * height * channels; i += channels) {
        image_data[i + 0] = r; // Red
        image_data[i + 1] = g; // Green
        image_data[i + 2] = b; // Blue
        image_data[i + 3] = a; // Alpha (Opacity)
    }

    printf("Generating %dx%d image (R:%d G:%d B:%d A:%d) to: %s\n", width, height, r, g, b, a, filename);
    
    // stbi_write_png is now correctly defined because STB_IMAGE_WRITE_IMPLEMENTATION is set.
    int success = stbi_write_png(filename, width, height, channels, image_data, stride);

    if (success) {
        printf("Successfully saved PNG.\n");
    } else {
        fprintf(stderr, "Failed to save PNG. (Check file permissions/path).\n");
    }

    platform_free(image_data);
    return success;
}


int main(int argc, char **argv) {
  // Initialize minimal state
  AppState app_state = {
      .output_file = NULL,
      .width = 64,
      .scale = 1.0f
  };
  int parse_result;

  printf("--- Single-Header Library Test Harness ---\n");

  // 1. Argument Parsing
  parse_result = argparse_process_args((ArgParseState *)&app_state, argv, argc,
                                       global_option_table);

  if (parse_result != 0) {
    return 1;
  }

  // 2. Platform Abstraction Test
  long long timestamp = platform_get_timestamp_ms();
  printf("Current Timestamp (ms via plat.h): %lld\n", timestamp);
  
  char cwd_buffer[MAX_PATH_CUSTOM];
  if (platform_get_current_working_dir(cwd_buffer, MAX_PATH_CUSTOM)) {
    printf("Current Working Dir (via plat.h): %s\n", cwd_buffer);
  }

  // 3. Image Generation Test
  if (app_state.output_file) {
      printf("Scale factor parsed: %.2f\n", app_state.scale);
      
      // Generate a fixed size image (64x64, Teal with full opacity)
      int img_width = 64*app_state.scale;
      int img_height = 64*app_state.scale;
      unsigned char r = 0;
      unsigned char g = 128;
      unsigned char b = 128;
      unsigned char a = 255; // Fully opaque

      if (!generate_and_save_png(app_state.output_file, img_width, img_height, r, g, b, a)) {
          fprintf(stderr, "Application failed to generate image.\n");
          return 1;
      }
  }
  
  printf("--- Test Harness Complete ---\n");

  return 0;
}
