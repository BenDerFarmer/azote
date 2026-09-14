#ifndef ICAT_H
#define ICAT_H

int icat_print_image(char *path, int channels, double scale);
int icat_print_image_rect(char *path, int channels, int cols, int rows);
int icat_print_image_by_data(unsigned char *data, int width, int height,
                             int channels, double scale);
int icat_print_image_by_data_rect(unsigned char *data, int width, int height,
                                  int channels, int cols, int rows);

#ifdef ICAT_IMPLEMENTATION

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

static const size_t ICAT_MAX_OUT_CHUNK_SIZE = 4096;
static const size_t ICAT_MAX_IN_CHUNK_SIZE = (ICAT_MAX_OUT_CHUNK_SIZE / 4) * 3;

#ifdef STB_IMAGE_IMPLEMENTATION

int icat_print_image(char *path, int channels, double scale) {
  int x, y, n;
  unsigned char *data = stbi_load(path, &x, &y, &n, channels);

  if (data == NULL) {
    return 1;
  }

  int status = icat_print_image_by_data(data, x, y, channels, scale);
  stbi_image_free(data);

  return status;
}

int icat_print_image_rect(char *path, int channels, int cols, int rows) {
  int x, y, n;
  unsigned char *data = stbi_load(path, &x, &y, &n, channels);

  if (data == NULL) {
    return 1;
  }

  int status = icat_print_image_by_data_rect(data, x, y, channels, cols, rows);
  stbi_image_free(data);

  return status;
}

#endif // STB_IMAGE_IMPLEMENTATION

void base64_encode(const void *data, size_t len, char *out);

int icat_print_image_by_data(unsigned char *data, int width, int height,
                             int channels, double scale) {

  struct winsize sz;
  ioctl(0, TIOCGWINSZ, &sz);
  int c = sz.ws_row * scale;

  return icat_print_image_by_data_rect(data, width, height, channels, c, -1);
}

int icat_print_image_by_data_rect(unsigned char *data, int width, int height,
                                  int channels, int cols, int rows) {

  size_t data_len = width * height * channels;

  unsigned char *dataPtr = data;
  unsigned char *dataEnd = data + data_len;

  char *out = (char *)malloc(ICAT_MAX_OUT_CHUNK_SIZE + 1);
  if (!out) {
    perror("malloc");
    return 1;
  }

  int first = 1;

  while (dataPtr < dataEnd) {
    size_t remainig = dataEnd - dataPtr;
    size_t chunk_size =
        remainig > ICAT_MAX_IN_CHUNK_SIZE ? ICAT_MAX_IN_CHUNK_SIZE : remainig;

    size_t out_len = 4 * ((chunk_size + 2) / 3);

    base64_encode(dataPtr, chunk_size, out);
    out[out_len] = '\0';

    int more = (dataPtr + chunk_size < dataEnd) ? 1 : 0;

    if (first) {

      printf("\x1b_Gt=d,a=T,f=%d,s=%d,v=%d", channels * 8, width, height);

      if (cols >= 0)
        printf(",c=%d", cols);

      if (rows >= 0)
        printf(",r=%d", rows);

      printf(",m=%d;%s\x1b\\", more, out);

    } else {
      printf("\x1b_Gm=%d;%s\x1b\\", more, out);
    }

    first = 0;
    dataPtr += chunk_size;
  }

  printf("\n");
  fflush(stdout);

  free(out);

  return 0;
}

void base64_encode(const void *data, size_t len, char *out) {
  static const char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                            "abcdefghijklmnopqrstuvwxyz"
                            "0123456789+/";
  const unsigned char *in = (const unsigned char *)data;
  if (!out)
    return;

  size_t i = 0, j = 0;
  while (i + 2 < len) {
    uint32_t triple = (in[i] << 16) | (in[i + 1] << 8) | in[i + 2];
    out[j++] = tbl[(triple >> 18) & 0x3F];
    out[j++] = tbl[(triple >> 12) & 0x3F];
    out[j++] = tbl[(triple >> 6) & 0x3F];
    out[j++] = tbl[triple & 0x3F];
    i += 3;
  }

  if (i < len) {
    uint32_t triple = (in[i] << 16);
    out[j++] = tbl[(triple >> 18) & 0x3F];

    if (i + 1 < len) {
      triple |= (in[i + 1] << 8);
      out[j++] = tbl[(triple >> 12) & 0x3F];
      out[j++] = tbl[(triple >> 6) & 0x3F];
      out[j++] = '=';
    } else {
      out[j++] = tbl[(triple >> 12) & 0x3F];
      out[j++] = '=';
      out[j++] = '=';
    }
  }
  return;
}

#endif // ICAT_IMPLEMENTATION

#endif // !ICAT_H
