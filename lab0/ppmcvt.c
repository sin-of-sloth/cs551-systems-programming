/********************************************************************
*   THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING     *
*   A TUTOR OR CODE WRITTEN BY OTHER STUDENTS                       *
*                               - ARJUN LAL                         *
********************************************************************/

#include "pbm.h"
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>


/************************************
UTILITY FUNCTION DECLARATIONS
************************************/

char * copy_string( char * src );
void is_transformation_set( int transformation_flag );
void validate_grayscale_px_val( char * px_val_str );
void validate_channel( char * channel );
void validate_scale( char * scale_str );
void is_output_file_set( char * output_file );
void is_input_file_set( char * input_file );
char * get_input_file( int argc, char *argv[], int optind );


/************************************
TRANSFORMATION FUNCTION DECLARATIONS
************************************/

PBMImage * bitmap( PPMImage * ppmimage );
PGMImage * grayscale( PPMImage * ppmimage, unsigned int grayscale_px_val );
void isolate_channel( PPMImage * ppmimage, char * channel );
void remove_channel( PPMImage * ppmimage, char * channel );
void sepia( PPMImage * ppmimage );
void mirror( PPMImage * ppmimage );
PPMImage * thumbnail( PPMImage * ppmimage, int scale );
PPMImage * tile( PPMImage * ppmimage, int scale );


/************************************
MAIN
************************************/

int main( int argc, char *argv[] ) {
    // DECLARATIONS AND INITIALIZATIONS
    int opt;
    char transformation = 'b';  // default option
    int transformation_flag = 0;
    char * input_file = NULL;
    char * output_file = NULL;
    int grayscale_px_val;
    char * channel = NULL;
    int scale;
    char * temp;

    // VALIDATING INPUTS
    while ((opt = getopt(argc, argv, "bg:i:r:smt:n:o:")) != -1) {
        // if the option is not for output file,
        // check if a transformation has already been specified
        if(opt != 'o') {
            // if yes, print error and exit
            is_transformation_set(transformation_flag);
            // else set flags
            transformation = opt;
            transformation_flag = 1;
        }

        // process the option accordingly
        switch (opt) {
            case 'b':
                continue;
            case 'g':
                validate_grayscale_px_val(optarg);
                grayscale_px_val = (int) strtol(optarg, &temp, 10);
                continue;
            case 'i':
                validate_channel(optarg);
                channel = copy_string(optarg);
                continue;
            case 'r':
                validate_channel(optarg);
                channel = copy_string(optarg);
                continue;
            case 's':
                continue;
            case 'm':
                continue;
            case 't':
                validate_scale(optarg);
                scale = (int) strtol(optarg, &temp, 10);
                continue;
            case 'n':
                validate_scale(optarg);
                scale = (int) strtol(optarg, &temp, 10);
                continue;
            case 'o':
                output_file = copy_string(optarg);
                continue;
            // if the option is not any of these, print error and exit
            default:
                fprintf(stderr, "Usage: ppmcvt [-bgirsmtno] [FILE]\n");
                exit(1);
        }
    }

    // throw error if output file not set
    is_output_file_set(output_file);

    // get input file
    input_file = get_input_file(argc, argv, optind);

    // get the image from file to our struct
    // input is always ppm
    PPMImage * ip_ppm;
    ip_ppm = read_ppmfile(input_file);

    // CALL RELEVANT TRANSFORMATION HANDLER
    switch(transformation) {
        // bitmap
        case 'b': {
            PBMImage * pbmimage;
            pbmimage = bitmap(ip_ppm);
            write_pbmfile(pbmimage, output_file);
            del_pbmimage(pbmimage);
            break;
        }
        // grayscale
        case 'g': {
            PGMImage * pgmimage;
            pgmimage = grayscale(ip_ppm, grayscale_px_val);
            write_pgmfile(pgmimage, output_file);
            del_pgmimage(pgmimage);
            break;
        }
        // isolate
        case 'i': {
            isolate_channel(ip_ppm, channel);
            write_ppmfile(ip_ppm, output_file);
            break;
        }
        // remove
        case 'r': {
            remove_channel(ip_ppm, channel);
            write_ppmfile(ip_ppm, output_file);
            break;
        }
        // sepia
        case 's': {
            sepia(ip_ppm);
            write_ppmfile(ip_ppm, output_file);
            break;
        }
        // mirror
        case 'm': {
            mirror(ip_ppm);
            write_ppmfile(ip_ppm, output_file);
            break;
        }
        // thumbnail
        case 't': {
            PPMImage * ppmimage;
            ppmimage = thumbnail(ip_ppm, scale);
            write_ppmfile(ppmimage, output_file);
            del_ppmimage(ppmimage);
            break;
        }
        // tile
        case 'n': {
            PPMImage * ppmimage;
            ppmimage = tile(ip_ppm, scale);
            write_ppmfile(ppmimage, output_file);
            del_ppmimage(ppmimage);
            break;
        }
    }

    del_ppmimage(ip_ppm);

    free(input_file);
    free(output_file);
    if(channel) {
        free(channel);
    }

    return 0;
}


/************************************
TRANSFORMATION FUNCTION DEFINITIONS
************************************/

/*
BITMAP:
b&w bits = Average ( R + G + B ) < PPMMax / 2
*/
PBMImage * bitmap( PPMImage * ppmimage ) {
    int h, w;
    PBMImage * pbmimage;
    pbmimage = new_pbmimage(
        ppmimage->width, ppmimage->height
    );

    for(h = 0; h < ppmimage->height; h++) {
        for(w = 0; w < ppmimage->width; w++) {
            pbmimage->pixmap[h][w] = (
                (float) (ppmimage->pixmap[0][h][w] + ppmimage->pixmap[1][h][w] + ppmimage->pixmap[2][h][w]) / 3
                <
                (float) ppmimage->max / 2
            );
        }
    }

    return pbmimage;
}


/*
GRAYSCALE
grayscale bits = ( Average( R + G + B ) / PPMMax ) × PGMMax
*/
PGMImage * grayscale( PPMImage * ppmimage, unsigned int grayscale_px_val ) {
    int h, w;
    PGMImage * pgmimage;
    pgmimage = new_pgmimage(
        ppmimage->width, ppmimage->height, grayscale_px_val
    );

    for(h = 0; h < ppmimage->height; h++) {
        for(w = 0; w < ppmimage->width; w++) {
            double rgb_average = (ppmimage->pixmap[0][h][w] + ppmimage->pixmap[1][h][w] + ppmimage->pixmap[2][h][w]) / (double) 3;
            pgmimage->pixmap[h][w] = rgb_average * grayscale_px_val / ppmimage->max;
        }
    }

    return pgmimage;
}


/*
ISOLATE:
For all pixels, set all but the specified “red”, “green” or “blue” channel to 0
*/
void isolate_channel( PPMImage * ppmimage, char * channel ) {
    int h, w;

    for(h = 0; h < ppmimage->height; h++) {
        for(w = 0; w < ppmimage->width; w++) {
            if(strcmp(channel, "red") != 0) {
                ppmimage->pixmap[0][h][w] = 0;
            }
            if(strcmp(channel, "green") != 0) {
                ppmimage->pixmap[1][h][w] = 0;
            }
            if(strcmp(channel, "blue") != 0) {
                ppmimage->pixmap[2][h][w] = 0;
            }
        }
    }
}


/*
REMOVE:
For all pixels, set the specified “red”, “green” or “blue” channel to 0
*/
void remove_channel( PPMImage * ppmimage, char * channel ) {
    int h, w;

    for(h = 0; h < ppmimage->height; h++) {
        for(w = 0; w < ppmimage->width; w++) {
            if(strcmp(channel, "red") == 0) {
                ppmimage->pixmap[0][h][w] = 0;
            }
            if(strcmp(channel, "green") == 0) {
                ppmimage->pixmap[1][h][w] = 0;
            }
            if(strcmp(channel, "blue") == 0) {
                ppmimage->pixmap[2][h][w] = 0;
            }
        }
    }
}


/*
SEPIA:
NewR = 0.393 ( OldR ) + 0.769 ( OldG ) + 0.189 x ( OldB )
NewG = 0.349 ( OldR ) + 0.686 ( OldG ) + 0.168 x ( OldB )
NewB = 0.272 ( OldR ) + 0.534 ( OldG ) + 0.131 x ( OldB )
*/
void sepia( PPMImage * ppmimage ) {
    int h, w;
    unsigned int r, g, b, new_r, new_g, new_b;

    for(h = 0; h < ppmimage->height; h++) {
        for(w = 0; w < ppmimage->width; w++) {
            r = ppmimage->pixmap[0][h][w];
            g = ppmimage->pixmap[1][h][w];
            b = ppmimage->pixmap[2][h][w];

            new_r = 0.393 * r + 0.769 * g + 0.189 * b;
            new_g = 0.349 * r + 0.686 * g + 0.168 * b;
            new_b = 0.272 * r + 0.534 * g + 0.131 * b;

            ppmimage->pixmap[0][h][w] = new_r > ppmimage->max ? ppmimage->max : new_r;
            ppmimage->pixmap[1][h][w] = new_g > ppmimage->max ? ppmimage->max : new_g;
            ppmimage->pixmap[2][h][w] = new_b > ppmimage->max ? ppmimage->max : new_b;
        }
    }
}


/*
MIRROR:
Vertically reflect the left half of the image onto the right half
*/
void mirror( PPMImage * ppmimage ) {
    int h, w, c;

    for(h = 0; h < ppmimage->height; h++) {
        for(w = 0; w < ppmimage->width / 2; w++) {
            for(c = 0; c < 3; c++)
                ppmimage->pixmap[c][h][ppmimage->width - w - 1] = ppmimage->pixmap[c][h][w];
        }
    }
}


/*
THUMBNAIL:
The height and width of the output thumbnail should be 1/n the height and
width of the original image, respectively, where n is the input scale factor.
Shrink the input image simply by outputting every nth pixel in both
dimensions starting with the first.
*/
PPMImage * thumbnail( PPMImage * ppmimage, int scale ) {
    int h, w, new_h, new_w, c;
    PPMImage * ppm_thumb;
    ppm_thumb = new_ppmimage(
        (ppmimage->width / scale) + ((ppmimage->width % scale) != 0),
        (ppmimage->height / scale) + ((ppmimage->height % scale) != 0),
        ppmimage->max
    );

    for(h = 0; h < ppm_thumb->height; h ++) {
        for(w = 0; w < ppm_thumb->width; w++) {
            for(c = 0; c < 3; c++) {
                ppm_thumb->pixmap[c][h][w] = ppmimage->pixmap[c][h * scale][w * scale];
            }
        }
    }

    return ppm_thumb;
}


/*
TILE:
Tile n 1/n scale thumbnails, where n is the input scale factor. The output
image should be the same size of the input image
*/
PPMImage * tile( PPMImage * ppmimage, int scale ) {
    int h, w, c, orig_h, orig_w;
    PPMImage * ppm_tile;
    ppm_tile = new_ppmimage(
        ppmimage->width, ppmimage->height, ppmimage->max
    );

    for(h = 0, orig_h = 0; h < ppm_tile->height; h++) {
        for(w = 0, orig_w = 0; w < ppm_tile->width; w++) {
            for(c = 0; c < 3; c++) {
                ppm_tile->pixmap[c][h][w] = ppmimage->pixmap[c][orig_h][orig_w];
            }
            orig_w += scale;
            if(orig_w >= ppmimage->width)   orig_w = 0;
        }
        orig_h += scale;
        if(orig_h >= ppmimage->height)   orig_h = 0;
    }

    return ppm_tile;
}
