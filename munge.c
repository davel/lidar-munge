#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <inttypes.h>
#include "tiffio.h"

const int m_width = 1024;
const int m_height = 1024;

int main(int argc, char* argv[]) {
    TIFF *output_image;
    uint16_t *buf;

    // Open the TIFF file
    if((output_image = TIFFOpen("foo.tiff", "w")) == NULL){
        assert(0);
    }
 
    buf = malloc(sizeof(uint16_t)*m_width*m_height);
    assert(buf != NULL);

    for (int x=0; x<m_width; x++) {
        for (int y=0; y<m_height; y++) {
            buf[x+y*m_width] = (x*y) % 65535;
        }
    }

    // We need to set some values for basic tags before we can add any data
    TIFFSetField(output_image, TIFFTAG_IMAGEWIDTH, m_width);
    TIFFSetField(output_image, TIFFTAG_IMAGELENGTH, m_height);
//    TIFFSetField(output_image, TIFFTAG_ROWSPERSTRIP, m_height);
    TIFFSetField(output_image, TIFFTAG_BITSPERSAMPLE, 16);
    TIFFSetField(output_image, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(output_image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
 
    TIFFSetField(output_image, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
 //   TIFFSetField(output_image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
 
    // Write the information to the file
    TIFFWriteEncodedStrip(output_image, 0, buf, m_width*m_height * sizeof(uint16_t));
 
    // Close the file
    TIFFClose(output_image);
}
