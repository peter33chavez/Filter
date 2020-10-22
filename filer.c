#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "helpers.h"
#include <math.h>
#include <ctype.h>
#include <cs50.h>
#include "bmp.h"

int main(int argc, char *argv[])
{

    // Define allowable filters
    char *filters = "bgrs";

    // Get filter flag and check validity
    char filter = getopt(argc, argv, filters);
    if (filter == '?')
    {
        fprintf(stderr, "Invalid filter.\n");
        return 1;
    }

    // Ensure only one filter
    if (getopt(argc, argv, filters) != -1)
    {
        fprintf(stderr, "Only one filter allowed.\n");
        return 2;
    }

    // Ensure proper usage
    if (argc != optind + 2)
    {
        fprintf(stderr, "Usage: filter [flag] infile outfile\n");
        return 3;
    }

    // Remember filenames
    char *infile = argv[optind];
    char *outfile = argv[optind + 1];

    // Open input file
    FILE *inptr = fopen(infile, "r");
    if (inptr == NULL)
    {
        fprintf(stderr, "Could not open %s.\n", infile);
        return 4;
    }

    // Open output file
    FILE *outptr = fopen(outfile, "w");
    if (outptr == NULL)
    {
        fclose(inptr);
        fprintf(stderr, "Could not create %s.\n", outfile);
        return 5;
    }

    // Read infile's BITMAPFILEHEADER
    BITMAPFILEHEADER bf;
    fread(&bf, sizeof(BITMAPFILEHEADER), 1, inptr);

    // Read infile's BITMAPINFOHEADER
    BITMAPINFOHEADER bi;
    fread(&bi, sizeof(BITMAPINFOHEADER), 1, inptr);

    // Ensure infile is (likely) a 24-bit uncompressed BMP 4.0
    if (bf.bfType != 0x4d42 || bf.bfOffBits != 54 || bi.biSize != 40 ||
        bi.biBitCount != 24 || bi.biCompression != 0)
    {
        fclose(outptr);
        fclose(inptr);
        fprintf(stderr, "Unsupported file format.\n");
        return 6;
    }

    int height = abs(bi.biHeight);
    int width = bi.biWidth;

    // Allocate memory for image
    RGBTRIPLE(*image)[width] = calloc(height, width * sizeof(RGBTRIPLE));
    if (image == NULL)
    {
        fprintf(stderr, "Not enough memory to store image.\n");
        fclose(outptr);
        fclose(inptr);
        return 7;
    }

    // Determine padding for scanlines
    int padding = (4 - (width * sizeof(RGBTRIPLE)) % 4) % 4;

    // Iterate over infile's scanlines
    for (int i = 0; i < height; i++)
    {
        // Read row into pixel array
        fread(image[i], sizeof(RGBTRIPLE), width, inptr);

        // Skip over padding
        fseek(inptr, padding, SEEK_CUR);
    }

    // Filter image
    switch (filter)
    {
        // Blur
        case 'b':
            blur(height, width, image);
            break;

        // Grayscale
        case 'g':
            grayscale(height, width, image);
            break;

        // Reflection
        case 'r':
            reflect(height, width, image);
            break;

        // Sepia
        case 's':
            sepia(height, width, image);
            break;
    }

    // Write outfile's BITMAPFILEHEADER
    fwrite(&bf, sizeof(BITMAPFILEHEADER), 1, outptr);

    // Write outfile's BITMAPINFOHEADER
    fwrite(&bi, sizeof(BITMAPINFOHEADER), 1, outptr);

    // Write new pixels to outfile
    for (int i = 0; i < height; i++)
    {
        // Write row to outfile
        fwrite(image[i], sizeof(RGBTRIPLE), width, outptr);

        // Write padding at end of row
        for (int k = 0; k < padding; k++)
        {
            fputc(0x00, outptr);
        }
    }
    
    // Free memory for image
    free(image);

    // Close infile
    fclose(inptr);

    // Close outfile
    fclose(outptr);

    return 0;
}

// Convert image to grayscale
void grayscale(int height, int width, RGBTRIPLE image[height][width])
{
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            //get each pixels 3 color values
            RGBTRIPLE color = image[i][j];
            // add all 3 color values together then divide them to find average
            int average = round((color.rgbtBlue + color.rgbtGreen + color.rgbtRed) / 3.0);
            // initalize the value 255 if average is over 255
            if (average > 255)
            {
                average = 255;
            }    
            //update each array in RGBTRIPLE with the average
            image[i][j].rgbtBlue = image[i][j].rgbtGreen = image[i][j].rgbtRed = average;

        }

    }
    return; //RGBTRIPLE updated values
}
// Convert image to sepia
void sepia(int height, int width, RGBTRIPLE image[height][width])
{
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            RGBTRIPLE color = image[i][j];
            int originalRed = color.rgbtRed;
            int originalGreen = color.rgbtGreen;
            int originalBlue = color.rgbtBlue;

            //makes sepia color tone
            int sepiaRed = round(.393 * originalRed + .769 * originalGreen + .189 * originalBlue);
            int sepiaGreen = round(.349 * originalRed + .686 * originalGreen + .168 * originalBlue);
            int sepiaBlue = round(.272 * originalRed + .534 * originalGreen + .131 * originalBlue);
            //caps values at 255
            if (sepiaRed > 255)
            {
                sepiaRed = 255;
            }
            if (sepiaGreen > 255)
            {
                sepiaGreen = 255;
            }    
            if (sepiaBlue > 255)
            {
                sepiaBlue = 255;
            }
            //initialize the new values to its respected value
            image[i][j].rgbtBlue = sepiaBlue;
            image[i][j].rgbtGreen = sepiaGreen;
            image[i][j].rgbtRed = sepiaRed;
        }
    }
    return;
}
//putting the values in color 1 into a temp variable so that i can initalize color 2 into 1
void swap(RGBTRIPLE *color1, RGBTRIPLE *color2)
{
    RGBTRIPLE temp = *color1;
    *color1 = *color2;
    *color2 = temp;
}

// Reflect image horizontally
void reflect(int height, int width, RGBTRIPLE image[height][width])
{
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width / 2; j++)
        {
            swap(&image[i][j], &image[i][width - 1 - j]);
        }
    }
    return;
}
RGBTRIPLE blur_pixel(int i, int j, int height, int width, RGBTRIPLE image[height][width])
{
    int pixel_counter = 0;
    int blue = 0;
    int red = 0;
    int green = 0;
    // loop to get all surrounding pixels
    for (int di = -1; di < 2; di++)
    {
        for (int dj = -1; dj < 2; dj++)
        {
            //make a new i and j so that you can make the if statement for the color calculations
            int new_i = i + di;
            int new_j = j + dj;
            if (new_i >= 0 && new_i < height && new_j >= 0 && new_j < width)
            {
                //increment a counter based off of the amount of vailid pixels
                //add up all the color channels for each pixel
                blue += image[new_i][new_j].rgbtBlue;
                red += image[new_i][new_j].rgbtRed;
                green += image[new_i][new_j].rgbtGreen;
            }
        }
    }
    RGBTRIPLE new_pixel;
    new_pixel.rgbtBlue = round((float)blue / pixel_counter);
    new_pixel.rgbtRed = round((float)red / pixel_counter);
    new_pixel.rgbtGreen = round((float)green / pixel_counter);
    return new_pixel;
}
// Blur image
void blur(int height, int width, RGBTRIPLE image[height][width])
{
    RGBTRIPLE new_image[height][width];
    //interate over image
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            new_image[i][j] = blur_pixel(i, j, height, width, image);
        }
    }    
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            image[i][j] = new_image[i][j];
        }
    }
    return;
}

