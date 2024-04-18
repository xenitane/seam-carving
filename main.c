#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOB_IMPL
#include "nob.h"
#include "stb_image.h"
#include "stb_image_write.h"

typedef struct {
  uint32_t red : 8;
  uint32_t green : 8;
  uint32_t blue : 8;
  uint32_t alpha : 8;
} Pixel;

typedef struct {
  int height;
  int width;
  int stride;
  Pixel *items;
} Img;

typedef struct {
  int height;
  int width;
  int stride;
  float *items;
} Mat;

#define mat_alloc(mat_kind, m_name, m_height, m_width)                         \
  NOB_ASSERT((m_width) > 0 && (m_height) > 0 &&                                \
             "enter valid matrix dimensions");                                 \
  mat_kind m_name = {0};                                                       \
  do {                                                                         \
    m_name.height = m_height;                                                  \
    m_name.stride = m_name.width = m_width;                                    \
    m_name.items =                                                             \
        NOB_REALLOC(NULL, sizeof(*m_name.items) * (m_width) * (m_height));     \
    NOB_ASSERT(m_name.items != NULL && "buy more ram lol");                    \
  } while (0)

#define MAT_AT(m, y, x) (m).items[(x) + (y) * (m).stride]
#define MAT_WITHIN(m, y, x)                                                    \
  (0 <= (y) && 0 <= (x) && (y) < (m).height && (x) < (m).width)
#define MAT_SAME_DIM(m1, m2)                                                   \
  ((m1).width == (m2).width && (m1).height == (m2).height)

static float pixel_to_lum(Pixel pixel) {
  /* (0.299*R + 0.587*G + 0.114*B) */
  return (0.299 * pixel.red + 0.587 * pixel.green + 0.114 * pixel.blue) / 255.0;
}

static void rgb_to_lum(Img img, Mat lum) {
  NOB_ASSERT(MAT_SAME_DIM(img, lum) &&
             "target and source must be of same size");
  for (int y = 0; y < img.height; y++) {
    for (int x = 0; x < img.width; x++) {
      MAT_AT(lum, y, x) = pixel_to_lum(MAT_AT(img, y, x));
    }
  }
}

static float sobel_filter_at(Mat lum, int row, int col) {
  static int fltr_conv_krnl[3][3] = {{1, 0, -1}, {2, 0, -2}, {1, 0, -1}};
  float vx = 0.0, vy = 0.0;
  for (int ci = -1; ci < 2; ci++) {
    for (int cj = -1; cj < 2; cj++) {
      if (MAT_WITHIN(lum, row + ci, col + cj)) {
        vx += fltr_conv_krnl[ci + 1][cj + 1] * MAT_AT(lum, row + ci, col + cj);
        vy += fltr_conv_krnl[cj + 1][ci + 1] * MAT_AT(lum, row + ci, col + cj);
      }
    }
  }
  return sqrtf((vx * vx + vy * vy));
}

static void sobel_filter(Mat lum, Mat grad) {
  NOB_ASSERT(MAT_SAME_DIM(lum, grad) &&
             "target and source must be of same size");
  for (int y = 0; y < lum.height; y++) {
    for (int x = 0; x < lum.width; x++) {
      MAT_AT(grad, y, x) = sobel_filter_at(lum, y, x);
    }
  }
}

static void build_dp(Mat mat, Mat dp) {
  NOB_ASSERT(MAT_SAME_DIM(mat, dp) && "target and source must be of same size");
  for (int x = 0; x < mat.width; x++) {
    MAT_AT(dp, 0, x) = MAT_AT(mat, 0, x);
  }
  for (int y = 1; y < mat.height; y++) {
    for (int x = 0; x < mat.width; x++) {
      float min_prev = FLT_MAX;
      for (int i = -1; i < 2; i++) {
        if (x + i >= 0 && x + i < mat.width &&
            min_prev > MAT_AT(dp, y - 1, x + i)) {
          min_prev = MAT_AT(dp, y - 1, x + i);
        }
      }
      MAT_AT(dp, y, x) = MAT_AT(mat, y, x) + min_prev;
    }
  }
}

static void mat_rm_col_at_row(Mat mat, int row, int col) {
  float *mat_row = &MAT_AT(mat, row, 0);
  memmove(mat_row + col, mat_row + col + 1,
          (mat.width - col - 1) * sizeof(float));
}

static void img_rm_col_at_row(Img img, int row, int col) {
  Pixel *pixel_row = &MAT_AT(img, row, 0);
  memmove(pixel_row + col, pixel_row + col + 1,
          (img.width - col - 1) * sizeof(Pixel));
}

static void remove_seam(Mat dp, Img img, Mat lum, Mat edges) {
  int y = dp.height - 1;
  int seam = 0;

  for (int i = 0; i < dp.width; i++) {
    if (MAT_AT(dp, y, seam) > MAT_AT(dp, y, i)) {
      seam = i;
    }
  }

  img_rm_col_at_row(img, y, seam);
  mat_rm_col_at_row(lum, y, seam);
  mat_rm_col_at_row(edges, y, seam);
  while (--y) {
    int seam_rm = seam;
    for (int dx = -1; dx < 2; dx++) {
      if (seam + dx >= 0 && seam + dx < dp.width &&
          MAT_AT(dp, y, seam_rm) > MAT_AT(dp, y, seam + dx)) {
        seam_rm = seam + dx;
      }
    }

    img_rm_col_at_row(img, y, seam_rm);
    mat_rm_col_at_row(lum, y, seam_rm);
    mat_rm_col_at_row(edges, y, seam_rm);
    for (int dx = -2; dx < 2; dx++) {
      if (seam + dx >= 0 && seam + dx < lum.width - 1) {
        MAT_AT(edges, y + 1, seam + dx) =
            sobel_filter_at(lum, y + 1, seam + dx);
      }
    }
    seam = seam_rm;
  }
  for (int dx = -2; dx < 2; dx++) {
    if (seam + dx >= 0 && seam + dx < lum.width - 1) {
      MAT_AT(edges, 0, seam + dx) = sobel_filter_at(lum, 0, seam + dx);
    }
  }
}

static void usage(const char *program) {
  nob_log(NOB_ERROR, "Usage: %s <input> <output>\n", program);
}

int main(int argc, char **argv) {
  const char *program = nob_shift_args(&argc, &argv);

  if (argc <= 0) {
    usage(program);
    nob_log(NOB_ERROR, "no input file provided");
    return EXIT_FAILURE;
  }
  const char *filepath = nob_shift_args(&argc, &argv);

  if (argc <= 0) {
    usage(program);
    nob_log(NOB_ERROR, "no output file provided");
    return EXIT_FAILURE;
  }
  const char *out_file_path = nob_shift_args(&argc, &argv);

  Img img = {0};

  img.items = (Pixel *)stbi_load(filepath, &img.width, &img.height, NULL,
                                 STBI_rgb_alpha);
  img.stride = img.width;
  if (img.items == NULL) {
    nob_log(NOB_ERROR, "unable to read file: %s", filepath);
    return EXIT_FAILURE;
  }

  mat_alloc(Mat, lum, img.height, img.width);
  mat_alloc(Mat, edges, img.height, img.width);
  mat_alloc(Mat, dp, img.height, img.width);

  rgb_to_lum(img, lum);
  sobel_filter(lum, edges);

  int rm_seams = 1;

  rm_seams = 500; // for user to set

  if (rm_seams * 3 > 2 * img.width)
    rm_seams = (img.width * 2) / 3;

  while (rm_seams--) {
    build_dp(edges, dp);
    remove_seam(dp, img, lum, edges);

    img.width--;
    lum.width--;
    edges.width--;
    dp.width--;
  }
  if (!stbi_write_png(out_file_path, img.width, img.height, STBI_rgb_alpha,
                      img.items, img.stride * sizeof(*img.items))) {
    nob_log(NOB_ERROR, "cannot write to file: %s", out_file_path);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
