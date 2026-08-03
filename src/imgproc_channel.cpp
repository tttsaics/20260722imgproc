#include "imgproc_channel.h" 
#include <stdint.h>         

extern "C" { // 以 C 連結規範匯出

void imgproc_image_keep_channel(ImgProcImage* image, ImgProcChannel keep_ch) { // 保留指定圖像顏色通道
    if (!image || !image->data) return; 
    if (image->channels != 4) return;

    uint8_t* pixels = (uint8_t*)image->data; // 轉型為 8-bit 位元組指標
    for (uint32_t y = 0; y < image->height; ++y) { 
        uint8_t* row = pixels + y * image->stride; // 計算當前列首位址
        for (uint32_t x = 0; x < image->width; ++x) { // 走訪當前列中每個像素
            uint8_t* pixel = row + x * 4; // 計算當前像素 4-byte (RGBA) 起始位址 
            if (keep_ch != IMGPROC_CHANNEL_R) pixel[0] = 0; 
            if (keep_ch != IMGPROC_CHANNEL_G) pixel[1] = 0; 
            if (keep_ch != IMGPROC_CHANNEL_B) pixel[2] = 0; 
        } 
    } 
} 

} 

