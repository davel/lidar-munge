#define _GNU_SOURCE
#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <inttypes.h>
#include <float.h>
#include <strings.h>
#include "tiffio.h"
#include <stdbool.h>

const int easting_start = 17;
const int northing_start = 50;
const int easting_count =2;
const int northing_count =2;

int main(int argc, char* argv[]) {
    double *fbuf = NULL;
    bool   *mbuf = NULL;
    int     image_width=0;
    int     image_length=0;
    double  min =  DBL_MAX;
    double  max = -DBL_MAX;

    int tilesize = 0;

    for (int easting = easting_start; easting < easting_start+easting_count; easting++) {
        for (int northing = northing_start; northing < northing_start+northing_count; northing++) {
            char *filename;
            assert(asprintf(&filename, "/home/davel/Downloads/lidar/tq%02d%02d_DSM_1m.asc", easting, northing));
            assert(filename);

            printf("%s\n", filename);
            FILE *asc = fopen(filename, "r");

            if (asc != NULL) {
                free(filename);
                assert(asc != NULL);

                int m_height = -1;
                int m_width  = -1;
                double cell_size = -1;
                double nodata = -1;
                int xllcorner;
                int yllcorner;

                assert(fscanf(asc, "ncols %d\n", &m_width));
                assert(fscanf(asc, "nrows %d\n", &m_height));
                assert(fscanf(asc, "xllcorner %d\n", &xllcorner));
                assert(fscanf(asc, "yllcorner %d\n", &yllcorner));
                assert(fscanf(asc, "cellsize %lf\n", &cell_size));
                assert(fscanf(asc, "NODATA_value %lf\n", &nodata));

                if (tilesize == 0) {
                    tilesize = m_width;
                    image_width = tilesize * easting_count;
                    image_length = tilesize * northing_count;
                    assert(image_width > 0);
                    assert(image_length > 0);
                    fbuf = malloc(sizeof(double) * image_width * image_length);
                    assert(fbuf != NULL);
                    mbuf = malloc(sizeof(bool)   * image_width * image_length);
                    assert(mbuf != NULL);
                }

                assert(tilesize > 0);
                assert(m_width == tilesize);
                assert(m_height == tilesize);

                for (int row=0; row<m_height; row++) {
                    for (int col=0; col<m_width; col++) {
                        int easting_full  = tilesize * (easting-easting_start) + col;
                        int northing_full = tilesize * (northing-northing_start+1)-row-1;
                        double *f = &fbuf[northing_full * image_width + easting_full];
                        bool *m = &mbuf[northing_full * image_width + easting_full];
                        if (col == (m_width-1)) {
                            assert(fscanf(asc, "%lf\n", f));
                        }
                        else {
                            assert(fscanf(asc, "%lf ", f));
                        }
                        if (*f == nodata) {
                            *f = 0;
                            *m = false;
                        }
                        else {
                            *m = true;
                            if (*f < min) min = *f;
                            if (*f > max) max = *f;
                        }
                    }
                }
                fclose(asc);
            }
            // Write the information to the file
            // Close the file
        }
    }

   TIFF *output_image;
    // Open the TIFF file
    if((output_image = TIFFOpen("foo.tiff", "w")) == NULL){
        assert(0);
    }
    assert(TIFFSetField(output_image, TIFFTAG_IMAGEWIDTH, image_width)==1);
    assert(TIFFSetField(output_image, TIFFTAG_IMAGELENGTH, image_length)==1);
    assert(TIFFSetField(output_image, TIFFTAG_BITSPERSAMPLE, 16)==1);
    assert(TIFFSetField(output_image, TIFFTAG_SAMPLESPERPIXEL, 2)==1);
    assert(TIFFSetField(output_image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK)==1);
    assert(TIFFSetField(output_image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG)==1);
    assert(TIFFSetField(output_image, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT)==1);
    assert(TIFFSetField(output_image, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE)==1);

    double scale = 65535.0/(max-min);

    printf("%lf %lf %lf\n", scale, min, max);

    uint16_t *dbuf = malloc(sizeof(uint16_t)*image_width*2);
    assert(dbuf != NULL);

    for (int y=0; y<image_length; y++) {
        int northing = image_length - y - 1;
        for (int x=0; x<image_width; x++) {
            if (mbuf[x+northing*image_width]) {
                int scaled = (fbuf[x+northing*image_width] - min)*scale;
                assert(scaled >= 0);
                assert(scaled <= 65535);
                dbuf[x*2] = (uint16_t) scaled;
                dbuf[x*2+1] = 65535;
            }
            else {
                dbuf[x*2] = 0;
                dbuf[x*2+1] = 0;
            }
        }
        assert(TIFFWriteScanline(output_image, dbuf, y, 0)==1);
    }

    free(fbuf);
    free(mbuf);
    TIFFClose(output_image);
    return 0;
}
