#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <inttypes.h>
#include <float.h>
#include "tiffio.h"

const int m_width = 1024;
const int m_height = 1024;

int main(int argc, char* argv[]) {
    TIFF *output_image;
    int m_height = -1;
    int m_width  = -1;
    double cell_size = -1;
    double nodata = -1;
    int xllcorner;
    int yllcorner;
    
    assert(scanf("ncols %d\n", &m_width));
    assert(scanf("nrows %d\n", &m_height));
    assert(scanf("xllcorner %d\n", &xllcorner));
    assert(scanf("yllcorner %d\n", &yllcorner));
    assert(scanf("cellsize %lf\n", &cell_size));
    assert(scanf("NODATA_value %lf\n", &nodata));

    // Open the TIFF file
    if((output_image = TIFFOpen("foo.tiff", "w")) == NULL){
        assert(0);
    }
 
    float *ibuf = malloc(sizeof(float)*m_width*m_height);
    assert(ibuf != NULL);

    float min = FLT_MAX;
    float max = -FLT_MAX;

    for (int y=0; y<m_height; y++) {
        for (int x=0; x<m_width; x++) {
            if (x == (m_width-1)) {
                assert(scanf("%f\n", &ibuf[x+y*m_width]));
            }
            else {
                assert(scanf("%f ", &ibuf[x+y*m_width]));
            }
            if (ibuf[x+y*m_width]>max) max = ibuf[x+y*m_width];
            if (ibuf[x+y*m_width]<min) min = ibuf[x+y*m_width];
        }
    }

    printf("%f %f\n", min, max);

    uint16_t *dbuf = malloc(sizeof(uint16_t)*m_width*m_height);
    assert(dbuf != NULL);

    for (int y=0; y<m_height; y++) {
        for (int x=0; x<m_width; x++) {
            dbuf[x+y*m_width] = (uint16_t) (ibuf[x+y*m_width]-min)*(65535.0/(max-min));
        }
    }

    free(ibuf);
    ibuf = NULL;

    // We need to set some values for basic tags before we can add any data
    TIFFSetField(output_image, TIFFTAG_IMAGEWIDTH, m_width);
    TIFFSetField(output_image, TIFFTAG_IMAGELENGTH, m_height);
    TIFFSetField(output_image, TIFFTAG_ROWSPERSTRIP, m_height);
    TIFFSetField(output_image, TIFFTAG_BITSPERSAMPLE, 16);
    TIFFSetField(output_image, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(output_image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
 
    TIFFSetField(output_image, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
 //   TIFFSetField(output_image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
 
    // Write the information to the file
    TIFFWriteEncodedStrip(output_image, 0, dbuf, m_width*m_height * sizeof(uint16_t));
 
    // Close the file
    TIFFClose(output_image);
}
