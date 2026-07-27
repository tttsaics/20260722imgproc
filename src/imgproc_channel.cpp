#include "imgproc_channel.h"
#include <stdint.h>

extern "C" {

void imgproc_image_keep_channel(ImgProcImage* image, ImgProcChannel keep_ch) {
    if (!image || !image->data) return;
    
    uint8_t* pixels = (uint8_t*)image->data;
    for (uint32_t y = 0; y < image->height; ++y) {
        for (uint32_t x = 0; x < image->width; ++x) {
            size_t idx = (size_t)y * image->stride + (size_t)x * 3;
            if (keep_ch != IMGPROC_CHANNEL_R) pixels[idx + 0] = 0; 
            if (keep_ch != IMGPROC_CHANNEL_G) pixels[idx + 1] = 0; 
            if (keep_ch != IMGPROC_CHANNEL_B) pixels[idx + 2] = 0; 
        }
    }
}

}
