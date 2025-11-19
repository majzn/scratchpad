#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <math.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define MAX_RECTS 1024
#define MAX_PATH 512
#define MAX_EXT_LEN 256
#define MAX_FILENAME 256
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
typedef struct {
  int w, h, x, y, area;
  char path[MAX_PATH];
  unsigned char *data;
  unsigned char *rotated_data;
  int channels;
  int placed;
  int original_w, original_h;
  int current_w, current_h;
  int rotated;
  int trim_x, trim_y;
  int trimmed_w, trimmed_h;
} Rect;
typedef struct {
  int w, h, area;
} Box;
typedef struct {
  int x, y, w, h;
} FreeRect;
typedef struct {
  Rect *rects;
  int count;
  Box box;
  int debug;
  int aspect_w, aspect_h;
  float scale_in;
  float scale_out;
  int allow_rotation;
  float final_rotation_angle;
  unsigned char bg_color[4];
  unsigned char fill_color[4];
  char output_format[10];
  char lock_axis[10];
  char allowed_exts_positive[MAX_EXT_LEN];
  char allowed_exts_negative[MAX_EXT_LEN];
  int bits_per_channel;
  unsigned char mono_tint[4];
  int output_channels;
  int output_raw;
  char output_base_filename[MAX_FILENAME];
  int grid_mode;
  int cell_w, cell_h;
  int grid_cols, grid_rows;
  int padding;
  char map_filename[MAX_FILENAME];
} State;
FreeRect *free_rects;
int free_count, free_cap;
int cmp_area_desc(const void *a, const void *b) {
  return ((Rect *)b)->area - ((Rect *)a)->area;
}
int cmp_width_desc(const void *a, const void *b) {
  return ((Rect *)b)->w - ((Rect *)a)->w;
}
int cmp_box(const void *a, const void *b) {
  return ((Box *)a)->area - ((Box *)b)->area;
}
int cmp_square(const void *a, const void *b) {
  Box *box_a = (Box *)a;
  Box *box_b = (Box *)b;
  double ratio_a, ratio_b, dev_a, dev_b;
  if (box_a->area != box_b->area) { return box_a->area - box_b->area; }
  ratio_a = (double)box_a->w / box_a->h;
  ratio_b = (double)box_b->w / box_b->h;
  dev_a = fabs(ratio_a - 1.0);
  dev_b = fabs(ratio_b - 1.0);
  if (dev_a < dev_b) { return -1; }
  if (dev_a > dev_b) { return 1; }
  return 0;
}
unsigned char *
rotate_rect_data(const unsigned char *src_data, int w, int h, int channels) {
  int new_w = h;
  int new_h = w;
  long new_size = (long)new_w * new_h * channels;
  unsigned char *new_data = (unsigned char *)malloc((size_t)new_size);
  int y, x, k;
  if (!new_data) { return (unsigned char *)NULL; }
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      long src_idx = (long)(y * w + x) * channels;
      int dst_x = h - 1 - y;
      int dst_y = x;
      long dst_idx = (long)(dst_y * new_w + dst_x) * channels;
      for (k = 0; k < channels; k++) {
        new_data[dst_idx + k] = src_data[src_idx + k];
      }
    }
  }
  return new_data;
}
void sample_bilinear(
    const unsigned char *src_data, int w, int h, int channels, float x,
    float y, unsigned char *out_pixel) {
  int x1, y1, x2, y2, k;
  float dx, dy, r1, r2, final_val;
  unsigned char p11, p21, p12, p22;
  if (x < 0.0f || x >= (float)w - 1.0f || y < 0.0f || y >= (float)h - 1.0f) {
    for (k = 0; k < channels; k++) { out_pixel[k] = 0; }
    if (channels < 4) { out_pixel[3] = 0; }
    return;
  }
  x1 = (int)x;
  y1 = (int)y;
  x2 = x1 + 1;
  y2 = y1 + 1;
  dx = x - (float)x1;
  dy = y - (float)y1;
  for (k = 0; k < channels; k++) {
    p11 = src_data[(long)(y1 * w + x1) * channels + k];
    p21 = src_data[(long)(y1 * w + x2) * channels + k];
    p12 = src_data[(long)(y2 * w + x1) * channels + k];
    p22 = src_data[(long)(y2 * w + x2) * channels + k];
    r1 = (float)p11 * (1.0f - dx) + (float)p21 * dx;
    r2 = (float)p12 * (1.0f - dx) + (float)p22 * dx;
    final_val = r1 * (1.0f - dy) + r2 * dy;
    out_pixel[k] = (unsigned char)final_val;
  }
}
void rotate_bitmap_trig(
    const unsigned char *src_data, int src_w, int src_h, int channels,
    unsigned char *dst_data, int dst_w, int dst_h, float theta_deg) {
  float theta, cos_theta, sin_theta, cx, cy, dx_c, dy_c, x_rel, y_rel,
      src_x_rel, src_y_rel, src_x, src_y;
  int dy, dx, k;
  unsigned char temp_pixel[4] = {0, 0, 0, 0};
  theta = theta_deg * M_PI / 180.0f;
  cos_theta = cosf(theta);
  sin_theta = sinf(theta);
  cx = (float)src_w / 2.0f;
  cy = (float)src_h / 2.0f;
  dx_c = (float)dst_w / 2.0f;
  dy_c = (float)dst_h / 2.0f;
  for (dy = 0; dy < dst_h; dy++) {
    for (dx = 0; dx < dst_w; dx++) {
      x_rel = (float)dx - dx_c;
      y_rel = (float)dy - dy_c;
      src_x_rel = x_rel * cos_theta + y_rel * sin_theta;
      src_y_rel = x_rel * (-sin_theta) + y_rel * cos_theta;
      src_x = src_x_rel + cx;
      src_y = src_y_rel + cy;
      sample_bilinear(
          src_data, src_w, src_h, channels, src_x, src_y, temp_pixel);
      long dst_idx = (long)(dy * dst_w + dx) * channels;
      for (k = 0; k < channels; k++) { dst_data[dst_idx + k] = temp_pixel[k]; }
    }
  }
}
char *
find_next_filename(const char *base_filename, const char *ext, char *buffer) {
  char test_filename[MAX_FILENAME];
  int version = 0;
  struct stat buffer_stat;
  size_t base_len, i;
  char base_no_num[MAX_FILENAME];
  int num_digits = 0;
  sprintf(test_filename, "%s.%s", base_filename, ext);
  if (stat(test_filename, &buffer_stat) != 0) {
    strcpy(buffer, test_filename);
    return buffer;
  }
  base_len = strlen(base_filename);
  for (i = base_len; i > 0; i--) {
    if (base_filename[i - 1] >= '0' && base_filename[i - 1] <= '9') {
      num_digits++;
    } else {
      break;
    }
  }
  if (num_digits > 0) {
    strncpy(base_no_num, base_filename, base_len - num_digits);
    base_no_num[base_len - num_digits] = '\0';
    version = atoi(base_filename + (base_len - num_digits));
    version++;
  } else {
    strncpy(base_no_num, base_filename, MAX_FILENAME);
    base_no_num[MAX_FILENAME - 1] = '\0';
    version = 1;
  }
  while (1) {
    sprintf(buffer, "%s%d.%s", base_no_num, version, ext);
    if (stat(buffer, &buffer_stat) != 0) { return buffer; }
    version++;
  }
}
void find_trim_box(Rect *r) {
  int w = r->original_w;
  int h = r->original_h;
  unsigned char *data = r->data;
  int ch = r->channels;
  int x, y;
  int min_x = w, max_x = 0;
  int min_y = h, max_y = 0;
  int found_pixel = 0;
  if (ch < 4) {
    r->trim_x = 0;
    r->trim_y = 0;
    r->trimmed_w = w;
    r->trimmed_h = h;
    return;
  }
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      if (data[(long)(y * w + x) * ch + 3] > 0) {
        found_pixel = 1;
        if (x < min_x) { min_x = x; }
        if (x > max_x) { max_x = x; }
        if (y < min_y) { min_y = y; }
        if (y > max_y) { max_y = y; }
      }
    }
  }
  if (!found_pixel) {
    r->trim_x = 0;
    r->trim_y = 0;
    r->trimmed_w = 0;
    r->trimmed_h = 0;
  } else {
    r->trim_x = min_x;
    r->trim_y = min_y;
    r->trimmed_w = max_x - min_x + 1;
    r->trimmed_h = max_y - min_y + 1;
  }
}
void remove_free_rect(int index) {
  if (index < free_count - 1) {
    free_rects[index] = free_rects[free_count - 1];
  }
  free_count--;
}
int find_best_fit(State *s, Rect *r, int *best_idx, int *best_x, int *best_y) {
  int min_waste = 2000000000;
  int current_w, current_h;
  int i, rot, allow_rot;
  allow_rot = s->allow_rotation;
  *best_idx = -1;
  for (i = 0; i < free_count; i++) {
    for (rot = 0; rot < (allow_rot ? 2 : 1); rot++) {
      current_w = (rot == 1) ? r->current_h : r->current_w;
      current_h = (rot == 1) ? r->current_w : r->current_h;
      if (current_w <= free_rects[i].w && current_h <= free_rects[i].h) {
        int leftover_w = free_rects[i].w - current_w;
        int leftover_h = free_rects[i].h - current_h;
        int current_waste = leftover_w < leftover_h ? leftover_w : leftover_h;
        if (current_waste < min_waste) {
          min_waste = current_waste;
          *best_idx = i;
          *best_x = free_rects[i].x;
          *best_y = free_rects[i].y;
          r->rotated = rot;
        }
      }
    }
  }
  return (*best_idx != -1);
}
void split_free_rect(int index, Rect *r) {
  FreeRect old, r1, r2;
  int new_w, new_h, j, k;
  old = free_rects[index];
  new_w = r->w;
  new_h = r->h;
  remove_free_rect(index);
  r1.w = 0;
  r2.w = 0;
  if (old.w > new_w && old.h > new_h) {
    if (old.w - new_w > old.h - new_h) {
      r1.x = old.x + new_w;
      r1.y = old.y;
      r1.w = old.w - new_w;
      r1.h = old.h;
      r2.x = old.x;
      r2.y = old.y + new_h;
      r2.w = new_w;
      r2.h = old.h - new_h;
    } else {
      r1.x = old.x;
      r1.y = old.y + new_h;
      r1.w = old.w;
      r1.h = old.h - new_h;
      r2.x = old.x + new_w;
      r2.y = old.y;
      r2.w = old.w - new_w;
      r2.h = new_h;
    }
  } else if (old.w > new_w) {
    r1.x = old.x + new_w;
    r1.y = old.y;
    r1.w = old.w - new_w;
    r1.h = old.h;
  } else if (old.h > new_h) {
    r1.x = old.x;
    r1.y = old.y + new_h;
    r1.w = old.w;
    r1.h = old.h - new_h;
  } else {
    return;
  }
  if (r1.w > 0 && r1.h > 0) {
    if (free_count >= free_cap) {
      free_cap = free_cap ? free_cap * 2 : 32;
      free_rects =
          (FreeRect *)realloc(free_rects, free_cap * sizeof(FreeRect));
    }
    free_rects[free_count++] = r1;
  }
  if (r2.w > 0 && r2.h > 0) {
    if (free_count >= free_cap) {
      free_cap = free_cap ? free_cap * 2 : 32;
      free_rects =
          (FreeRect *)realloc(free_rects, free_cap * sizeof(FreeRect));
    }
    free_rects[free_count++] = r2;
  }
  for (j = 0; j < free_count; j++) {
    for (k = j + 1; k < free_count; k++) {
      FreeRect fr1 = free_rects[j];
      FreeRect fr2 = free_rects[k];
      if (fr1.x >= fr2.x && fr1.y >= fr2.y && fr1.x + fr1.w <= fr2.x + fr2.w &&
          fr1.y + fr1.h <= fr2.y + fr2.h) {
        remove_free_rect(j);
        j--;
        break;
      }
      if (fr2.x >= fr1.x && fr2.y >= fr1.y && fr2.x + fr2.w <= fr1.x + fr1.w &&
          fr2.y + fr2.h <= fr1.y + fr1.h) {
        remove_free_rect(k);
        k--;
      }
    }
  }
}
int try_box_maxrects(State *s, Box box) {
  int i, best_idx, best_x, best_y;
  Rect *r;
  s->box = box;
  free_count = 1;
  free_cap = 1;
  free_rects = (FreeRect *)realloc(free_rects, free_cap * sizeof(FreeRect));
  free_rects[0].x = 0;
  free_rects[0].y = 0;
  free_rects[0].w = box.w;
  free_rects[0].h = box.h;
  for (i = 0; i < s->count; i++) {
    r = &s->rects[i];
    r->placed = 0;
    r->x = -1;
    r->y = -1;
    r->rotated = 0;
    r->w = r->current_w;
    r->h = r->current_h;
    if (r->current_w > box.w && r->current_h > box.w) { return 0; }
    if (r->current_h > box.h && r->current_w > box.h) { return 0; }
  }
  for (i = 0; i < s->count; i++) {
    r = &s->rects[i];
    if (find_best_fit(s, r, &best_idx, &best_x, &best_y)) {
      r->x = best_x;
      r->y = best_y;
      r->placed = 1;
      if (r->rotated) {
        r->w = r->current_h;
        r->h = r->current_w;
      } else {
        r->w = r->current_w;
        r->h = r->current_h;
      }
      split_free_rect(best_idx, r);
    } else {
      return 0;
    }
  }
  return 1;
}
int pack_rects_grid(State *s) {
  int i;
  int current_col = 0;
  int current_row = 0;
  int rect_idx = 0;
  int total_cells = s->grid_rows * s->grid_cols;
  int v_cell_w = 0;
  int v_cell_h = 0;
  int final_pack_w, final_pack_h;
  int dynamic_cell = (s->cell_w == 0);
  if (s->grid_cols <= 0 || s->grid_rows <= 0) { return 0; }
  if (s->cell_w == -1) {
    if (s->count == 0) { return 0; }
    v_cell_w = s->rects[0].current_w;
    v_cell_h = s->rects[0].current_h;
    s->cell_w = v_cell_w;
    s->cell_h = v_cell_h;
    printf(
        "Grid Mode: Auto-setting cell size to first image's dimensions "
        "(%dx%d).\n",
        v_cell_w, v_cell_h);
  } else if (dynamic_cell) {
    for (i = 0; i < s->count; i++) {
      if (s->rects[i].current_w > v_cell_w) {
        v_cell_w = s->rects[i].current_w;
      }
      if (s->rects[i].current_h > v_cell_h) {
        v_cell_h = s->rects[i].current_h;
      }
    }
    if (v_cell_w == 0 || v_cell_h == 0) { return 0; }
    printf(
        "Grid Mode: Dynamic cell sizing (-cell 0). Virtual cell spacing "
        "determined by max image size (%dx%d).\n",
        v_cell_w, v_cell_h);
  } else {
    v_cell_w = s->cell_w;
    v_cell_h = s->cell_h;
  }
  s->box.w = s->grid_cols * v_cell_w + (s->grid_cols + 1) * s->padding;
  s->box.h = s->grid_rows * v_cell_h + (s->grid_rows + 1) * s->padding;
  s->box.area = s->box.w * s->box.h;
  for (i = 0; i < s->count; i++) { s->rects[i].placed = 0; }
  for (i = 0; i < s->count && rect_idx < total_cells; i++) {
    Rect *r = &s->rects[i];
    current_col = rect_idx % s->grid_cols;
    current_row = rect_idx / s->grid_cols;
    r->x = current_col * v_cell_w + (current_col + 1) * s->padding;
    r->y = current_row * v_cell_h + (current_row + 1) * s->padding;
    if (dynamic_cell) {
      final_pack_w = r->current_w;
      final_pack_h = r->current_h;
      r->x += (v_cell_w - final_pack_w) / 2;
      r->y += (v_cell_h - final_pack_h) / 2;
    } else {
      final_pack_w = v_cell_w;
      final_pack_h = v_cell_h;
    }
    r->w = final_pack_w;
    r->h = final_pack_h;
    r->placed = 1;
    r->rotated = 0;
    rect_idx++;
  }
  s->count = i;
  return 1;
}
int pack_rects(State *s) {
  int min_w, min_h, w, h;
  int total_area = 0;
  Box *boxes;
  int box_count = 0, box_cap = 1024;
  int tried = 0;
  int greedy_w;
  int i;
  int low, high, mid;
  int best_box_idx = -1;
  for (i = 0; i < s->count; i++) { total_area += s->rects[i].area; }
  min_w = 0;
  min_h = 0;
  for (i = 0; i < s->count; i++) {
    int max_dim = (s->rects[i].current_w > s->rects[i].current_h)
                      ? s->rects[i].current_w
                      : s->rects[i].current_h;
    if (s->allow_rotation) {
      if (max_dim > min_w) { min_w = max_dim; }
      if (max_dim > min_h) { min_h = max_dim; }
    } else {
      if (s->rects[i].current_w > min_w) { min_w = s->rects[i].current_w; }
      if (s->rects[i].current_h > min_h) { min_h = s->rects[i].current_h; }
    }
  }
  greedy_w = 0;
  for (i = 0; i < s->count; i++) { greedy_w += s->rects[i].current_w; }
  boxes = (Box *)malloc(box_cap * sizeof(Box));
  if (s->lock_axis[0] == 'W') {
    w = greedy_w;
    h = min_h;
    if (box_count >= box_cap) {
      box_cap *= 2;
      boxes = realloc(boxes, box_cap * sizeof(Box));
    }
    boxes[box_count].w = w;
    boxes[box_count].h = h;
    boxes[box_count].area = w * h;
    box_count++;
    printf("Axis Lock Mode: Width fixed to total greedy width (%d).\n", w);
  } else if (s->lock_axis[0] == 'H') {
    printf(
        "Axis Lock Mode: Height fixed to minimum possible height (%d) at "
        "varying widths.\n",
        min_h);
    for (w = min_w; w <= greedy_w; w++) {
      h = (total_area + w - 1) / w;
      if (h < min_h) { h = min_h; }
      if (box_count >= box_cap) {
        box_cap *= 2;
        boxes = realloc(boxes, box_cap * sizeof(Box));
      }
      boxes[box_count].w = w;
      boxes[box_count].h = h;
      boxes[box_count].area = w * h;
      box_count++;
      if (box_count >= 5000) { break; }
    }
  } else if (s->aspect_w > 0 && s->aspect_h > 0) {
    double aspect = (double)s->aspect_w / s->aspect_h;
    int step = s->aspect_w;
    printf("Aspect Ratio Mode: %d:%d\n", s->aspect_w, s->aspect_h);
    for (w = min_w; w <= greedy_w; w += step) {
      h = (int)ceil((double)w / aspect);
      if (h < min_h) {
        h = min_h;
        w = (int)ceil((double)h * aspect);
      }
      if (w < min_w) {
        w = min_w;
        h = (int)ceil((double)w / aspect);
      }
      if (w * h < total_area) { continue; }
      if (box_count >= box_cap) {
        box_cap *= 2;
        boxes = realloc(boxes, box_cap * sizeof(Box));
      }
      boxes[box_count].w = w;
      boxes[box_count].h = h;
      boxes[box_count].area = w * h;
      box_count++;
      if (box_count >= 5000) { break; }
    }
  } else if (s->aspect_w == -1) {
    int largest_dim = (min_w > min_h) ? min_w : min_h;
    int max_dim = (int)ceil(sqrt((double)total_area));
    int dim;
    printf("Closest to Square Mode (1:1 aspect).\n");
    for (dim = max_dim; dim <= greedy_w; dim++) {
      int h1 = (total_area + dim - 1) / dim;
      int w2 = (total_area + dim - 1) / dim;
      int h2 = dim;
      int w1 = dim;
      if (w1 >= min_w && h1 >= min_h && w1 >= largest_dim &&
          h1 >= largest_dim) {
        if (box_count >= box_cap) {
          box_cap *= 2;
          boxes = realloc(boxes, box_cap * sizeof(Box));
        }
        boxes[box_count].w = w1;
        boxes[box_count].h = h1;
        boxes[box_count].area = w1 * h1;
        box_count++;
      }
      if (w2 >= min_w && h2 >= min_h && w2 >= largest_dim &&
          h2 >= largest_dim) {
        if (box_count >= box_cap) {
          box_cap *= 2;
          boxes = realloc(boxes, box_cap * sizeof(Box));
        }
        boxes[box_count].w = w2;
        boxes[box_count].h = h2;
        boxes[box_count].area = w2 * h2;
        box_count++;
      }
      if (box_count >= 5000) { break; }
    }
  } else {
    for (w = min_w; w <= greedy_w; w++) {
      h = (total_area + w - 1) / w;
      if (h < min_h) { h = min_h; }
      if (box_count >= box_cap) {
        box_cap *= 2;
        boxes = realloc(boxes, box_cap * sizeof(Box));
      }
      boxes[box_count].w = w;
      boxes[box_count].h = h;
      boxes[box_count].area = w * h;
      box_count++;
      if (box_count >= 5000) { break; }
    }
  }
  printf("Generated %d candidate boxes.\n", box_count);
  qsort(boxes, box_count, sizeof(Box), cmp_box);
  if (s->aspect_w == -1 || s->lock_axis[0] == 'H') {
    qsort(boxes, box_count, sizeof(Box), cmp_square);
  }
  low = 0;
  high = box_count - 1;
  while (low <= high) {
    mid = low + (high - low) / 2;
    tried++;
    printf(
        "\rSearching Boxes: Attempt %d (Checking %dx%d)...", tried,
        boxes[mid].w, boxes[mid].h);
    fflush(stdout);
    if (try_box_maxrects(s, boxes[mid])) {
      best_box_idx = mid;
      high = mid - 1;
    } else {
      low = mid + 1;
    }
  }
  printf("\n");
  if (best_box_idx != -1) {
    printf("\nSUCCESS! Found solution after %d attempts.\n", tried);
    try_box_maxrects(s, boxes[best_box_idx]);
    free(boxes);
    return 1;
  }
  free(boxes);
  return 0;
}
void quantize_image(
    unsigned char *data, int w, int h, int channels, int bits_per_channel,
    unsigned char *mono_tint) {
  long pixel_count = (long)w * h;
  long i;
  int k;
  if (bits_per_channel >= 8) { return; }
  if (bits_per_channel == 1) {
    float threshold = 127.5f;
    for (i = 0; i < pixel_count; i++) {
      long idx = i * channels;
      float luminance =
          (float)(data[idx] + data[idx + 1] + data[idx + 2]) / 3.0f;
      int bit = (luminance >= threshold);
      if (bit) {
        data[idx + 0] = mono_tint[0];
        data[idx + 1] = mono_tint[1];
        data[idx + 2] = mono_tint[2];
        if (channels == 4) { data[idx + 3] = mono_tint[3]; }
      } else {
        data[idx + 0] = 0;
        data[idx + 1] = 0;
        data[idx + 2] = 0;
        if (channels == 4) { data[idx + 3] = 0; }
      }
    }
    return;
  }
  {
    int max_val = (1 << bits_per_channel) - 1;
    float scale = (float)max_val / 255.0f;
    float rescale = 255.0f / max_val;
    for (i = 0; i < pixel_count; i++) {
      for (k = 0; k < channels; k++) {
        long idx = i * channels + k;
        int quantized = (int)roundf(data[idx] * scale);
        data[idx] = (unsigned char)roundf((float)quantized * rescale);
      }
    }
  }
}
void save_packed_image(State *s, const char *output_full_path) {
  int final_w = (int)ceil((float)s->box.w * s->scale_out);
  int final_h = (int)ceil((float)s->box.h * s->scale_out);
  int channels_rgba = 4;
  int actual_channels_out = s->output_channels;
  long img_size = (long)final_w * final_h * channels_rgba;
  unsigned char *img = (unsigned char *)calloc((size_t)img_size, 1);
  unsigned char *base_image;
  int i;
  printf(
      "Writing packed data (%dx%d -> %dx%d, Scale Out: %.2fx, Final Rotation: "
      "%.1f deg) to %s (BPC: %d, Channels: %d, Raw: %s)...\n",
      s->box.w, s->box.h, final_w, final_h, s->scale_out,
      s->final_rotation_angle, output_full_path, s->bits_per_channel,
      actual_channels_out, s->output_raw ? "Yes" : "No");
  base_image = (unsigned char *)calloc((size_t)img_size, 1);
  if (!base_image) {
    free(img);
    return;
  }
  {
    int y, x;
    for (y = 0; y < final_h; y++) {
      for (x = 0; x < final_w; x++) {
        long idx = (long)(y * final_w + x) * channels_rgba;
        memcpy(&base_image[idx], s->bg_color, 4);
      }
    }
  }
  for (i = 0; i < s->count; i++) {
    Rect *r = &s->rects[i];
    if (r->x < 0 || r->y < 0) { continue; }
    {
      const unsigned char *current_data =
          r->rotated ? r->rotated_data : r->data;
      int current_data_w = r->rotated ? r->original_h : r->original_w;
      int current_data_h = r->rotated ? r->original_w : r->original_h;
      int start_dst_x = (int)floor((float)r->x * s->scale_out);
      int end_dst_x = (int)ceil(((float)r->x + (float)r->w) * s->scale_out);
      int start_dst_y = (int)floor((float)r->y * s->scale_out);
      int end_dst_y = (int)ceil(((float)r->y + (float)r->h) * s->scale_out);
      int content_w = r->trimmed_w;
      int content_h = r->trimmed_h;
      int content_trim_x = r->trim_x;
      int content_trim_y = r->trim_y;
      if (r->rotated) {
        if (current_data == (unsigned char *)NULL) { continue; }
        content_w = r->trimmed_h;
        content_h = r->trimmed_w;
      }
      {
        int dst_y, dst_x, k;
        for (dst_y = start_dst_y; dst_y < end_dst_y && dst_y < final_h;
             dst_y++) {
          for (dst_x = start_dst_x; dst_x < end_dst_x && dst_x < final_w;
               dst_x++) {
            float canvas_x = (float)dst_x / s->scale_out;
            float canvas_y = (float)dst_y / s->scale_out;
            float relative_x = canvas_x - (float)r->x;
            float relative_y = canvas_y - (float)r->y;
            float final_src_x_unrotated;
            float final_src_y_unrotated;
            float scaled_content_x =
                relative_x * ((float)content_w / (float)r->w);
            float scaled_content_y =
                relative_y * ((float)content_h / (float)r->h);
            if (r->rotated) {
              final_src_x_unrotated = scaled_content_x + (float)r->trim_y;
              final_src_y_unrotated =
                  scaled_content_y +
                  (float)(r->original_w - (r->trim_x + r->trimmed_w));
            } else {
              final_src_x_unrotated = scaled_content_x + (float)content_trim_x;
              final_src_y_unrotated = scaled_content_y + (float)content_trim_y;
            }
            {
              int src_x = (int)floor(final_src_x_unrotated);
              int src_y = (int)floor(final_src_y_unrotated);
              if (src_x >= 0 && src_x < current_data_w && src_y >= 0 &&
                  src_y < current_data_h) {
                long dst_idx = (long)(dst_y * final_w + dst_x) * channels_rgba;
                long src_idx =
                    (long)(src_y * current_data_w + src_x) * r->channels;
                unsigned char img_r = current_data[src_idx + 0];
                unsigned char img_g = current_data[src_idx + 1];
                unsigned char img_b = current_data[src_idx + 2];
                unsigned char img_a =
                    (r->channels == 4) ? current_data[src_idx + 3] : 255;
                unsigned char dest_r = s->fill_color[0];
                unsigned char dest_g = s->fill_color[1];
                unsigned char dest_b = s->fill_color[2];
                float alpha_ratio = (float)img_a / 255.0f;
                float inv_alpha_ratio = 1.0f - alpha_ratio;
                base_image[dst_idx + 0] =
                    (unsigned char)(img_r * alpha_ratio +
                                    dest_r * inv_alpha_ratio);
                base_image[dst_idx + 1] =
                    (unsigned char)(img_g * alpha_ratio +
                                    dest_g * inv_alpha_ratio);
                base_image[dst_idx + 2] =
                    (unsigned char)(img_b * alpha_ratio +
                                    dest_b * inv_alpha_ratio);
                base_image[dst_idx + 3] =
                    (unsigned char)(img_a > 0 ? 255 : s->fill_color[3]);
              }
            }
          }
        }
      }
    }
  }
  if (s->final_rotation_angle != 0.0f) {
    float theta = s->final_rotation_angle * M_PI / 180.0f;
    float cos_t = fabs(cosf(theta));
    float sin_t = fabs(sinf(theta));
    int new_final_w =
        (int)ceil((double)final_w * cos_t + (double)final_h * sin_t);
    int new_final_h =
        (int)ceil((double)final_w * sin_t + (double)final_h * cos_t);
    free(img);
    img_size = (long)new_final_w * new_final_h * channels_rgba;
    img = (unsigned char *)calloc((size_t)img_size, 1);
    printf(
        "Applying %.1f degree rotation. New output size: %dx%d\n",
        s->final_rotation_angle, new_final_w, new_final_h);
    rotate_bitmap_trig(
        base_image, final_w, final_h, channels_rgba, img, new_final_w,
        new_final_h, -s->final_rotation_angle);
    final_w = new_final_w;
    final_h = new_final_h;
  } else {
    memcpy(img, base_image, (size_t)img_size);
  }
  free(base_image);
  if (s->bits_per_channel != 8 && s->bits_per_channel != 16) {
    quantize_image(
        img, final_w, final_h, channels_rgba, s->bits_per_channel,
        s->mono_tint);
  }
  {
    unsigned char *output_data = (unsigned char *)NULL;
    long output_data_size = 0;
    int save_success = 0;
    if (actual_channels_out != channels_rgba) {
      output_data = (unsigned char *)malloc(
          (long)final_w * final_h * actual_channels_out);
      if (output_data == (unsigned char *)NULL) {
        free(img);
        return;
      }
      output_data_size = (long)final_w * final_h * actual_channels_out;
      if (actual_channels_out == 1 || actual_channels_out == 2) {
        long i;
        for (i = 0; i < (long)final_w * final_h; i++) {
          unsigned char gray = (unsigned char)roundf(
              img[i * 4 + 0] * 0.2126f + img[i * 4 + 1] * 0.7152f +
              img[i * 4 + 2] * 0.0722f);
          output_data[i * actual_channels_out] = gray;
          if (actual_channels_out == 2) {
            output_data[i * actual_channels_out + 1] = img[i * 4 + 3];
          }
        }
      } else if (actual_channels_out == 3) {
        long i;
        for (i = 0; i < (long)final_w * final_h; i++) {
          memcpy(&output_data[i * actual_channels_out], &img[i * 4], 3);
        }
      }
    } else {
      output_data = img;
      output_data_size = img_size;
    }
    if (s->bits_per_channel == 16) {
      long final_16bit_size = output_data_size * 2;
      unsigned short *output_16bit =
          (unsigned short *)malloc((size_t)final_16bit_size);
      unsigned char *temp_data = output_data;
      if (output_16bit == (unsigned short *)NULL) {
        if (output_data != img) { free(output_data); }
        free(img);
        return;
      }
      {
        long i;
        for (i = 0; i < output_data_size; i++) {
          output_16bit[i] = (unsigned short)(temp_data[i] * 257);
        }
      }
      if (temp_data != img) { free(temp_data); }
      output_data = (unsigned char *)output_16bit;
      output_data_size = final_16bit_size;
      printf("NOTE: Data stored as 16-bit integers in memory.\n");
    }
    if (output_data != img) { free(img); }
    if (s->output_raw) {
      FILE *f = fopen(output_full_path, "wb");
      if (f) {
        if (fwrite(output_data, 1, (size_t)output_data_size, f) ==
            (size_t)output_data_size) {
          save_success = 1;
        }
        fclose(f);
      }
    } else if (strcmp(s->output_format, "png") == 0) {
      save_success = stbi_write_png(
          output_full_path, final_w, final_h, actual_channels_out, output_data,
          final_w * actual_channels_out * (s->bits_per_channel > 8 ? 2 : 1));
    } else if (
        strcmp(s->output_format, "jpg") == 0 ||
        strcmp(s->output_format, "jpeg") == 0) {
      save_success = stbi_write_jpg(
          output_full_path, final_w, final_h, 3, output_data, 90);
    } else if (strcmp(s->output_format, "bmp") == 0) {
      save_success = stbi_write_bmp(
          output_full_path, final_w, final_h, actual_channels_out,
          output_data);
    } else {
      fprintf(
          stderr, "ERROR: Unsupported output format: %s\n", s->output_format);
    }
    if (save_success) {
      printf(
          "Saved packed data successfully: %s (%dx%d, BPC: %d, Channels: %d, "
          "Size: %ld bytes)\n",
          output_full_path, final_w, final_h, s->bits_per_channel,
          actual_channels_out, output_data_size);
    } else {
      fprintf(
          stderr, "ERROR: Failed to write packed data to %s\n",
          output_full_path);
    }
    free(output_data);
  }
}
void save_atlas_map(
    State *s, const char *image_filename, const char *map_full_path) {
  FILE *f = fopen(map_full_path, "w");
  int i;
  int final_w = (int)ceil((float)s->box.w * s->scale_out);
  int final_h = (int)ceil((float)s->box.h * s->scale_out);
  if (!f) {
    fprintf(stderr, "ERROR: Failed to open map file: %s\n", map_full_path);
    return;
  }
  fprintf(f, "atlas_filename: %s\n", image_filename);
  fprintf(f, "atlas_width: %d\n", final_w);
  fprintf(f, "atlas_height: %d\n", final_h);
  fprintf(f, "sprites:\n");
  for (i = 0; i < s->count; i++) {
    Rect *r = &s->rects[i];
    if (!r->placed) { continue; }
    {
      int offset_x = r->trim_x;
      int offset_y = r->trim_y;
      int trimmed_w = r->trimmed_w;
      int trimmed_h = r->trimmed_h;
      if (r->rotated) {
        offset_x = r->trim_y;
        offset_y = r->original_w - (r->trim_x + r->trimmed_w);
        trimmed_w = r->trimmed_h;
        trimmed_h = r->trimmed_w;
      }
      fprintf(f, "  %s:\n", r->path);
      fprintf(f, "    frame:\n");
      fprintf(f, "      x: %d\n", (int)floor((float)r->x * s->scale_out));
      fprintf(f, "      y: %d\n", (int)floor((float)r->y * s->scale_out));
      fprintf(
          f, "      w: %d\n",
          (int)ceil(((float)r->x + (float)r->w) * s->scale_out) -
              (int)floor((float)r->x * s->scale_out));
      fprintf(
          f, "      h: %d\n",
          (int)ceil(((float)r->y + (float)r->h) * s->scale_out) -
              (int)floor((float)r->y * s->scale_out));
      fprintf(f, "    source_size:\n");
      fprintf(f, "      w: %d\n", r->original_w);
      fprintf(f, "      h: %d\n", r->original_h);
      fprintf(f, "    sprite_source_size:\n");
      fprintf(f, "      x: %d\n", offset_x);
      fprintf(f, "      y: %d\n", offset_y);
      fprintf(f, "      w: %d\n", trimmed_w);
      fprintf(f, "      h: %d\n", trimmed_h);
      fprintf(f, "    rotated: %s\n", r->rotated ? "true" : "false");
      fprintf(
          f, "    trimmed: %s\n",
          (r->trimmed_w != r->original_w || r->trimmed_h != r->original_h)
              ? "true"
              : "false");
    }
  }
  fclose(f);
}
int check_extension(
    const char *filename, const char *allowed_positive,
    const char *allowed_negative) {
  const char *dot = strrchr(filename, '.');
  char ext[10];
  char *ext_copy = (char *)NULL;
  char *token = (char *)NULL;
  int found = 0;
  if (!dot || dot == filename) { return (allowed_positive[0] == '\0'); }
  strncpy(ext, dot + 1, 9);
  ext[9] = '\0';
  if (allowed_negative[0] != '\0') {
    ext_copy = strdup(allowed_negative);
    token = strtok(ext_copy, " ");
    while (token != (char *)NULL) {
      if (strcasecmp(ext, token) == 0) {
        free(ext_copy);
        return 0;
      }
      token = strtok((char *)NULL, " ");
    }
    free(ext_copy);
  }
  if (allowed_positive[0] == '\0' || strcmp(allowed_positive, "*") == 0) {
    return 1;
  }
  ext_copy = strdup(allowed_positive);
  token = strtok(ext_copy, " ");
  while (token != (char *)NULL) {
    if (strcasecmp(ext, token) == 0) {
      found = 1;
      break;
    }
    token = strtok((char *)NULL, " ");
  }
  free(ext_copy);
  return found;
}
int load_images(const char *dir, State *s) {
  DIR *d;
  struct dirent *ent;
  char path[MAX_PATH];
  int file_count = 0;
  int processed = 0;
  int w, h, ch;
  unsigned char *data;
  s->count = 0;
  s->rects = (Rect *)malloc(MAX_RECTS * sizeof(Rect));
  d = opendir(dir);
  if (!d) {
    fprintf(stderr, "Cannot open directory: %s\n", dir);
    return 0;
  }
  while ((ent = readdir(d)) != (struct dirent *)NULL) {
    if (ent->d_name[0] != '.') { file_count++; }
  }
  rewinddir(d);
  printf(
      "Scanning directory '%s' (Scale In: %.2fx, Grid Mode: %s)...\n", dir,
      s->scale_in, s->grid_mode ? "Yes" : "No");
  while ((ent = readdir(d)) != (struct dirent *)NULL) {
    if (ent->d_name[0] == '.') { continue; }
    if (!check_extension(
            ent->d_name, s->allowed_exts_positive, s->allowed_exts_negative)) {
      continue;
    }
    snprintf(path, MAX_PATH, "%s/%s", dir, ent->d_name);
    data = stbi_load(path, &w, &h, &ch, 0);
    processed++;
    printf(
        "\rLoading Images: %d/%d (%.1f%%)", processed, file_count,
        (float)processed * 100.0f / file_count);
    fflush(stdout);
    if (!data) { continue; }
    if (s->count >= MAX_RECTS) {
      fprintf(stderr, "\nToo many images (Max: %d)\n", MAX_RECTS);
      stbi_image_free(data);
      break;
    }
    {
      Rect *r = &s->rects[s->count];
      int use_w, use_h;
      r->data = data;
      r->channels = ch;
      r->original_w = w;
      r->original_h = h;
      r->placed = 0;
      r->x = -1;
      r->y = -1;
      r->rotated = 0;
      r->rotated_data = (unsigned char *)NULL;
      find_trim_box(r);
      use_w = r->trimmed_w;
      use_h = r->trimmed_h;
      use_w = (int)ceil((float)use_w * s->scale_in);
      use_h = (int)ceil((float)use_h * s->scale_in);
      r->w = use_w;
      r->h = use_h;
      r->area = use_w * use_h;
      r->current_w = use_w;
      r->current_h = use_h;
      if (s->allow_rotation && r->current_w != r->current_h) {
        r->rotated_data = rotate_rect_data(data, w, h, ch);
      }
      strncpy(r->path, ent->d_name, MAX_PATH - 1);
      r->path[MAX_PATH - 1] = '\0';
      s->count++;
    }
  }
  printf("\n");
  closedir(d);
  return s->count > 0;
}
int parse_color(const char *str, unsigned char *color) {
  int r, g, b, a;
  if (sscanf(str, "%d,%d,%d,%d", &r, &g, &b, &a) != 4) { return 0; }
  color[0] = (unsigned char)r;
  color[1] = (unsigned char)g;
  color[2] = (unsigned char)b;
  color[3] = (unsigned char)a;
  return 1;
}
int main(int argc, char **argv) {
  State s;
  char *input_dir = ".";
  char output_base_name[MAX_FILENAME] = "packed";
  int sort_by_area = 1;
  int i;
  char final_output_path[MAX_PATH];
  const char *ext;
  char base_name_buffer[MAX_FILENAME];
  char *final_name;
  int pack_success = 0;
  memset(&s, 0, sizeof(State));
  s.scale_in = 1.0f;
  s.scale_out = 1.0f;
  s.cell_w = -1;
  s.cell_h = -1;
  s.bits_per_channel = 8;
  s.output_channels = 4;
  strcpy(s.output_format, "png");
  strcpy(s.map_filename, "");
  s.bg_color[0] = s.bg_color[1] = s.bg_color[2] = s.bg_color[3] = 0;
  s.fill_color[0] = s.fill_color[1] = s.fill_color[2] = s.fill_color[3] = 0;
  s.mono_tint[0] = s.mono_tint[1] = s.mono_tint[2] = 255;
  s.mono_tint[3] = 255;
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
      s.debug = 1;
    } else if (
        strcmp(argv[i], "-raw") == 0 || strcmp(argv[i], "--raw-output") == 0) {
      s.output_raw = 1;
    } else if (
        strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
      if (i + 1 < argc) {
        const char *base_name = argv[++i];
        const char *dot = strrchr(base_name, '.');
        if (dot && dot != base_name && strlen(dot) <= 5) {
          strncpy(output_base_name, base_name, dot - base_name);
          output_base_name[dot - base_name] = '\0';
        } else {
          strncpy(output_base_name, base_name, MAX_FILENAME - 1);
        }
      }
    } else if (
        strcmp(argv[i], "-map") == 0 || strcmp(argv[i], "--map-file") == 0) {
      if (i + 1 < argc) {
        strncpy(s.map_filename, argv[++i], MAX_FILENAME - 1);
      }
    } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0) {
      if (i + 1 < argc) { input_dir = argv[++i]; }
    } else if (
        strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--sort-width") == 0) {
      sort_by_area = 0;
    } else if (
        strcmp(argv[i], "-rot") == 0 || strcmp(argv[i], "--rotate") == 0) {
      s.allow_rotation = 1;
    } else if (
        strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--square") == 0) {
      s.aspect_w = -1;
    } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--ratio") == 0) {
      if (i + 1 < argc) {
        char ratio_str[64];
        char *sep;
        strncpy(ratio_str, argv[++i], 63);
        ratio_str[63] = '\0';
        sep = strchr(ratio_str, ':');
        if (sep) {
          *sep = '\0';
          s.aspect_w = atoi(ratio_str);
          s.aspect_h = atoi(sep + 1);
          if (s.aspect_w <= 0 || s.aspect_h <= 0) {
            fprintf(
                stderr, "ERROR: Invalid ratio format. Use positive integers "
                        "(e.g., 1:2).\n");
            return 1;
          }
        } else {
          fprintf(stderr, "ERROR: Ratio must be in W:H format (e.g., 1:2).\n");
          return 1;
        }
      }
    } else if (
        strcmp(argv[i], "-grid") == 0 ||
        strcmp(argv[i], "--grid-cells") == 0) {
      if (i + 1 < argc) {
        char dims_str[64];
        char *sep;
        strncpy(dims_str, argv[++i], 63);
        dims_str[63] = '\0';
        sep = strchr(dims_str, 'x');
        if (sep) {
          *sep = '\0';
          s.grid_cols = atoi(dims_str);
          s.grid_rows = atoi(sep + 1);
          if (s.grid_cols <= 0 || s.grid_rows <= 0) {
            fprintf(
                stderr, "ERROR: Invalid grid format. Use positive integers "
                        "(e.g., 8x8).\n");
            return 1;
          }
          s.grid_mode = 1;
        } else {
          fprintf(
              stderr,
              "ERROR: Grid must be in COLUMNSxROWS format (e.g., 8x8).\n");
          return 1;
        }
      }
    } else if (
        strcmp(argv[i], "-cell") == 0 || strcmp(argv[i], "--cell-size") == 0) {
      if (i + 1 < argc) {
        char dims_str[64];
        char *sep;
        strncpy(dims_str, argv[++i], 63);
        dims_str[63] = '\0';
        sep = strchr(dims_str, 'x');
        if (sep) {
          *sep = '\0';
          s.cell_w = atoi(dims_str);
          s.cell_h = atoi(sep + 1);
          if (s.cell_w < 0 || s.cell_h < 0) {
            fprintf(
                stderr, "ERROR: Invalid cell size. Use non-negative integers "
                        "(e.g., 32x32 or 0x0).\n");
            return 1;
          }
        } else if (strcmp(dims_str, "0") == 0) {
          s.cell_w = 0;
          s.cell_h = 0;
        } else {
          fprintf(
              stderr, "ERROR: Cell size must be in WxH format (e.g., 32x32) "
                      "or '0' for dynamic sizing.\n");
          return 1;
        }
      }
    } else if (
        strcmp(argv[i], "-pad") == 0 || strcmp(argv[i], "--padding") == 0) {
      if (i + 1 < argc) {
        s.padding = atoi(argv[++i]);
        if (s.padding < 0) {
          fprintf(stderr, "ERROR: Padding must be non-negative.\n");
          return 1;
        }
      }
    } else if (
        strcmp(argv[i], "-sin") == 0 || strcmp(argv[i], "--scale-in") == 0) {
      if (i + 1 < argc) {
        s.scale_in = strtof(argv[++i], (char **)NULL);
        if (s.scale_in <= 0.0f) {
          fprintf(
              stderr,
              "ERROR: Scale factor for --scale-in must be positive.\n");
          return 1;
        }
      }
    } else if (
        strcmp(argv[i], "-sout") == 0 || strcmp(argv[i], "--scale-out") == 0) {
      if (i + 1 < argc) {
        s.scale_out = strtof(argv[++i], (char **)NULL);
        if (s.scale_out <= 0.0f) {
          fprintf(
              stderr,
              "ERROR: Scale factor for --scale-out must be positive.\n");
          return 1;
        }
      }
    } else if (
        strcmp(argv[i], "-frot") == 0 ||
        strcmp(argv[i], "--final-rotation") == 0) {
      if (i + 1 < argc) {
        s.final_rotation_angle = strtof(argv[++i], (char **)NULL);
        while (s.final_rotation_angle < 0.0f) {
          s.final_rotation_angle += 360.0f;
        }
        while (s.final_rotation_angle >= 360.0f) {
          s.final_rotation_angle -= 360.0f;
        }
      }
    } else if (
        strcmp(argv[i], "-bgc") == 0 || strcmp(argv[i], "--bg-color") == 0) {
      if (i + 1 < argc && !parse_color(argv[++i], s.bg_color)) {
        fprintf(
            stderr, "ERROR: Invalid color format for --bg-color. Use R,G,B,A "
                    "(e.g., 255,0,0,255).\n");
        return 1;
      }
    } else if (
        strcmp(argv[i], "-fc") == 0 || strcmp(argv[i], "--fill-color") == 0) {
      if (i + 1 < argc && !parse_color(argv[++i], s.fill_color)) {
        fprintf(
            stderr, "ERROR: Invalid color format for --fill-color. Use "
                    "R,G,B,A (e.g., 255,0,0,255).\n");
        return 1;
      }
    } else if (
        strcmp(argv[i], "-bpp") == 0 ||
        strcmp(argv[i], "--bits-per-pixel") == 0) {
      if (i + 1 < argc) {
        s.bits_per_channel = atoi(argv[++i]);
        if (s.bits_per_channel < 1 || s.bits_per_channel > 16) {
          fprintf(
              stderr,
              "ERROR: Invalid BPP. Choose an integer between 1 and 16.\n");
          return 1;
        }
      }
    } else if (
        strcmp(argv[i], "-tint") == 0 || strcmp(argv[i], "--mono-tint") == 0) {
      if (i + 1 < argc && !parse_color(argv[++i], s.mono_tint)) {
        fprintf(
            stderr, "ERROR: Invalid color format for --mono-tint. Use R,G,B,A "
                    "(e.g., 255,0,0,255).\n");
        return 1;
      }
    } else if (
        strcmp(argv[i], "-ch") == 0 || strcmp(argv[i], "--channels") == 0) {
      if (i + 1 < argc) {
        s.output_channels = atoi(argv[++i]);
        if (s.output_channels < 1 || s.output_channels > 4) {
          fprintf(
              stderr, "ERROR: Invalid channel count. Choose 1 (Grayscale), 2 "
                      "(GrayscaleA), 3 (RGB), or 4 (RGBA).\n");
          return 1;
        }
      }
    } else if (
        strcmp(argv[i], "-fmt") == 0 || strcmp(argv[i], "--format") == 0) {
      if (i + 1 < argc) {
        char *fmt_str = argv[++i];
        if (strcmp(fmt_str, "png") == 0 || strcmp(fmt_str, "jpg") == 0 ||
            strcmp(fmt_str, "jpeg") == 0 || strcmp(fmt_str, "bmp") == 0) {
          strncpy(s.output_format, fmt_str, 9);
        } else {
          fprintf(
              stderr,
              "ERROR: Invalid output format. Choose png, jpg, or bmp.\n");
          return 1;
        }
      }
    } else if (
        strcmp(argv[i], "-lock") == 0 || strcmp(argv[i], "--lock-axis") == 0) {
      if (i + 1 < argc) {
        char *lock_str = argv[++i];
        if (strcmp(lock_str, "W") == 0 || strcmp(lock_str, "H") == 0) {
          s.lock_axis[0] = lock_str[0];
        } else {
          fprintf(
              stderr,
              "ERROR: Invalid lock axis. Choose W (Width) or H (Height).\n");
          return 1;
        }
      }
    } else if (strcmp(argv[i], "-x") == 0 || strcmp(argv[i], "--exts") == 0) {
      int pos_len = 0;
      int neg_len = 0;
      s.allowed_exts_positive[0] = '\0';
      s.allowed_exts_negative[0] = '\0';
      for (i++; i < argc && argv[i][0] != '-'; i++) {
        char *ext_str = argv[i];
        if (ext_str[0] == '!') {
          if (neg_len + (int)strlen(ext_str) > MAX_EXT_LEN) {
            fprintf(stderr, "ERROR: Negative extension list too long.\n");
            return 1;
          }
          if (neg_len > 0) { s.allowed_exts_negative[neg_len++] = ' '; }
          strcpy(s.allowed_exts_negative + neg_len, ext_str + 1);
          neg_len += (int)strlen(ext_str) - 1;
        } else {
          if (pos_len + (int)strlen(ext_str) > MAX_EXT_LEN) {
            fprintf(stderr, "ERROR: Positive extension list too long.\n");
            return 1;
          }
          if (pos_len > 0) { s.allowed_exts_positive[pos_len++] = ' '; }
          strcpy(s.allowed_exts_positive + pos_len, ext_str);
          pos_len += (int)strlen(ext_str);
        }
      }
      i--;
    } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      printf("Usage: %s [options]\n", argv[0]);
      printf(
          "  -i, --input <dir>         	Input directory (default: .)\n");
      printf("  -o, --output <base_name>  	Output base filename "
             "(default: packed). Extension is added automatically.\n");
      printf("  -map, --map-file <name>   	Output YAML-like metadata map "
             "file (.txt format).\n");
      printf(
          "  -grid COLUMNSxROWS        	Enable Grid Mode: Scales images to "
          "cell size and places them on a fixed grid (e.g., 8x8).\n");
      printf("  -cell WxH|0               	Cell size for Grid Mode "
             "(e.g., 32x32). Use '0' for dynamic/actual image sizing.\n");
      printf("  -pad N                    	Padding around each cell in "
             "Grid Mode (default: 0).\n");
      printf("  -raw, --raw-output        	Output raw binary pixel data "
             "instead of an image file.\n");
      printf("  -fmt, --format <fmt>      	Output format: png, jpg, or "
             "bmp (default: png). Ignored if -raw is used.\n");
      printf("  -x, --exts <exts...>      	Space-delimited list of "
             "allowed extensions.\n");
      printf(
          "  -d, --debug               	Enable verbose debug output.\n");
      printf("  -w, --sort-width          	Sort by width (descending) "
             "instead of area.\n");
      printf("  -r, --rotate            		Allow 90-degree "
             "rotation for MaxRects packing.\n");
      printf("  -rat, --ratio W:H           Force final box to match aspect "
             "ratio (e.g., 1:2).\n");
      printf("  -s, --square              	Search for a final box "
             "closest to a square (1:1).\n");
      printf("  -lock, --lock-axis <W|H>  	Fix one axis: W=pack "
             "wide/rightward, H=pack tall/downward.\n");
      printf("  -sin, --scale-in <f>      	Scale input image dimensions "
             "by factor <f> before packing.\n");
      printf("  -sout, --scale-out <f>    	Scale final output image "
             "dimensions by factor <f> before saving.\n");
      printf("  -frot, --final-rotation <a> Apply arbitrary rotation (0-360 "
             "deg) to the entire sheet.\n");
      printf("  -bpp, --bits-per-pixel <n> 	Output color depth (1 to 16. "
             "Default: 8).\n");
      printf("  -ch, --channels <n>       	Output channels (1, 2, 3, or "
             "4. Default: 4).\n");
      printf("  -tint, --tint <R,G,B,A>			Tint color for 1 BPP "
             "output (default: 255,255,255,255).\n");
      printf("  -bgc, --bg-color <R,G,B,A> 	Canvas background color "
             "(default: 0,0,0,0).\n");
      printf("  -fc, --fill-color <R,G,B,A> Color for empty area inside the "
             "packed sheet (default: 0,0,0,0).\n");
      printf("  -h, --help                	Show this help message.\n");
      return 0;
    }
  }
  if (s.aspect_w > 0 && s.aspect_w == -1) {
    fprintf(
        stderr, "ERROR: Cannot use both ratio (-r) and square (-s) modes "
                "simultaneously.\n");
    return 1;
  }
  if ((s.aspect_w > 0 || s.aspect_w == -1) && s.lock_axis[0] != '\0') {
    fprintf(
        stderr, "ERROR: Cannot use axis locking (-lock) with ratio (-r) or "
                "square (-s) modes simultaneously.\n");
    return 1;
  }
  if (!load_images(input_dir, &s)) {
    fprintf(stderr, "No images loaded or directory not found.\n");
    return 1;
  }
  printf("Loaded %d images.\n", s.count);
  if (!s.grid_mode) {
    if (sort_by_area) {
      qsort(s.rects, s.count, sizeof(Rect), cmp_area_desc);
      printf("Sorting by area (descending).\n");
    } else {
      qsort(s.rects, s.count, sizeof(Rect), cmp_width_desc);
      printf("Sorting by width (descending).\n");
    }
    pack_success = pack_rects(&s);
  } else {
    printf(
        "Using Grid Mode (%dx%d cells, %s cell size, %d padding).\n",
        s.grid_cols, s.grid_rows,
        (s.cell_w == -1)  ? "Implicit"
        : (s.cell_w == 0) ? "Dynamic"
                          : "Fixed",
        s.padding);
    pack_success = pack_rects_grid(&s);
  }
  if (pack_success) {
    ext = s.output_raw ? "raw" : s.output_format;
    strncpy(base_name_buffer, output_base_name, MAX_FILENAME);
    final_name = find_next_filename(base_name_buffer, ext, final_output_path);
    save_packed_image(&s, final_name);
    if (s.map_filename[0] != '\0') {
      save_atlas_map(&s, final_name, s.map_filename);
      printf("Saved atlas metadata to: %s\n", s.map_filename);
    }
  } else {
    printf("Failed to pack rectangles.\n");
    return 1;
  }
  for (i = 0; i < s.count; i++) {
    stbi_image_free(s.rects[i].data);
    if (s.rects[i].rotated_data) { free(s.rects[i].rotated_data); }
  }
  free(s.rects);
  if (free_rects) { free(free_rects); }
  return 0;
}
