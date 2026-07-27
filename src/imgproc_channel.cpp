#include "imgproc_channel.h"
#include <stdint.h>//引入stdint.h以使用uint8_t、uint32_t等固定大小的整數類型

extern "C" {

void imgproc_image_keep_channel(ImgProcImage* image, ImgProcChannel keep_ch) {// 保留指定的通道(顏色)，將其他通道設為0
    if (!image || !image->data) return;// 如果圖像或圖像數據為空則返回
    
    uint8_t* pixels = (uint8_t*)image->data;// 將圖像數據轉換為uint8_t指針，以便按字節訪問像素數據(因為 RGB 的每個像素通道數值都是 1 字節(0~255)，這樣才能精確進行 byte 級別的指標位移運算。 )
    for (uint32_t y = 0; y < image->height; ++y) {
        for (uint32_t x = 0; x < image->width; ++x) {
            size_t idx = (size_t)y * image->stride + (size_t)x * 3;// 計算當前像素的索引位置，快速跨過前面y行的所有資料，跳轉到第y行的起始位址，stride 是每行的字節數，乘以 3 是因為每個像素有 3 個通道(RGB)
            if (keep_ch != IMGPROC_CHANNEL_R) pixels[idx + 0] = 0; 
            if (keep_ch != IMGPROC_CHANNEL_G) pixels[idx + 1] = 0; 
            if (keep_ch != IMGPROC_CHANNEL_B) pixels[idx + 2] = 0; 
        }
    }
}

}
