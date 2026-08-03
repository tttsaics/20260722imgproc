#include "imgproc_channel.h" // 引入通道標頭檔
#include <stdint.h>          // 引入整數型態定義

extern "C" { // 以 C 連結規範匯出

void imgproc_image_keep_channel(ImgProcImage* image, ImgProcChannel keep_ch) { // 保留指定圖像顏色通道
    if (!image || !image->data) return; // 傳入指標空值檢查
    if (image->channels != 4) return;   // 僅支援 4 通道 (RGBA) 格式

    uint8_t* pixels = (uint8_t*)image->data; // 轉型為 8-bit 位元組指標
    for (uint32_t y = 0; y < image->height; ++y) { // 逐列遍歷 (Y 軸)
        uint8_t* row = pixels + y * image->stride; // 計算當前列首位址
        for (uint32_t x = 0; x < image->width; ++x) { // 逐欄遍歷 (X 軸)
            uint8_t* pixel = row + x * 4; // 計算當前像素 4-byte (RGBA) 起始位址 
            if (keep_ch != IMGPROC_CHANNEL_R) pixel[0] = 0; // 若不保留 R 通道則設為 0
            if (keep_ch != IMGPROC_CHANNEL_G) pixel[1] = 0; // 若不保留 G 通道則設為 0
            if (keep_ch != IMGPROC_CHANNEL_B) pixel[2] = 0; // 若不保留 B 通道則設為 0
        } 
    } 
} 

} 

