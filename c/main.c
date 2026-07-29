#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

const int COLOR_NUM = 8;

typedef struct
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    int count;
} ColorCount;

int comp(const void *a, const void *b)
{
    const ColorCount *ca = a;
    const ColorCount *cb = b;

    return cb->count - ca->count;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    }

    int width, height, channels;

    unsigned char *img = stbi_load(
        argv[1],
        &width,
        &height,
        &channels,
        0);

    if (!img)
    {
        printf("Failed to load image\n");
        return 1;
    }

    printf("Original: %dx%d\n", width, height);

    float scale = atof(argv[2]);

    int newWidth = width * scale;
    int newHeight = height * scale / 1.3;

    unsigned char *resized = malloc(
        newWidth * newHeight * channels);

    unsigned char *result = stbir_resize_uint8_linear(
        img,
        width,
        height,
        0,
        resized,
        newWidth,
        newHeight,
        0,
        channels == 3 ? STBIR_RGB : STBIR_RGBA);

    if (result == NULL)
    {
        printf("Resize failed\n");
        return 1;
    }

    ColorCount hist[4096];

    for (int i = 0; i < width * height * 3; i += 3)
    {
        unsigned char r = img[i];
        unsigned char g = img[i + 1];
        unsigned char b = img[i + 2];

        int index =
            ((r >> 4) << 8) |
            ((g >> 4) << 4) |
            (b >> 4);

        hist[index].r = r & 0xF0;
        hist[index].g = g & 0xF0;
        hist[index].b = b & 0xF0;
        hist[index].count++;
    }

    int n = sizeof(hist) / sizeof(hist[0]);
    qsort(hist, n, sizeof(hist[0]), comp);

    for (int i = 0; i < COLOR_NUM; i++)
    {
        printf(
            "%d: RGB(%d,%d,%d) count=%d\n",
            i,
            hist[i].r,
            hist[i].g,
            hist[i].b,
            hist[i].count);
    }

    stbi_write_png(
        "resized.png",
        newWidth,
        newHeight,
        channels,
        resized,
        newWidth * channels);

    stbi_image_free(img);

    printf("Done\n");

    for (int y = 0; y < newHeight; y++)
    {
        for (int x = 0; x < newWidth; x++)
        {
            int index = (y * newWidth + x) * 3;

            unsigned char r = resized[index];
            unsigned char g = resized[index + 1];
            unsigned char b = resized[index + 2];

            int dist[COLOR_NUM];
            int minn = 1000;
            int idx = 0;
            for (int i = 0; i < COLOR_NUM; i++)
            {
                dist[i] = pow(pow(r - hist[i].r, 2) + pow(g - hist[i].g, 2) + pow(b - hist[i].b, 2), 1.0 / 3.0);
                if (dist[i] < minn)
                {
                    minn = dist[i];
                    idx = i;
                }
            }

            printf(
                "\033[48;2;%d;%d;%dm  \033[0m",
                hist[idx].r, hist[idx].g, hist[idx].b);
        }

        printf("\n");
    }

    free(resized);

    return 0;
}