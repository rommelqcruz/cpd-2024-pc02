#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <omp.h>

typedef struct {
    unsigned char r, g, b;
} Pixel;

typedef struct {
    int width;
    int height;
    Pixel* data;
} Image;

// Function to read PPM image
Image* readPPM(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Cannot open file %s\n", filename);
        exit(1);
    }

    Image* img = malloc(sizeof(Image));
    char line[100];
    fgets(line, sizeof(line), file); // Read P3
    
    // Skip comments
    do {
        fgets(line, sizeof(line), file);
    } while (line[0] == '#');

    sscanf(line, "%d %d", &img->width, &img->height);
    fgets(line, sizeof(line), file); // Read max color value

    img->data = malloc(img->width * img->height * sizeof(Pixel));
    
    for (int i = 0; i < img->width * img->height; i++) {
        int r, g, b;
        fscanf(file, "%d %d %d", &r, &g, &b);
        img->data[i].r = (unsigned char)r;
        img->data[i].g = (unsigned char)g;
        img->data[i].b = (unsigned char)b;
    }

    fclose(file);
    return img;
}

// Function to write PPM image
void writePPM(const char* filename, Image* img) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Cannot open file %s for writing\n", filename);
        exit(1);
    }

    fprintf(file, "P3\n");
    fprintf(file, "%d %d\n", img->width, img->height);
    fprintf(file, "255\n");

    for (int i = 0; i < img->width * img->height; i++) {
        fprintf(file, "%d %d %d ", 
                img->data[i].r, 
                img->data[i].g, 
                img->data[i].b);
        if ((i + 1) % img->width == 0) fprintf(file, "\n");
    }

    fclose(file);
}

// Calculate average color in a region
Pixel calculateAverage(Image* img, int x, int y, int width, int height) {
    long sum_r = 0, sum_g = 0, sum_b = 0;
    int count = 0;

    for (int j = y; j < y + height; j++) {
        for (int i = x; i < x + width; i++) {
            Pixel p = img->data[j * img->width + i];
            sum_r += p.r;
            sum_g += p.g;
            sum_b += p.b;
            count++;
        }
    }

    Pixel avg = {
        (unsigned char)(sum_r / count),
        (unsigned char)(sum_g / count),
        (unsigned char)(sum_b / count)
    };
    return avg;
}

// Calculate color variance in a region
double calculateVariance(Image* img, int x, int y, int width, int height, Pixel avg) {
    double sum = 0;
    int count = 0;

    for (int j = y; j < y + height; j++) {
        for (int i = x; i < x + width; i++) {
            Pixel p = img->data[j * img->width + i];
            double dr = p.r - avg.r;
            double dg = p.g - avg.g;
            double db = p.b - avg.b;
            sum += dr*dr + dg*dg + db*db;
            count++;
        }
    }

    return sum / (count * 3);
}

// Fill region with a color
void fillRegion(Image* img, int x, int y, int width, int height, Pixel color) {
    for (int j = y; j < y + height; j++) {
        for (int i = x; i < x + width; i++) {
            img->data[j * img->width + i] = color;
        }
    }
}

// Sequential version of quadtree compression
void denoisingSequential(Image* img, int x, int y, int width, int height, double threshold) {
    if (width <= 1 || height <= 1) return;

    Pixel avg = calculateAverage(img, x, y, width, height);
    double variance = calculateVariance(img, x, y, width, height, avg);

    if (variance < threshold) {
        fillRegion(img, x, y, width, height, avg);
    } else {
        int new_width = width / 2;
        int new_height = height / 2;

        denoisingSequential(img, x, y, new_width, new_height, threshold);
        denoisingSequential(img, x + new_width, y, new_width, new_height, threshold);
        denoisingSequential(img, x, y + new_height, new_width, new_height, threshold);
        denoisingSequential(img, x + new_width, y + new_height, new_width, new_height, threshold);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <input.ppm> <output.ppm> <threshold>\n", argv[0]);
        return 1;
    }

    const char* input_file = argv[1];
    const char* output_file = argv[2];
    double threshold = atof(argv[3]);

    Image* img = readPPM(input_file);

    // Sequential version
    double start_time = omp_get_wtime();
    denoisingSequential(img, 0, 0, img->width, img->height, threshold);
    double sequential_time = omp_get_wtime() - start_time;

    printf("Sequential time: %.3f seconds\n", sequential_time);

    writePPM(output_file, img);

    free(img->data);
    free(img);

    return 0;
}
