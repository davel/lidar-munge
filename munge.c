#define _GNU_SOURCE
#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <inttypes.h>
#include <float.h>
#include <strings.h>
#include "tiffio.h"
#include <stdbool.h>
#include <string.h>

const int easting_start = 17;
const int northing_start = 50;
const int easting_count =2;
const int northing_count =2;

int main(int argc, char* argv[]) {
    float *tbuf = NULL;
    float *sbuf = NULL;
    bool   *mbuf = NULL;
    int     image_width=0;
    int     image_length=0;
    float  min =  DBL_MAX;
    float  max = -DBL_MAX;

    int tilesize = 0;

    for (int easting = easting_start; easting < easting_start+easting_count; easting++) {
        for (int northing = northing_start; northing < northing_start+northing_count; northing++) {
            for (int chan = 0; chan < 2; chan++) {
                char *filename;
                if (chan==0) {
                    assert(asprintf(&filename, "/home/davel/Downloads/lidar/tq%02d%02d_DSM_50cm.asc", easting, northing));
                }
                else {
                    assert(asprintf(&filename, "/home/davel/Downloads/lidar/tq%02d%02d_DTM_50cm.asc", easting, northing));
                }
                assert(filename);

                printf("%s\n", filename);
                FILE *asc = fopen(filename, "r");

                if (asc != NULL) {
                    assert(asc != NULL);

                    int m_height = -1;
                    int m_width  = -1;
                    float cell_size = -1;
                    float nodata = -1;
                    int xllcorner;
                    int yllcorner;

                    assert(fscanf(asc, "ncols %d\n", &m_width));
                    assert(fscanf(asc, "nrows %d\n", &m_height));
                    assert(fscanf(asc, "xllcorner %d\n", &xllcorner));
                    assert(fscanf(asc, "yllcorner %d\n", &yllcorner));
                    assert(fscanf(asc, "cellsize %f\n", &cell_size));
                    assert(fscanf(asc, "NODATA_value %f\n", &nodata));

                    if (tilesize == 0) {
                        tilesize = m_width;
                        image_width  = tilesize * easting_count;
                        image_length = tilesize * northing_count;
                        assert(image_width > 0);
                        assert(image_length > 0);
                        tbuf = malloc(sizeof(float) * image_width * image_length);
                        assert(tbuf != NULL);
                        sbuf = malloc(sizeof(float) * image_width * image_length);
                        assert(sbuf != NULL);
                        mbuf = malloc(sizeof(bool)   * image_width * image_length);
                        assert(mbuf != NULL);
                        memset(mbuf, true, sizeof(bool)*image_width * image_length);
                    }

                    assert(tilesize > 0);
                    assert(m_width  == tilesize);
                    assert(m_height == tilesize);

                    for (int row=0; row<m_height; row++) {
                        for (int col=0; col<m_width; col++) {
                            int easting_full  = tilesize * (easting-easting_start) + col;
                            int northing_full = tilesize * (northing-northing_start+1)-row-1;
                            float *f = chan == 0 ? &sbuf[northing_full * image_width + easting_full]
                                                 : &tbuf[northing_full * image_width + easting_full];
                            bool *m   = &mbuf[northing_full * image_width + easting_full];
                            if (col == (m_width-1)) {
                                assert(fscanf(asc, "%f\n", f));
                            }
                            else {
                                assert(fscanf(asc, "%f ", f));
                            }
                            if (*f == nodata) {
                                *f = 0;
                                *m = false;
                            }
                            else {
                                if (*f < min) min = *f;
                                if (*f > max) max = *f;
                            }
                        }
                    }
                    fclose(asc);
                }
                else {
                    printf("No data for %s\n", filename);
                    for (int row=0; row<tilesize; row++) {
                        for (int col=0; col<tilesize; col++) {
                            int easting_full  = tilesize * (easting-easting_start) + col;
                            int northing_full = tilesize * (northing-northing_start+1)-row-1;
                            mbuf[northing_full * image_width + easting_full] = false;
                        }
                    }
                }
                free(filename);
            }
        }
    }

   TIFF *output_image;
    // Open the TIFF file
    if((output_image = TIFFOpen("foo.tiff", "w")) == NULL){
        assert(0);
    }
    assert(TIFFSetField(output_image, TIFFTAG_IMAGEWIDTH, image_width)==1);
    assert(TIFFSetField(output_image, TIFFTAG_IMAGELENGTH, image_length)==1);
    assert(TIFFSetField(output_image, TIFFTAG_BITSPERSAMPLE, 32)==1);
    assert(TIFFSetField(output_image, TIFFTAG_SAMPLESPERPIXEL, 3)==1);
    assert(TIFFSetField(output_image, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP)==1);
    assert(TIFFSetField(output_image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB)==1);
    assert(TIFFSetField(output_image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG)==1);
    assert(TIFFSetField(output_image, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT)==1);
    assert(TIFFSetField(output_image, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE)==1);

    float scale = 1.0/(max-min);

    printf("%lf %lf %lf\n", scale, min, max);

    float *dbuf = malloc(sizeof(float)*image_width*3);
    assert(dbuf != NULL);

    for (int y=0; y<image_length; y++) {
        int northing = image_length - y - 1;
        for (int x=0; x<image_width; x++) {
            if (mbuf[x+northing*image_width]) {
                float tscaled = (tbuf[x+northing*image_width] - min)*scale;
                float sscaled = (sbuf[x+northing*image_width] - min)*scale;

                dbuf[x*3+0] = sscaled;
                dbuf[x*3+1] = sscaled;
                dbuf[x*3+2] = tscaled;
            }
            else {
                dbuf[x*3+0] = -1.0;
                dbuf[x*3+1] = -1.0;
                dbuf[x*3+2] = 0.5;
            }
        }
        assert(TIFFWriteScanline(output_image, dbuf, y, 0)==1);
    }

    free(sbuf);
    free(tbuf);
    free(mbuf);
    free(dbuf);
    TIFFClose(output_image);
    return 0;
}
