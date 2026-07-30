#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

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

unsigned char *resize(unsigned char *img, int width, int height, int newWidth, int newHeight, int channels)
{
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
        return NULL;
    }

    stbi_write_png(
        "resized.png",
        newWidth,
        newHeight,
        channels,
        resized,
        newWidth * channels);

    return resized;
}

ColorCount *create_hist(unsigned char *img, int width, int height, int channels, int color_num, float brightness, ColorCount hist[4096], int n)
{
    memset(hist, 0, sizeof(ColorCount) * 4096);

    for (int i = 0; i < width * height * channels; i += channels)
    {
        unsigned char r = img[i];
        unsigned char g = img[i + 1];
        unsigned char b = img[i + 2];
        unsigned char a = 255;
        if (channels == 4)
        {
            a = img[i + 3];
        }

        if (a < 20)
        {
            continue;
        }

        int index =
            ((r >> 4) << 8) |
            ((g >> 4) << 4) |
            (b >> 4);

        hist[index].r = r & 0xF0;
        hist[index].g = g & 0xF0;
        hist[index].b = b & 0xF0;
        hist[index].count++;
    }

    qsort(hist, n, sizeof(hist[0]), comp);

    for (int i = 0; i < color_num; ++i)
    {
        hist[i].r = fmin(255, hist[i].r * brightness);
        hist[i].g = fmin(255, hist[i].g * brightness);
        hist[i].b = fmin(255, hist[i].b * brightness);
    }

    return hist;
}

void print_img(unsigned char *img, int width, int height, int channels, int color_num, int liftblack, bool monochrome, bool ascii, ColorCount *hist)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int index = (y * width + x) * channels;

            unsigned char r = img[index];
            unsigned char g = img[index + 1];
            unsigned char b = img[index + 2];
            unsigned char a = 255;
            if (channels == 4)
            {
                a = img[index + 3];
                if (a < 20)
                {
                    printf("  ");
                    continue;
                }
            }

            int dist[color_num];
            int minn = 1000;
            int idx = 0;
            for (int i = 0; i < color_num; i++)
            {
                dist[i] = pow(pow(r - hist[i].r, 2) + pow(g - hist[i].g, 2) + pow(b - hist[i].b, 2), 1.0 / 3.0);
                if (dist[i] < minn)
                {
                    minn = dist[i];
                    idx = i;
                }
            }

            char density[128] = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^'.";
            size_t char_num = strlen(density);
            int max_spaces = sizeof(density) - char_num - 1;
            if (liftblack > max_spaces)
                liftblack = max_spaces;
            if (liftblack > 0)
            {
                memset(density + char_num, ' ', liftblack);
                density[char_num + liftblack] = '\0';
            }
            int total_len = char_num + liftblack;

            int brightness = 255 - (r + b + g) / 3;
            char c = density[brightness * (total_len - 1) / 255];

            if (!monochrome)
            {
                printf(
                    ascii ? "\033[38;2;%d;%d;%dm%c%c\033[0m" : "\033[48;2;%d;%d;%dm  \033[0m",
                    hist[idx].r,
                    hist[idx].g, hist[idx].b, c, c);
            }
            else
            {
                printf(
                    ascii ? "\033[38;2;%d;%d;%dm%c%c\033[0m" : "\033[48;2;%d;%d;%dm  \033[0m",
                    brightness,
                    brightness, brightness, c, c);
            }
        }

        printf("\n");
    }
}

int main(int argc, char *argv[])
{
    int color_num = 32;
    float scale = 1.0;
    char *filename = NULL;
    bool ascii = true;
    bool monochrome = false;
    float brightness = 1.0;
    int liftblack = 0;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--colors") == 0 ||
            strcmp(argv[i], "-c") == 0)
        {
            if (i + 1 < argc)
            {
                color_num = atoi(argv[++i]);
            }
        }
        else if (strncmp(argv[i], "-c=", 3) == 0)
        {
            color_num = atoi(argv[i] + 3);
        }
        else if (strcmp(argv[i], "--scale") == 0 ||
                 strcmp(argv[i], "-s") == 0)
        {
            if (i + 1 < argc)
            {
                scale = atof(argv[++i]);
            }
        }
        else if (strncmp(argv[i], "-s=", 3) == 0)
        {
            scale = atof(argv[i] + 3);
        }
        else if (strcmp(argv[i], "--no_ascii") == 0 ||
                 strcmp(argv[i], "-n") == 0)
        {
            ascii = false;
        }
        else if (strcmp(argv[i], "--monochrome") == 0 ||
                 strcmp(argv[i], "-m") == 0)
        {
            monochrome = true;
        }
        else if (strcmp(argv[i], "--brightness") == 0 ||
                 strcmp(argv[i], "-b") == 0)
        {
            if (i + 1 < argc)
            {
                brightness = atof(argv[++i]);
            }
        }
        else if (strncmp(argv[i], "-b=", 3) == 0)
        {
            brightness = atof(argv[i] + 3);
        }
        else if (strcmp(argv[i], "--liftblack") == 0 ||
                 strcmp(argv[i], "-l") == 0)
        {
            if (i + 1 < argc)
            {
                liftblack = atoi(argv[++i]);
            }
        }
        else if (strncmp(argv[i], "-l=", 3) == 0)
        {
            liftblack = atoi(argv[i] + 3);
        }
        else if (strcmp(argv[i], "--help") == 0 ||
                 strcmp(argv[i], "-h") == 0)
        {
            printf("Usage: %s [options] <image_file>\n", argv[0]);
            printf("Options:\n");
            printf("  -c, --colors N        Number of colors to use (default 8)\n");
            printf("  -s, --scale N         Scale factor (default 1.0)\n");
            printf("  -n, --no_ascii        Use block characters instead of ASCII\n");
            printf("  -m, --monochrome      Monochrome output\n");
            printf("  -b, --brightness N    Brightness multiplier (default 1.0)\n");
            printf("  -l, --liftblack N     Add N spaces to the density map for black pixels\n");
            printf("  -h, --help            Show this help\n");
            printf("  <image_file>          Path to image\n");
            return 0;
        }
        else
        {
            filename = argv[i];
        }
    }

    int width, height, channels;
    unsigned char *img = stbi_load(argv[1], &width, &height, &channels, 4);
    if (!img)
    {
        printf("Failed to load image\n");
        return 1;
    }
    int newWidth = width * scale;
    int newHeight = height * scale;
    unsigned char *resized = resize(img, width, height, newWidth, newHeight, 4);
    stbi_image_free(img);
    if (!resized)
        return 1;

    ColorCount hist[4096];
    int hist_size = sizeof(hist) / sizeof(hist[0]);
    create_hist(resized, newWidth, newHeight, 4, color_num, brightness, hist, hist_size);

    printf("\n");
    print_img(resized, newWidth, newHeight, 4, color_num, liftblack, monochrome, ascii, hist);
    printf("\n");
    free(resized);

    return 0;
}