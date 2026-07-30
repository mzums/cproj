#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

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

int check_gif(const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        return 0;
    }

    char header[6];

    if (fread(header, 1, 6, file) != 6)
    {
        fclose(file);
        return 0;
    }

    fclose(file);

    return memcmp(header, "GIF87a", 6) == 0 ||
           memcmp(header, "GIF89a", 6) == 0;
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

    return resized;
}

ColorCount *create_hist(unsigned char *img, int width, int height, int channels, ColorCount hist[4096])
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
                if (a < 50)
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
            memset(density + char_num, ' ', liftblack);
            density[char_num + liftblack] = '\0';
            char_num = strlen(density);

            int brightness = 255 - (r + b + g) / 3;

            char c = density[brightness * char_num / 255];

            char *print_str = "\033[38;2;%d;%d;%dm%c%c\033[0m";
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
    int color_num = 8;
    float scale = 1.0;
    char *filename = NULL;
    bool ascii = true;
    bool monochrome = false;
    float brightness = 1.0;
    int liftblack = 0;
    bool isgif = false;

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
        else
        {
            filename = argv[i];
        }
    }

    if (check_gif(argv[1]))
    {
        isgif = true;
        printf("is gif");
    }

    int width, height, channels;

    unsigned char *img = stbi_load(
        argv[1],
        &width,
        &height,
        &channels,
        4);

    channels = 4;

    if (!img)
    {
        printf("Failed to load image\n");
        return 1;
    }

    int newWidth = width * scale;
    int newHeight = height * scale;

    unsigned char *resized = resize(img, width, height, newWidth, newHeight, channels);

    if (resized == NULL)
    {
        printf("Resize failed\n");
        return 1;
    }

    ColorCount hist[4096];
    create_hist(resized, newWidth, newHeight, channels, hist);

    int n = sizeof(hist) / sizeof(hist[0]);
    qsort(hist, n, sizeof(hist[0]), comp);

    for (int i = 0; i < color_num; ++i)
    {
        hist[i].r = fmin(255, hist[i].r * brightness);
        hist[i].g = fmin(255, hist[i].g * brightness);
        hist[i].b = fmin(255, hist[i].b * brightness);
    }

    stbi_write_png(
        "resized.png",
        newWidth,
        newHeight,
        channels,
        resized,
        newWidth * channels);

    stbi_image_free(img);

    printf("\n");

    print_img(resized, newWidth, newHeight, channels, color_num, liftblack, monochrome, ascii, hist);

    printf("\n");

    free(resized);

    return 0;
}