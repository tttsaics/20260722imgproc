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
    if (!filter_handle) {       // 檢查輸出指標是否為空值
        IMGPROC_LOG_ERROR("'filter_handle' is null."); // 記錄錯誤日誌
        return IMGPROC_ERROR_INVALID_ARG;              // 回傳無效引數錯誤
    }

    GrayscaleFilter* filter = new (std::nothrow) GrayscaleFilter; // 配置濾鏡物件記憶體 (不拋出例外)
    if (!filter) {                                                // 檢查記憶體配置是否失敗
        IMGPROC_LOG_ERROR("Failed to allocate memory for GrayscaleFilter."); // 記錄記憶體分配失敗日誌
        return IMGPROC_ERROR_OUT_OF_MEMOERY;                                 // 回傳記憶體不足錯誤
    }

    *filter_handle = static_cast<ImgProcFilterHandle>(filter); // 將內部結構指標轉型並填入輸出控制代碼
    IMGPROC_LOG_INFO("GrayscaleFilter created successfully.");  // 記錄成功建立日誌
    return IMGPROC_SUCCESS;                                    // 回傳成功狀態
}

// 銷毀灰階濾鏡實例
ImgProcStatus grayscale_filter_destroy(ImgProcFilterHandle filter_handle) {
    if (filter_handle) {                                        // 檢查濾鏡控制代碼是否有效
        delete static_cast<GrayscaleFilter*>(filter_handle);   // 轉型為實體型別並釋放記憶體
        IMGPROC_LOG_INFO("GrayscaleFilter destroyed.");        // 記錄銷毀成功日誌
    }
    return IMGPROC_SUCCESS;                                    // 回傳成功狀態
}

// 執行影像灰階轉換處理
ImgProcStatus grayscale_filter_transform(ImgProcFilterHandle filter_handle, ImgProcImage* input, ImgProcImage** output) {
    if (!filter_handle) {                                      // 檢查濾鏡控制代碼是否為空值
        IMGPROC_LOG_ERROR("'filter_handle' is null.");         // 記錄錯誤日誌
        return IMGPROC_ERROR_INVALID_ARG;                      // 回傳無效引數錯誤
    }
    if (!input || !input->data) {                              // 檢查輸入影像及影像資料首位址是否為空值
        IMGPROC_LOG_ERROR("Input image or image data is null."); // 記錄錯誤日誌
        return IMGPROC_ERROR_INVALID_ARG;                      // 回傳無效引數錯誤
    }
    if (input->channels != 4) {                                // 檢查影像通道數是否為 4 (RGBA)
        IMGPROC_LOG_ERROR("GrayscaleFilter requires 4-channel RGBA image, got %u.", input->channels); // 記錄通道不合錯誤日誌
        return IMGPROC_ERROR_INVALID_ARG;                      // 回傳無效引數錯誤
    }
    if (!output) {                                             // 檢查輸出影像指標位址是否為空值
        IMGPROC_LOG_ERROR("'output' pointer is null.");        // 記錄錯誤日誌
        return IMGPROC_ERROR_INVALID_ARG;                      // 回傳無效引數錯誤
    }

    uint32_t bpp = 4;                                          // 設定每個像素的位元組數 (Bytes Per Pixel, 4 bytes for RGBA)

    ImgProcImage* out_img = new (std::nothrow) ImgProcImage;   // 配置輸出影像結構體記憶體
    if (!out_img) {                                            // 檢查結構體配置是否成功
        IMGPROC_LOG_ERROR("Failed to allocate ImgProcImage structure."); // 記錄記憶體分配失敗日誌
        return IMGPROC_ERROR_OUT_OF_MEMOERY;                   // 回傳記憶體不足錯誤
    }

    out_img->data = malloc(input->data_size);                  // 為輸出影像配置像素資料陣列記憶體
    if (!out_img->data) {                                      // 檢查像素資料記憶體配置是否成功
        delete out_img;                                        // 釋放先前已配置的結構體記憶體
        IMGPROC_LOG_ERROR("Failed to allocate memory for grayscale image data."); // 記錄記憶體分配失敗日誌
        return IMGPROC_ERROR_OUT_OF_MEMOERY;                   // 回傳記憶體不足錯誤
    }

    out_img->width = input->width;                             // 複製影像寬度資訊
    out_img->height = input->height;                           // 複製影像高度資訊
    out_img->stride = input->stride;                           // 複製影像每行跨度 (stride) 資訊
    out_img->channels = bpp;                                   // 設定輸出影像通道數為 4
    out_img->data_size = input->data_size;                     // 複製影像資料總位元組大小
    out_img->release_fn = [](ImgProcImage* img) {              // 設定輸出影像自訂釋放函式 (Lambda)
        if (img) {                                             // 檢查影像物件是否非空
            if (img->data) {                                   // 檢查影像像素資料指標是否非空
                free(img->data);                               // 釋放像素資料記憶體
                img->data = nullptr;                           // 將像素資料指標歸零
            }
        }
    };

    const uint8_t* raw_data = static_cast<const uint8_t*>(input->data); // 將輸入像素資料轉型為 8-bit 指標
    uint8_t* out_data = static_cast<uint8_t*>(out_img->data);          // 將輸出像素資料轉型為 8-bit 指標

    for (uint32_t y = 0; y < input->height; ++y) {                      // 逐列遍歷影像 (Y 軸)
        const uint8_t* row = raw_data + (y * input->stride);           // 計算當前輸入列首位址
        uint8_t* out_row = out_data + (y * input->stride);             // 計算當前輸出列首位址
        for (uint32_t x = 0; x < input->width; ++x) {                   // 逐欄遍歷影像像素 (X 軸)
            const uint8_t* pixel = row + (x * bpp);                    // 取得當前輸入像素 (RGBA) 位址
            uint8_t* out_pixel = out_row + (x * bpp);                  // 取得當前輸出像素 (RGBA) 位址
            uint8_t r = pixel[0];                                      // 讀取紅通道 (R) 值
            uint8_t g = pixel[1];                                      // 讀取綠通道 (G) 值
            uint8_t b = pixel[2];                                      // 讀取藍通道 (B) 值

            uint8_t gray = static_cast<uint8_t>(0.299 * r + 0.587 * g + 0.114 * b); // 依亮度加權公式計算灰階值 (Y = 0.299R + 0.587G + 0.114B)

            out_pixel[0] = gray;                                       // 將灰階值填入輸出 R 通道
            out_pixel[1] = gray;                                       // 將灰階值填入輸出 G 通道
            out_pixel[2] = gray;                                       // 將灰階值填入輸出 B 通道
            for (uint32_t c = 3; c < bpp; ++c) {                       // 處理其餘通道 (如 Alpha 通道)
                out_pixel[c] = pixel[c];                               // 保留原始 Alpha 透明度數值
            }
        }
    }

    if (input->release_fn) {                                           // 檢查輸入影像是否設有釋放回调函式
        input->release_fn(input);                                      // 呼叫釋放函式清理輸入影像
    }

    *output = out_img;                                                 // 將建立好的輸出影像物件寫入輸出指標
    IMGPROC_LOG_INFO("Grayscale transformation complete.");             // 記錄灰階轉換完成日誌
    return IMGPROC_SUCCESS;                                            // 回傳成功狀態
}
