#ifndef WAVL_H
#define WAVL_H
#define _CRT_SECURE_NO_WARNINGS
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#define PACK_STRUCT
#pragma pack(push, 1)
#else
#define PACK_STRUCT __attribute__((packed))
#endif
typedef struct PACK_STRUCT {
  char chunk_id[4];
  uint32_t chunk_size;
  char format[4];
} riff_header_t;

typedef struct PACK_STRUCT {
  char subchunk_id[4];
  uint32_t subchunk_size;
  uint16_t audio_format;
  uint16_t num_channels;
  uint32_t sample_rate;
  uint32_t byte_rate;
  uint16_t block_align;
  uint16_t bits_per_sample;
} fmt_chunk_t;

typedef struct PACK_STRUCT {
  char subchunk_id[4];
  uint32_t subchunk_size;
} data_chunk_header_t;

#if defined(_WIN32) || defined(_WIN64)
#pragma pack(pop)
#endif

typedef struct {
  uint16_t audio_format;
  uint16_t num_channels;
  uint32_t sample_rate;
  uint32_t byte_rate;
  uint16_t block_align;
  uint16_t bits_per_sample;
  uint32_t data_size;
  void *data;
} wav_file_t;

static int read_chunk_header(FILE *file, char *id, uint32_t *size) {
  if (fread(id, 1, 4, file) != 4) {
    return 0;
  }
  if (fread(size, 4, 1, file) != 1) {
    return 0;
  }
  return 1;
}

static int skip_chunk(FILE *file, uint32_t size) {
  if (fseek(file, size, SEEK_CUR) != 0) {
    return 0;
  }
  return 1;
}

wav_file_t *wav_load(const char *filename) {
  FILE *file;
  riff_header_t riff_header;
  fmt_chunk_t fmt_chunk;
  data_chunk_header_t data_header;
  wav_file_t *wav;
  int found_fmt;
  int found_data;
  char chunk_id[4];
  uint32_t chunk_size;
  long file_size;
  long current_pos;

  file = fopen(filename, "rb");
  if (!file) {
    return NULL;
  }

  if (fread(&riff_header, sizeof(riff_header_t), 1, file) != 1) {
    fclose(file);
    return NULL;
  }

  if (memcmp(riff_header.chunk_id, "RIFF", 4) != 0 ||
      memcmp(riff_header.format, "WAVE", 4) != 0) {
    fclose(file);
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  file_size = ftell(file);
  fseek(file, sizeof(riff_header_t), SEEK_SET);

  found_fmt = 0;
  found_data = 0;
  memset(&fmt_chunk, 0, sizeof(fmt_chunk_t));
  memset(&data_header, 0, sizeof(data_chunk_header_t));

  while (ftell(file) < file_size) {
    current_pos = ftell(file);

    if (!read_chunk_header(file, chunk_id, &chunk_size)) {
      break;
    }

    if (memcmp(chunk_id, "fmt ", 4) == 0) {
      if (chunk_size < 16) {
        fclose(file);
        return NULL;
      }

      if (fread(&fmt_chunk.audio_format, 2, 1, file) != 1 ||
          fread(&fmt_chunk.num_channels, 2, 1, file) != 1 ||
          fread(&fmt_chunk.sample_rate, 4, 1, file) != 1 ||
          fread(&fmt_chunk.byte_rate, 4, 1, file) != 1 ||
          fread(&fmt_chunk.block_align, 2, 1, file) != 1 ||
          fread(&fmt_chunk.bits_per_sample, 2, 1, file) != 1) {
        fclose(file);
        return NULL;
      }

      if (chunk_size > 16) {
        if (!skip_chunk(file, chunk_size - 16)) {
          fclose(file);
          return NULL;
        }
      }

      found_fmt = 1;
    } else if (memcmp(chunk_id, "data", 4) == 0) {
      data_header.subchunk_size = chunk_size;
      memcpy(data_header.subchunk_id, chunk_id, 4);
      found_data = 1;
      break;
    } else {
      if (!skip_chunk(file, chunk_size)) {
        break;
      }
    }

    if (chunk_size % 2 == 1) {
      fseek(file, 1, SEEK_CUR);
    }
  }

  if (!found_fmt || !found_data) {
    fclose(file);
    return NULL;
  }

  if (fmt_chunk.audio_format != 1 && fmt_chunk.audio_format != 3) {
    fclose(file);
    return NULL;
  }

  wav = (wav_file_t *)malloc(sizeof(wav_file_t));
  if (!wav) {
    fclose(file);
    return NULL;
  }

  wav->audio_format = fmt_chunk.audio_format;
  wav->num_channels = fmt_chunk.num_channels;
  wav->sample_rate = fmt_chunk.sample_rate;
  wav->byte_rate = fmt_chunk.byte_rate;
  wav->block_align = fmt_chunk.block_align;
  wav->bits_per_sample = fmt_chunk.bits_per_sample;
  wav->data_size = data_header.subchunk_size;

  wav->data = malloc(wav->data_size);
  if (!wav->data) {
    free(wav);
    fclose(file);
    return NULL;
  }

  if (fread(wav->data, 1, wav->data_size, file) != wav->data_size) {
    free(wav->data);
    free(wav);
    fclose(file);
    return NULL;
  }

  fclose(file);
  return wav;
}

void wav_free(wav_file_t *wav) {
  if (wav) {
    if (wav->data) {
      free(wav->data);
    }
    free(wav);
  }
}

int wav_save(const char *filename, const wav_file_t *wav) {
  FILE *file;
  riff_header_t riff_header;
  fmt_chunk_t fmt_chunk;
  data_chunk_header_t data_header;
  uint32_t file_size;

  if (!wav || !wav->data) {
    return 0;
  }

  file = fopen(filename, "wb");
  if (!file) {
    return 0;
  }

  file_size =
      4 + sizeof(fmt_chunk_t) + sizeof(data_chunk_header_t) + wav->data_size;

  memcpy(riff_header.chunk_id, "RIFF", 4);
  riff_header.chunk_size = file_size;
  memcpy(riff_header.format, "WAVE", 4);

  memcpy(fmt_chunk.subchunk_id, "fmt ", 4);
  fmt_chunk.subchunk_size = 16;
  fmt_chunk.audio_format = wav->audio_format;
  fmt_chunk.num_channels = wav->num_channels;
  fmt_chunk.sample_rate = wav->sample_rate;
  fmt_chunk.byte_rate = wav->byte_rate;
  fmt_chunk.block_align = wav->block_align;
  fmt_chunk.bits_per_sample = wav->bits_per_sample;

  memcpy(data_header.subchunk_id, "data", 4);
  data_header.subchunk_size = wav->data_size;

  if (fwrite(&riff_header, sizeof(riff_header_t), 1, file) != 1 ||
      fwrite(&fmt_chunk, sizeof(fmt_chunk_t), 1, file) != 1 ||
      fwrite(&data_header, sizeof(data_chunk_header_t), 1, file) != 1 ||
      fwrite(wav->data, 1, wav->data_size, file) != wav->data_size) {
    fclose(file);
    return 0;
  }

  fclose(file);
  return 1;
}

#endif
