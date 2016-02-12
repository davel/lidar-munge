#define _GNU_SOURCE
#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <inttypes.h>
#include <float.h>
#include <strings.h>
#include "tiffio.h"

const int tilesize = 1000;

const int x_tile_start = 17;
const int y_tile_start = 51;
const int x_tile_count =2;
const int y_tile_count =2;

int main(int argc, char* argv[]) {
    TIFF *output_image;
    int m_height = -1;
    int m_width  = -1;
    double cell_size = -1;
    double nodata = -1;
    int xllcorner;
    int yllcorner;

    // Open the TIFF file
    if((output_image = TIFFOpen("foo.tiff", "w")) == NULL){
        assert(0);
    }
    TIFFSetField(output_image, TIFFTAG_IMAGEWIDTH, tilesize * x_tile_count);
    TIFFSetField(output_image, TIFFTAG_IMAGELENGTH, tilesize * y_tile_count);
    assert(TIFFSetField(output_image, TIFFTAG_ROWSPERSTRIP, tilesize * y_tile_count)==1);
    TIFFSetField(output_image, TIFFTAG_BITSPERSAMPLE, 16);
    TIFFSetField(output_image, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(output_image, TIFFTAG_PHOTOMETRIC, 1); // black is zero
 
//    TIFFSetField(output_image, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);

    double *fbuf = malloc(sizeof(double) * tilesize * x_tile_count * y_tile_count * tilesize);
    assert(fbuf != NULL);
    double min =  DBL_MAX;
    double max = -DBL_MAX;

    for (int tilex = x_tile_start; tilex < x_tile_start+x_tile_count; tilex++) {
        for (int tiley = y_tile_start; tiley < y_tile_start+y_tile_count; tiley++) {
            char *filename;
            assert(asprintf(&filename, "/home/davel/Downloads/lidar/tq%02d%02d_DTM_1m.asc", tilex, tiley));
            assert(filename);

            printf("%s\n", filename);
            FILE *asc = fopen(filename, "r");

            if (asc != NULL) {
                free(filename);
                assert(asc != NULL);

                assert(fscanf(asc, "ncols %d\n", &m_width));
                assert(fscanf(asc, "nrows %d\n", &m_height));
                assert(fscanf(asc, "xllcorner %d\n", &xllcorner));
                assert(fscanf(asc, "yllcorner %d\n", &yllcorner));
                assert(fscanf(asc, "cellsize %lf\n", &cell_size));
                assert(fscanf(asc, "NODATA_value %lf\n", &nodata));

                assert(m_width == tilesize);
                assert(m_height == tilesize);

                for (int y=0; y<m_height; y++) {
                    for (int x=0; x<m_width; x++) {
                        double *f = &fbuf[(tilesize*x_tile_count)*(y+tilesize*(tiley-y_tile_start)) +tilesize*(tilex-x_tile_start) + x];
                        if (x == (m_width-1)) {
                            assert(fscanf(asc, "%lf\n", f));
                        }
                        else {
                            assert(fscanf(asc, "%lf ", f));
                        }
                        if (*f == nodata) {
                            *f = 0;
                        }
                        if (*f < min) min = *f;
                        if (*f > max) max = *f;
                    }
                }
                fclose(asc);
            }
            // Write the information to the file
            // Close the file
        }
    }

    double scale = 65535.0/(max-min);

    printf("%lf %lf %lf\n", scale, min, max);

    uint16_t *dbuf = malloc(sizeof(uint16_t) * tilesize * x_tile_count * tilesize * y_tile_count);
    assert(dbuf != NULL);

    for (int y=0; y<(tilesize * y_tile_count); y++) {
        for (int x=0; x<(tilesize * x_tile_count); x++) {
            dbuf[x+y*tilesize*x_tile_count] = (uint16_t) (fbuf[x+y*tilesize*x_tile_count] - min)*scale;
        }
    }

    TIFFWriteEncodedStrip(output_image, 0, dbuf, sizeof(uint16_t) * tilesize * x_tile_count * tilesize * y_tile_count);
    TIFFClose(output_image);
    return 0;
}
