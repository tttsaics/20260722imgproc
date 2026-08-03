#include <imgproc_filter_common.h> 
#include <imgproc_logger.h>        

#include <algorithm>               
#include <cmath>                   
#include <cstdlib>          
#include <new>                  

#ifdef __cplusplus
extern "C" {               
#endif

struct GrayscaleFilter {           // 定義灰階濾鏡內部狀態結構
    bool use_weighted = true;      // 註記是否使用權重演算法 (預設為 true)
};

ImgProcStatus grayscale_filter_create(ImgProcFilterHandle* filter_handle, ImgProcConfigHandle filter_config_handle); // 宣告創建濾鏡控制代碼函式
ImgProcStatus grayscale_filter_destroy(ImgProcFilterHandle filter_handle);                                           // 宣告銷毀濾鏡控制代碼函式
ImgProcStatus grayscale_filter_transform(ImgProcFilterHandle filter_handle, ImgProcImage* input, ImgProcImage** output); // 宣告執行灰階轉換函式

#ifdef __cplusplus
}
#endif

IMGPROC_FILTER_DECLARE_CREATE_FN(grayscale_filter_create)       // 註冊濾鏡創建入口函式
IMGPROC_FILTER_DECLARE_DESTROY_FN(grayscale_filter_destroy)     // 註冊濾鏡銷毀入口函式
IMGPROC_FILTER_DECLARE_TRANSFORM_FN(grayscale_filter_transform) // 註冊濾鏡轉換入口函式

// 建立灰階濾鏡實例
ImgProcStatus grayscale_filter_create(ImgProcFilterHandle* filter_handle, ImgProcConfigHandle filter_config_handle) {
    (void)filter_config_handle; // 標記未使用的設定參數，避免編譯器發出警告
    if (!filter_handle) {       
        IMGPROC_LOG_ERROR("'filter_handle' is null."); 
        return IMGPROC_ERROR_INVALID_ARG;           
    }

    GrayscaleFilter* filter = new (std::nothrow) GrayscaleFilter; 
    if (!filter) {                                              
        IMGPROC_LOG_ERROR("Failed to allocate memory for GrayscaleFilter."); 
        return IMGPROC_ERROR_OUT_OF_MEMOERY;                                 
    }

    *filter_handle = static_cast<ImgProcFilterHandle>(filter); 
    IMGPROC_LOG_INFO("GrayscaleFilter created successfully.");  
    return IMGPROC_SUCCESS;                                    
}

// 銷毀灰階濾鏡實例
ImgProcStatus grayscale_filter_destroy(ImgProcFilterHandle filter_handle) {
    if (filter_handle) {                                     
        delete static_cast<GrayscaleFilter*>(filter_handle);   
        IMGPROC_LOG_INFO("GrayscaleFilter destroyed.");        
    }
    return IMGPROC_SUCCESS;                                    
}

// 執行影像灰階轉換處理
ImgProcStatus grayscale_filter_transform(ImgProcFilterHandle filter_handle, ImgProcImage* input, ImgProcImage** output) {
    if (!filter_handle) {                                      
        IMGPROC_LOG_ERROR("'filter_handle' is null.");        
        return IMGPROC_ERROR_INVALID_ARG;                      
    }
    if (!input || !input->data) {                              
        IMGPROC_LOG_ERROR("Input image or image data is null."); 
        return IMGPROC_ERROR_INVALID_ARG;                    
    }
    if (input->channels != 4) {                                // 檢查影像通道數是否為 4 (RGBA)
        IMGPROC_LOG_ERROR("GrayscaleFilter requires 4-channel RGBA image, got %u.", input->channels); 
        return IMGPROC_ERROR_INVALID_ARG;                
    }
    if (!output) {                                             
        IMGPROC_LOG_ERROR("'output' pointer is null.");        
        return IMGPROC_ERROR_INVALID_ARG;                     
    }

    uint32_t bpp = 4;                                          // 設定每個像素的位元組數 (Bytes Per Pixel, 4 bytes for RGBA)

    ImgProcImage* out_img = new (std::nothrow) ImgProcImage;   // 配置輸出影像結構體記憶體
    if (!out_img) {                                            
        IMGPROC_LOG_ERROR("Failed to allocate ImgProcImage structure."); 
        return IMGPROC_ERROR_OUT_OF_MEMOERY;                  
    }

    out_img->data = malloc(input->data_size);                  // 為輸出影像配置像素資料陣列記憶體
    if (!out_img->data) {                                  
        delete out_img;                                
        IMGPROC_LOG_ERROR("Failed to allocate memory for grayscale image data."); 
        return IMGPROC_ERROR_OUT_OF_MEMOERY;                
    }

    out_img->width = input->width;                         
    out_img->height = input->height;                           
    out_img->stride = input->stride;                          
    out_img->channels = bpp;                                
    out_img->data_size = input->data_size;                    
    out_img->release_fn = [](ImgProcImage* img) {              // 設定輸出影像自訂釋放函式 (Lambda)
        if (img) {                                            
            if (img->data) {                                   
                free(img->data);                               
                img->data = nullptr;                           
            }
        }
    };

    const uint8_t* raw_data = static_cast<const uint8_t*>(input->data); // 將輸入像素資料轉型為 8-bit 指標
    uint8_t* out_data = static_cast<uint8_t*>(out_img->data);          // 將輸出像素資料轉型為 8-bit 指標

    for (uint32_t y = 0; y < input->height; ++y) {                      
        const uint8_t* row = raw_data + (y * input->stride);           
        uint8_t* out_row = out_data + (y * input->stride);             
        for (uint32_t x = 0; x < input->width; ++x) {                   
            const uint8_t* pixel = row + (x * bpp);                    // 取得當前輸入像素 (RGBA) 位址
            uint8_t* out_pixel = out_row + (x * bpp);                  // 取得當前輸出像素 (RGBA) 位址
            uint8_t r = pixel[0];                                      
            uint8_t g = pixel[1];                                    
            uint8_t b = pixel[2];                                  

            uint8_t gray = static_cast<uint8_t>(0.299 * r + 0.587 * g + 0.114 * b); // 依亮度加權公式計算灰階值 (Y = 0.299R + 0.587G + 0.114B)

            out_pixel[0] = gray;                                       
            out_pixel[1] = gray;                                       
            out_pixel[2] = gray;                                       
            for (uint32_t c = 3; c < bpp; ++c) {                       
                out_pixel[c] = pixel[c];                               
            }
        }
    }

    if (input->release_fn) {                                           // 檢查輸入影像是否設有釋放回调函式
        input->release_fn(input);                                      // 呼叫釋放函式清理輸入影像
    }

    *output = out_img;                                                 // 將建立好的輸出影像物件寫入輸出指標
    IMGPROC_LOG_INFO("Grayscale transformation complete.");             
    return IMGPROC_SUCCESS;
}                                          
