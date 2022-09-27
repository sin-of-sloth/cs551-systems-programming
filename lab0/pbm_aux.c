/********************************************************************
*   THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING     *
*   A TUTOR OR CODE WRITTEN BY OTHER STUDENTS                       *
*                               - ARJUN LAL                         *
********************************************************************/

#include "pbm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN_GREYSCALE_PIXEL_VALUE 1
#define MAX_GREYSCALE_PIXEL_VALUE 65535
#define MIN_SCALE_VALUE 1
#define MAX_SCALE_VALUE 8

PPMImage * new_ppmimage( unsigned int w, unsigned int h, unsigned int m ) {
    PPMImage * ppmimage;
    ppmimage = (PPMImage *) malloc(sizeof(PPMImage));
    if(!ppmimage) {
        printf("could not allocate ppmimage");
    }
    ppmimage->height = h;
    ppmimage->width = w;
    ppmimage->max = m;
    int i, j;
    for(i = 0; i < 3; i++) {
        ppmimage->pixmap[i] = (unsigned int **) malloc(sizeof(unsigned int *) * h);
        if(!ppmimage->pixmap[i]) {
            printf("could not allocate ppmimage->pixmap[i]");
        }
        for(j = 0; j < h; j++) {
            ppmimage->pixmap[i][j] = (unsigned int *) malloc(sizeof(unsigned int) * w);
            if(!ppmimage->pixmap[i][j]) {
                printf("could not allocate ppmimage->pixmap[i][j]");
            }
        }
    }
    return ppmimage;
}

PBMImage * new_pbmimage( unsigned int w, unsigned int h ) {
    PBMImage * pbmimage;
    pbmimage = (PBMImage *) malloc(sizeof(PBMImage));
    if(!pbmimage) {
        printf("could not allocate pbmimage");
    }
    pbmimage->height = h;
    pbmimage->width = w;
    pbmimage->pixmap = (unsigned int **) malloc(sizeof(unsigned int *) * h);
    int i;
    for(i = 0; i < h; i++) {
        pbmimage->pixmap[i] = (unsigned int *) malloc(sizeof(unsigned int) * w);
    }
    return pbmimage;
}

PGMImage * new_pgmimage( unsigned int w, unsigned int h, unsigned int m ) {
    PGMImage * pgmimage;
    pgmimage = (PGMImage *) malloc(sizeof(PGMImage));
    if(!pgmimage) {
        printf("could not allocate pbmimage");
    }
    pgmimage->height = h;
    pgmimage->width = w;
    pgmimage->max = m;
    pgmimage->pixmap = (unsigned int **) malloc(sizeof(unsigned int *) * h);
    int i;
    for(i = 0; i < h; i++) {
        pgmimage->pixmap[i] = (unsigned int *) malloc(sizeof(unsigned int) * w);
    }
    return pgmimage;
}

void del_ppmimage( PPMImage * p ) {
    int i, h;
    for(i = 0; i < 3; i++) {
        for(h =  0; h < p->height; h++) {
            free(p->pixmap[i][h]);
        }
        free(p->pixmap[i]);
    }
    free(p);
}

void del_pgmimage( PGMImage * p ) {
    int h;
    for(h = 0; h < p->height; h++) {
        free(p->pixmap[h]);
    }
    free(p->pixmap);
    free(p);
}

void del_pbmimage( PBMImage * p ) {
    int h;
    for(h = 0; h < p->height; h++) {
        free(p->pixmap[h]);
    }
    free(p->pixmap);
    free(p);
}

char * copy_string( char * src ) {
    char * dest = (char *) malloc(sizeof(char) * (strlen(src) + 1));
    if(dest == NULL) {
        printf("couldn't allocate");
        exit(1);
    }
    int i;
    for(i = 0; src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
    return dest;
}

void is_transformation_set( int transformation_flag ) {
    if(transformation_flag) {
        fprintf(stderr, "Error: Multiple transformations specified\n");
        exit(1);
    }
    return;
}

void validate_grayscale_px_val( char * px_val_str ) {
    char * temp;
    long px_val = strtol(px_val_str, &temp, 10);
    if(/*strcmp(temp, "") != 0 || */px_val < MIN_GREYSCALE_PIXEL_VALUE || px_val > MAX_GREYSCALE_PIXEL_VALUE) {
        fprintf(stderr, "Error: Invalid max grayscale pixel value: %s; must be less than 65,536\n", px_val_str);
        exit(1);
    }
    return;
}

void validate_channel( char * channel ) {
    if((strcmp(channel, "red") != 0) && (strcmp(channel, "green") != 0) && (strcmp(channel, "blue") != 0)) {
        fprintf(stderr, "Error: Invalid channel specification: (%s); should be 'red', 'green' or 'blue'\n", channel);
        exit(1);
    }
    return;
}

void validate_scale( char * scale_str ) {
    char * temp;
    long scale = strtol(scale_str, &temp, 10);
    if(/*strcmp(temp, "") != 0 || */scale < MIN_SCALE_VALUE || scale > MAX_SCALE_VALUE) {
        fprintf(stderr, "Error: Invalid scale factor: %s; must be 1-8\n", scale_str);
        exit(1);
    }
    return;
}

void is_output_file_set( char * output_file ) {
    if(!output_file) {
        fprintf(stderr, "Error: No output file specified\n");
        exit(1);
    }
    return;
}

char * get_input_file( int argc, char *argv[], int optind ) {
    // if there's no arguments left, we're missing input file
    if(argc - optind == 0) {
        fprintf(stderr, "Error: No input file specified\n");
        exit(1);
    }

    // if there's more than one argument left, throw error
    // if(argc - optind > 1) {
    //     fprintf(stderr, "Usage: ppmcvt [-bgirsmtno] [FILE]\n");
    //     exit(1);
    // }

    return copy_string(argv[optind]);
}
