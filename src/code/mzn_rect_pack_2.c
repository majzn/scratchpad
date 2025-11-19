#define _CRT_SECURE_NO_WARNINGS
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PLATFORM_IMPLEMENTATION
#define ARG_PARSE_IMPLEMENTATION

#include "args.h"
#include "plat.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define MAX_RECTS 65536
#define MAX_EXT_LEN 256
#define MAX_FILENAME 256
#ifndef MAX_PATH_CUSTOM
#define MAX_PATH_CUSTOM 512
#endif

#define CAST_STATE(S) ((ArgParseState *)(S))

typedef struct {
  int width;
  int height;
  int x;
  int y;
  int area;
  char path[MAX_PATH_CUSTOM];
  unsigned char *data;
  int channels;
  int placed;
  int originalWidth;
  int originalHeight;
  int currentWidth;
  int currentHeight;
  int trimX;
  int trimY;
  int trimmedWidth;
  int trimmedHeight;
  int rotated;
} Rect;
typedef struct {
  int width;
  int height;
  int area;
} Box;
typedef struct {
  int x;
  int y;
  int width;
  int height;
} FreeRect;
typedef struct {
  Rect *rects;
  int count;
  Box box;
  int debug;
  float scaleIn;
  unsigned char bgColor[4];
  char outputFormat[10];
  char allowedExtsPositive[MAX_EXT_LEN];
  char allowedExtsNegative[MAX_EXT_LEN];
  char mapFilename[MAX_FILENAME];
  char inputDir[MAX_FILENAME];
  char outputBaseFilename[MAX_FILENAME];
  float weightArea;
  float weightShort;
  float weightLong;
  int allowRotation;
} State;
FreeRect *GLOBAL_FREE_RECTS;
int GLOBAL_FREE_COUNT;
int GLOBAL_FREE_CAP;

int utility_parse_color(const char *str, unsigned char *color) {
  int r, g, b, a;
  if (sscanf(str, "%d,%d,%d,%d", &r, &g, &b, &a) != 4) {
    return 0;
  }
  if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255 || a < 0 ||
      a > 255) {
    return 0;
  }
  color[0] = (unsigned char)r;
  color[1] = (unsigned char)g;
  color[2] = (unsigned char)b;
  color[3] = (unsigned char)a;
  return 1;
}
int utility_case_insensitive_strcmp(const char *s1, const char *s2) {
  unsigned char c1, c2;
  for (;;) {
    c1 = (unsigned char)*s1++;
    c2 = (unsigned char)*s2++;
    if (c1 != c2) {
      c1 = (unsigned char)tolower(c1);
      c2 = (unsigned char)tolower(c2);
      if (c1 != c2) {
        return c1 < c2 ? -1 : 1;
      }
    }
    if (c1 == '\0') {
      break;
    }
  }
  return 0;
}

int utility_check_extension(const char *filename, const char *allowedPositive,
                            const char *allowedNegative) {
  const char *dot = strrchr(filename, '.');
  char ext[10];
  char *extCopy = (char *)NULL;
  char *token = (char *)NULL;
  int found = 0;

  if (!dot || dot == filename) {
    return (allowedPositive[0] == '\0');
  }
  strncpy(ext, dot + 1, 9);
  ext[9] = '\0';

  if (allowedNegative[0] != '\0') {
    extCopy = (char *)platform_malloc(strlen(allowedNegative) + 1);
    strcpy(extCopy, allowedNegative);
    token = strtok(extCopy, " ");
    while (token != (char *)NULL) {
      if (utility_case_insensitive_strcmp(ext, token) == 0) {
        platform_free(extCopy);
        return 0;
      }
      token = strtok((char *)NULL, " ");
    }
    platform_free(extCopy);
  }

  if (allowedPositive[0] == '\0' || strcmp(allowedPositive, "*") == 0) {
    return 1;
  }

  extCopy = (char *)platform_malloc(strlen(allowedPositive) + 1);
  strcpy(extCopy, allowedPositive);
  token = strtok(extCopy, " ");
  while (token != (char *)NULL) {
    if (utility_case_insensitive_strcmp(ext, token) == 0) {
      found = 1;
      break;
    }
    token = strtok((char *)NULL, " ");
  }
  platform_free(extCopy);
  return found;
}

int utility_parse_color_value(ArgParseState *argState, char **argv, int argc,
                              int *i_ptr) {
  State *state = (State *)argState;
  int i = *i_ptr;
  if (i + 1 >= argc) {
    fprintf(stderr, "ERROR: Missing value for argument %s.\n", argv[i]);
    return 0;
  }
  if (!utility_parse_color(argv[i + 1], state->bgColor)) {
    fprintf(stderr,
            "ERROR: Invalid color format for %s. Use R,G,B,A (e.g., "
            "255,0,0,255).\n",
            argv[i]);
    return 0;
  }
  *i_ptr = i + 1;
  return 1;
}

int utility_parse_output_name(ArgParseState *argState, char **argv, int argc,
                              int *i_ptr) {
  State *state = (State *)argState;
  int i = *i_ptr;
  const char *baseName;
  const char *dot;
  if (i + 1 >= argc) {
    fprintf(stderr, "ERROR: Missing output filename for %s.\n", argv[i]);
    return 0;
  }
  baseName = argv[i + 1];
  dot = strrchr(baseName, '.');
  if (dot && dot != baseName && (int)strlen(dot) <= 5) {
    strncpy(state->outputBaseFilename, baseName, (int)(dot - baseName));
    state->outputBaseFilename[dot - baseName] = '\0';
  } else {
    strncpy(state->outputBaseFilename, baseName, MAX_FILENAME - 1);
    state->outputBaseFilename[MAX_FILENAME - 1] = '\0';
  }
  *i_ptr = i + 1;
  return 1;
}

int utility_parse_map_file(ArgParseState *argState, char **argv, int argc,
                           int *i_ptr) {
  State *state = (State *)argState;
  int i = *i_ptr;
  if (i + 1 >= argc) {
    fprintf(stderr, "ERROR: Missing map filename for %s.\n", argv[i]);
    return 0;
  }
  strncpy(state->mapFilename, argv[i + 1], MAX_FILENAME - 1);
  state->mapFilename[MAX_FILENAME - 1] = '\0';
  *i_ptr = i + 1;
  return 1;
}

int utility_parse_exts(ArgParseState *argState, char **argv, int argc,
                       int *i_ptr) {
  State *state = (State *)argState;
  int i = *i_ptr;
  int posLen = 0;
  int negLen = 0;
  int consumed = 0;

  state->allowedExtsPositive[0] = '\0';
  state->allowedExtsNegative[0] = '\0';

  for (i = i + 1; i < argc && argv[i][0] != '-'; i++) {
    char *extStr = argv[i];
    if (extStr[0] == '!') {
      if (negLen + (int)strlen(extStr) + 1 > MAX_EXT_LEN) {
        fprintf(stderr, "ERROR: Negative extension list too long.\n");
        return 0;
      }
      if (negLen > 0) {
        state->allowedExtsNegative[negLen++] = ' ';
      }
      strcpy(state->allowedExtsNegative + negLen, extStr + 1);
      negLen += (int)strlen(extStr) - 1;
    } else {
      if (posLen + (int)strlen(extStr) + 1 > MAX_EXT_LEN) {
        fprintf(stderr, "ERROR: Positive extension list too long.\n");
        return 0;
      }
      if (posLen > 0) {
        state->allowedExtsPositive[posLen++] = ' ';
      }
      strcpy(state->allowedExtsPositive + posLen, extStr);
      posLen += (int)strlen(extStr);
    }
    consumed++;
  }
  *i_ptr = i - 1;
  if (consumed == 0) {
    fprintf(stderr, "ERROR: Missing extensions after -x.\n");
    return 0;
  }
  return 1;
}

int utility_flag_handler(ArgParseState *argState, char **argv, int argc,
                         int *i_ptr) {
  State *state = (State *)argState;
  if (strcmp(argv[*i_ptr], "-d") == 0 || strcmp(argv[*i_ptr], "--debug") == 0) {
    state->debug = 1;
  } else if (strcmp(argv[*i_ptr], "-h") == 0 ||
             strcmp(argv[*i_ptr], "--help") == 0) {
    return -1;
  } else if (strcmp(argv[*i_ptr], "-R") == 0 ||
             strcmp(argv[*i_ptr], "--rotation") == 0) {
    state->allowRotation = 1;
  }
  return 1;
}

int utility_input_dir_handler(ArgParseState *argState, char **argv, int argc,
                              int *i_ptr) {
  State *state = (State *)argState;
  int i = *i_ptr;
  if (i + 1 >= argc) {
    fprintf(stderr, "ERROR: Missing input directory for %s.\n", argv[i]);
    return 0;
  }
  strncpy(state->inputDir, argv[i + 1], MAX_FILENAME - 1);
  state->inputDir[MAX_FILENAME - 1] = '\0';
  *i_ptr = i + 1;
  return 1;
}

int utility_parse_format(ArgParseState *argState, char **argv, int argc,
                         int *i_ptr) {
  State *state = (State *)argState;
  int i = *i_ptr;
  char *fmtStr;
  if (i + 1 >= argc) {
    fprintf(stderr, "ERROR: Missing format string for %s.\n", argv[i]);
    return 0;
  }
  fmtStr = argv[i + 1];
  if (strcmp(fmtStr, "png") == 0 || strcmp(fmtStr, "jpg") == 0 ||
      strcmp(fmtStr, "jpeg") == 0 || strcmp(fmtStr, "bmp") == 0) {
    strncpy(state->outputFormat, fmtStr, 9);
    state->outputFormat[9] = '\0';
  } else {
    fprintf(stderr,
            "ERROR: Invalid output format. Choose png, jpg, or bmp for %s.\n",
            argv[i]);
    return 0;
  }
  *i_ptr = i + 1;
  return 1;
}

int utility_parse_wa_weight(ArgParseState *argState, char **argv, int argc,
                            int *i_ptr) {
  State *state = (State *)argState;
  return argparse_parse_float(argv, argc, i_ptr, &state->weightArea,
                              "Wasted Area");
}

int utility_parse_ssf_weight(ArgParseState *argState, char **argv, int argc,
                             int *i_ptr) {
  State *state = (State *)argState;
  return argparse_parse_float(argv, argc, i_ptr, &state->weightShort,
                              "Short Side Fit");
}

int utility_parse_lsf_weight(ArgParseState *argState, char **argv, int argc,
                             int *i_ptr) {
  State *state = (State *)argState;
  return argparse_parse_float(argv, argc, i_ptr, &state->weightLong,
                              "Long Side Fit");
}

ArgOption GLOBAL_OPTION_TABLE[] = {
    {"-h", "--help", utility_flag_handler, "Show this help message.", 0, 0},
    {"-d", "--debug", utility_flag_handler, "Enable verbose debug output.", 0,
     0},
    {"-R", "--rotation", utility_flag_handler,
     "Allow 90-degree rectangle rotation (increases efficiency).", 0, 0},
    {"-o", "--output", utility_parse_output_name,
     "Output base filename (default: packed).", 0, 0},
    {"-map", "--map-file", utility_parse_map_file,
     "Output YAML-like metadata map file (.txt format).", 0, 0},
    {"-i", "--input", utility_input_dir_handler,
     "Input directory (default: .).", 0, 0},
    {"-bgc", "--bg-color", utility_parse_color_value,
     "Canvas background color R,G,B,A (default: 0,0,0,0).", 0, 0},
    {"-fmt", "--format", utility_parse_format,
     "Output format: png, jpg, or bmp (default: png).", 0, 0},
    {"-x", "--exts", utility_parse_exts,
     "Space-delimited list of allowed extensions (prefix with ! to exclude).",
     0, 0},
    {"-WA", "--weight-area", utility_parse_wa_weight,
     "Weight for minimizing normalized wasted area (float, default: 1.0).", 0,
     0},
    {"-SSF", "--weight-short", utility_parse_ssf_weight,
     "Weight for minimizing normalized short side gap (float, default: 0.0).",
     0, 0},
    {"-LSF", "--weight-long", utility_parse_lsf_weight,
     "Weight for minimizing normalized long side gap (float, default: 0.0).", 0,
     0},
    {NULL, NULL, NULL, NULL, 0, 0}};

int utility_case_insensitive_strcmp(const char *s1, const char *s2);
int utility_parse_color(const char *str, unsigned char *color);

int compare_rect_area_desc(const void *a, const void *b) {
  return ((Rect *)b)->area - ((Rect *)a)->area;
}

void application_find_trim_box(Rect *rect) {
  int width, height, channels, x, y;
  unsigned char *data;
  int minX, maxX, minY, maxY;
  int foundPixel = 0;

  width = rect->originalWidth;
  height = rect->originalHeight;
  data = rect->data;
  channels = rect->channels;

  minX = width;
  maxX = 0;
  minY = height;
  maxY = 0;

  if (channels < 4) {
    rect->trimX = 0;
    rect->trimY = 0;
    rect->trimmedWidth = width;
    rect->trimmedHeight = height;
    return;
  }

  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
      if (data[(long)(y * width + x) * channels + 3] > 0) {
        foundPixel = 1;
        if (x < minX) {
          minX = x;
        }
        if (x > maxX) {
          maxX = x;
        }
        if (y < minY) {
          minY = y;
        }
        if (y > maxY) {
          maxY = y;
        }
      }
    }
  }

  if (!foundPixel) {
    rect->trimX = 0;
    rect->trimY = 0;
    rect->trimmedWidth = 0;
    rect->trimmedHeight = 0;
  } else {
    rect->trimX = minX;
    rect->trimY = minY;
    rect->trimmedWidth = maxX - minX + 1;
    rect->trimmedHeight = maxY - minY + 1;
  }
}

void application_remove_free_rect(int index) {
  if (index < GLOBAL_FREE_COUNT - 1) {
    GLOBAL_FREE_RECTS[index] = GLOBAL_FREE_RECTS[GLOBAL_FREE_COUNT - 1];
  }
  GLOBAL_FREE_COUNT--;
}

int application_pack_rects_internal(State *state) {
  int i;

  for (i = 0; i < state->count; i++) {
    state->rects[i].placed = 0;
    state->rects[i].rotated = 0;
  }

  return 0;
}

int application_pack_rects(State *state) {
  int i;
  long long totalArea = 0;
  int maxDim = 0;
  int currentTestWidth = 0;
  int currentTestHeight = 0;
  const char *rStr;

  for (i = 0; i < state->count; i++) {
    int dim = state->rects[i].currentWidth > state->rects[i].currentHeight
                  ? state->rects[i].currentWidth
                  : state->rects[i].currentHeight;
    if (dim > maxDim) {
      maxDim = dim;
    }
    totalArea +=
        (long long)state->rects[i].currentWidth * state->rects[i].currentHeight;
  }

  if (maxDim == 0 || totalArea == 0)
    return 0;

  currentTestWidth = maxDim;
  currentTestHeight = maxDim;

  rStr = state->allowRotation ? "ON" : "OFF";

  printf("Starting custom pack search with initial size %dx%d (WA: %.2f, SSF: "
         "%.2f, LSF: %.2f, Rotation: %s)...\n",
         currentTestWidth, currentTestHeight, state->weightArea,
         state->weightShort, state->weightLong, rStr);

  return 0;
}

char *application_find_next_filename(const char *baseFilename, const char *ext,
                                     char *buffer) {
  char testFilename[MAX_FILENAME];
  int version = 0;
  size_t baseLen;
  int i;
  char baseNoNum[MAX_FILENAME];
  int numDigits = 0;

  sprintf(testFilename, "%s.%s", baseFilename, ext);
  if (!platform_file_exists(testFilename)) {
    strcpy(buffer, testFilename);
    return buffer;
  }

  baseLen = strlen(baseFilename);
  for (i = (int)baseLen; i > 0; i--) {
    if (baseFilename[i - 1] >= '0' && baseFilename[i - 1] <= '9') {
      numDigits++;
    } else {
      break;
    }
  }

  if (numDigits > 0) {
    strncpy(baseNoNum, baseFilename, baseLen - numDigits);
    baseNoNum[baseLen - numDigits] = '\0';
    version = atoi(baseFilename + (baseLen - numDigits));
    version++;
  } else {
    strncpy(baseNoNum, baseFilename, MAX_FILENAME);
    baseNoNum[MAX_FILENAME - 1] = '\0';
    version = 1;
  }

  while (1) {
    sprintf(buffer, "%s%d.%s", baseNoNum, version, ext);
    if (!platform_file_exists(buffer)) {
      return buffer;
    }
    version++;
  }
}

void application_save_packed_image(State *state, const char *outputPath) {
  int finalWidth, finalHeight, chRgba;
  long imgSize;
  unsigned char *image;
  int i;

  finalWidth = state->box.width;
  finalHeight = state->box.height;
  chRgba = 4;
  imgSize = (long)finalWidth * finalHeight * chRgba;

  image = (unsigned char *)platform_malloc((size_t)imgSize);
  memset(image, 0, (size_t)imgSize);
  if (!image) {
    return;
  }

  printf("Writing packed data (%dx%d) to %s (Format: %s)...\n",
         state->box.width, state->box.height, outputPath, state->outputFormat);

  if (state->bgColor[0] != 0 || state->bgColor[1] != 0 ||
      state->bgColor[2] != 0 || state->bgColor[3] != 0) {
    int y, x;
    for (y = 0; y < finalHeight; y++) {
      for (x = 0; x < finalWidth; x++) {
        long idx = (long)(y * finalWidth + x) * chRgba;
        memcpy(&image[idx], state->bgColor, 4);
      }
    }
  }

  for (i = 0; i < state->count; i++) {
    Rect *rect = &state->rects[i];
    if (rect->x < 0 || rect->y < 0 || !rect->placed) {
      continue;
    }
    {
      const unsigned char *currentData = rect->data;
      int currentDataW = rect->originalWidth;
      int currentDataH = rect->originalHeight;
      float packedW = (float)rect->width;
      float packedH = (float)rect->height;
      int contentW = rect->trimmedWidth;
      int contentH = rect->trimmedHeight;
      int contentTrimX = rect->trimX;
      int contentTrimY = rect->trimY;

      if (currentData == (unsigned char *)NULL) {
        continue;
      }
      {
        int dstY, dstX;
        int startDstX = rect->x;
        int endDstX = rect->x + rect->width;
        int startDstY = rect->y;
        int endDstY = rect->y + rect->height;

        for (dstY = startDstY; dstY < endDstY && dstY < finalHeight; dstY++) {
          for (dstX = startDstX; dstX < endDstX && dstX < finalWidth; dstX++) {
            float normalizedX = (float)(dstX - rect->x) / packedW;
            float normalizedY = (float)(dstY - rect->y) / packedH;
            float finalSrcX;
            float finalSrcY;

            if (rect->rotated) {
              finalSrcX = normalizedY * contentW + (float)contentTrimX;
              finalSrcY = (1.0f - normalizedX) * contentH + (float)contentTrimY;
            } else {
              finalSrcX = normalizedX * contentW + (float)contentTrimX;
              finalSrcY = normalizedY * contentH + (float)contentTrimY;
            }
            {
              int srcX = (int)(floor(finalSrcX));
              int srcY = (int)(floor(finalSrcY));
              if (srcX >= 0 && srcX < currentDataW && srcY >= 0 &&
                  srcY < currentDataH) {
                long dstIdx = (long)(dstY * finalWidth + dstX) * chRgba;
                long srcIdx =
                    (long)(srcY * currentDataW + srcX) * rect->channels;
                unsigned char imgR, imgG, imgB, imgA;
                float aRatio, iARatio;

                imgR = currentData[srcIdx + 0];
                imgG = currentData[srcIdx + 1];
                imgB = currentData[srcIdx + 2];
                imgA = currentData[srcIdx + 3];

                aRatio = (float)imgA / 255.0f;
                iARatio = 1.0f - aRatio;

                image[dstIdx + 0] =
                    (unsigned char)(imgR * aRatio +
                                    image[dstIdx + 0] * iARatio);
                image[dstIdx + 1] =
                    (unsigned char)(imgG * aRatio +
                                    image[dstIdx + 1] * iARatio);
                image[dstIdx + 2] =
                    (unsigned char)(imgB * aRatio +
                                    image[dstIdx + 2] * iARatio);
                image[dstIdx + 3] =
                    (imgA > image[dstIdx + 3]) ? imgA : image[dstIdx + 3];
              }
            }
          }
        }
      }
    }
  }
  {
    int saveSuccess = 0;
    if (strcmp(state->outputFormat, "png") == 0) {
      saveSuccess = stbi_write_png(outputPath, finalWidth, finalHeight, chRgba,
                                   image, finalWidth * chRgba);
    } else if (strcmp(state->outputFormat, "jpg") == 0 ||
               strcmp(state->outputFormat, "jpeg") == 0) {
      saveSuccess =
          stbi_write_jpg(outputPath, finalWidth, finalHeight, 4, image, 90);
    } else if (strcmp(state->outputFormat, "bmp") == 0) {
      saveSuccess =
          stbi_write_bmp(outputPath, finalWidth, finalHeight, chRgba, image);
    } else {
      fprintf(stderr, "ERROR: Unsupported output format: %s\n",
              state->outputFormat);
    }
    if (saveSuccess) {
      printf("Saved packed data successfully: %s (%dx%d)\n", outputPath,
             finalWidth, finalHeight);
    } else {
      fprintf(stderr, "ERROR: Failed to write packed data to %s\n", outputPath);
    }
    platform_free(image);
  }
}

void application_save_atlas_map(State *state, const char *imageFilename,
                                const char *mapPath) {
  FILE *f;
  int i;
  int finalWidth, finalHeight;

  f = fopen(mapPath, "w");
  finalWidth = state->box.width;
  finalHeight = state->box.height;

  if (!f) {
    fprintf(stderr, "ERROR: Failed to open map file: %s\n", mapPath);
    return;
  }

  fprintf(f, "atlas_filename: %s\n", imageFilename);
  fprintf(f, "atlas_width: %d\n", finalWidth);
  fprintf(f, "atlas_height: %d\n", finalHeight);
  fprintf(f, "sprites:\n");

  for (i = 0; i < state->count; i++) {
    Rect *rect = &state->rects[i];
    if (!rect->placed) {
      continue;
    }
    {
      int offX, offY, tW, tH;
      int fX, fY, fWR, fHR;

      offX = rect->trimX;
      offY = rect->trimY;
      tW = rect->trimmedWidth;
      tH = rect->trimmedHeight;
      fX = rect->x;
      fY = rect->y;
      fWR = rect->width;
      fHR = rect->height;

      fprintf(f, "  %s:\n", rect->path);
      fprintf(f, "    frame:\n");
      fprintf(f, "      x: %d\n", fX);
      fprintf(f, "      y: %d\n", fY);
      fprintf(f, "      w: %d\n", fWR);
      fprintf(f, "      h: %d\n", fHR);
      fprintf(f, "    source_size:\n");
      fprintf(f, "      w: %d\n", rect->originalWidth);
      fprintf(f, "      h: %d\n", rect->originalHeight);
      fprintf(f, "    sprite_source_size:\n");
      fprintf(f, "      x: %d\n", offX);
      fprintf(f, "      y: %d\n", offY);
      fprintf(f, "      w: %d\n", tW);
      fprintf(f, "      h: %d\n", tH);
      fprintf(f, "    rotated: %s\n", rect->rotated ? "true" : "false");
      fprintf(f, "    trimmed: %s\n",
              (rect->trimmedWidth != rect->originalWidth ||
               rect->trimmedHeight != rect->originalHeight)
                  ? "true"
                  : "false");
    }
  }
  fclose(f);
}

int application_load_images(const char *dir, State *state) {
  PlatformDirHandle *dirHandle;
  char path[MAX_PATH_CUSTOM];
  char filename[MAX_PATH_CUSTOM];
  int fileCount;
  int processed;
  int width, height, channels;
  unsigned char *data;

  state->count = 0;
  fileCount = 0;
  processed = 0;

  state->rects = (Rect *)platform_malloc(MAX_RECTS * sizeof(Rect));
  if (!state->rects) {
    return 0;
  }

  dirHandle = platform_dir_open(dir);
  if (!dirHandle) {
    fprintf(stderr, "Cannot open directory: %s\n", dir);
    return 0;
  }

  while (platform_dir_read(dirHandle, filename, MAX_PATH_CUSTOM)) {
    fileCount++;
  }
  platform_dir_close(dirHandle);

  dirHandle = platform_dir_open(dir);
  if (!dirHandle) {
    fprintf(stderr, "Cannot re-open directory: %s\n", dir);
    platform_free(state->rects);
    return 0;
  }

  printf("Scanning directory '%s'...\n", dir);

  while (platform_dir_read(dirHandle, filename, MAX_PATH_CUSTOM)) {
    if (filename[0] == '.') {
      continue;
    }
    if (!utility_check_extension(filename, state->allowedExtsPositive,
                                 state->allowedExtsNegative)) {
      continue;
    }

    platform_path_join(dir, filename, path, MAX_PATH_CUSTOM);
    data = stbi_load(path, &width, &height, &channels, 4);
    processed++;

    printf("\rLoading Images: %d/%d (%.1f%%)", processed, fileCount,
           (float)processed * 100.0f / fileCount);
    fflush(stdout);

    if (!data) {
      continue;
    }
    if (state->count >= MAX_RECTS) {
      fprintf(stderr, "\nToo many images (Max: %d)\n", MAX_RECTS);
      stbi_image_free(data);
      break;
    }
    {
      Rect *rect = &state->rects[state->count];
      int useW, useH;

      rect->data = data;
      rect->channels = 4;
      rect->originalWidth = width;
      rect->originalHeight = height;
      rect->placed = 0;
      rect->rotated = 0;
      rect->x = -1;
      rect->y = -1;

      application_find_trim_box(rect);

      useW = rect->trimmedWidth;
      useH = rect->trimmedHeight;
      useW = (int)(ceil((double)useW * state->scaleIn));
      useH = (int)(ceil((double)useH * state->scaleIn));

      rect->width = useW;
      rect->height = useH;
      rect->area = useW * useH;
      rect->currentWidth = useW;
      rect->currentHeight = useH;

      strncpy(rect->path, filename, MAX_PATH_CUSTOM - 1);
      rect->path[MAX_PATH_CUSTOM - 1] = '\0';
      state->count++;
    }
  }

  printf("\n");
  platform_dir_close(dirHandle);
  return state->count > 0;
}

State state;

int main(int argc, char **argv) {
  int parse_result;
  char finalOutputPath[MAX_PATH_CUSTOM];
  const char *ext;
  char baseFilenameBuffer[MAX_FILENAME];
  char *finalName;
  int packSuccess;
  int i;

  packSuccess = 0;
  memset(&state, 0, sizeof(State));

  strncpy(state.inputDir, ".", MAX_FILENAME);
  strcpy(state.outputBaseFilename, "packed");
  state.scaleIn = 1.0f;
  strcpy(state.outputFormat, "png");
  strcpy(state.mapFilename, "");
  state.bgColor[0] = state.bgColor[1] = state.bgColor[2] = state.bgColor[3] = 0;
  state.allowRotation = 0;
  state.weightArea = 1.0f;
  state.weightShort = 0.0f;
  state.weightLong = 0.0f;

  parse_result = argparse_process_args(CAST_STATE(&state), argv, argc,
                                       GLOBAL_OPTION_TABLE);

  if (parse_result != 0) { return -1; }

  if (!application_load_images(state.inputDir, &state)) {
    fprintf(stderr, "No images loaded or directory not found.\n");
    return 1;
  }
  printf("Loaded %d images.\n", state.count);

  packSuccess = application_pack_rects(&state);

  if (packSuccess) {
    ext = state.outputFormat;
    strncpy(baseFilenameBuffer, state.outputBaseFilename, MAX_FILENAME);
    finalName = application_find_next_filename(baseFilenameBuffer, ext,
                                               finalOutputPath);
    application_save_packed_image(&state, finalName);
    if (state.mapFilename[0] != '\0') {
      application_save_atlas_map(&state, finalName, state.mapFilename);
      printf("Saved atlas metadata to: %s\n", state.mapFilename);
    }
  } else {
    printf("Failed to pack rectangles.\n");
    return 1;
  }

  for (i = 0; i < state.count; i++) {
    stbi_image_free(state.rects[i].data);
  }
  platform_free(state.rects);

  if (GLOBAL_FREE_RECTS) {
    platform_free(GLOBAL_FREE_RECTS);
  }
  return 0;
}
