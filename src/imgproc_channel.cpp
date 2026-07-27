#include "imgproc_channel.h"
#include <stdint.h>

extern "C" {

void imgproc_image_keep_channel(ImgProcImage* image, ImgProcChannel keep_ch) {
    if (!image || !image->data) return;
    
    uint32_t bpp = (image->width > 0) ? (image->stride / image->width) : 3;
    uint8_t* pixels = (uint8_t*)image->data;
    for (uint32_t y = 0; y < image->height; ++y) {
        uint8_t* row = pixels + y * image->stride;
        for (uint32_t x = 0; x < image->width; ++x) {
            uint8_t* pixel = row + x * bpp;
            if (keep_ch != IMGPROC_CHANNEL_R) pixel[0] = 0; 
            if (keep_ch != IMGPROC_CHANNEL_G) pixel[1] = 0; 
            if (keep_ch != IMGPROC_CHANNEL_B) pixel[2] = 0; 
        }
    }
}

}
