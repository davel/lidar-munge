#define _GNU_SOURCE
#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <inttypes.h>
#include <float.h>
#include <strings.h>
#include "tiffio.h"

const int tilesize = 2000;

const int x_tile_start = 22;
const int y_tile_start = 40;
const int x_tile_count =10;
const int y_tile_count =11;

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
    TIFFSetField(output_image, TIFFTAG_TILEWIDTH, tilesize);
    TIFFSetField(output_image, TIFFTAG_TILELENGTH, tilesize);
    TIFFSetField(output_image, TIFFTAG_BITSPERSAMPLE, 16);
    TIFFSetField(output_image, TIFFTAG_SAMPLESPERPIXEL, 1);
//    TIFFSetField(output_image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
 
    TIFFSetField(output_image, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);

    uint16_t *dbuf = malloc(sizeof(uint16_t)*tilesize*tilesize);
    assert(dbuf != NULL);

    for (int tilex = x_tile_start; tilex < x_tile_start+x_tile_count; tilex++) {
        for (int tiley = y_tile_start; tiley < y_tile_start+y_tile_count; tiley++) {
            char *filename;
            assert(asprintf(&filename, "/home/davel/Downloads/lidar/tq%02d%02d_DSM_50cm.asc", tilex, tiley));
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
                        double f;
                        if (x == (m_width-1)) {
                            assert(fscanf(asc, "%lf\n", &f));
                        }
                        else {
                            assert(fscanf(asc, "%lf ", &f));
                        }
                        if (f == nodata) {
                            dbuf[x+m_width*y] = 0;
                        }
                        else {
                            float scaled = (f+100.0)*20.0;
                            assert(scaled>=0);
                            assert(scaled<65536);
                            dbuf[x+m_width*y] = (uint16_t) scaled;
                        }
                    }
                }
            }
            else {
                bzero(dbuf, sizeof(uint16_t) * m_width * m_height);
            }

            // Write the information to the file
            assert(TIFFWriteTile(output_image, dbuf, tilex - x_tile_start, tiley - y_tile_start, 0, 0));
            // Close the file
        }
    }

    TIFFClose(output_image);
    return 0;
}
