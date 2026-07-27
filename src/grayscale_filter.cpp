#include <imgproc_filter_common.h>
#include <imgproc_logger.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <new>

#ifdef __cplusplus
extern "C" {
#endif

struct GrayscaleFilter {};//定義GrayscaleFilter結構體，這個結構體目前沒有任何成員，僅用於表示灰階濾鏡的實例。

ImgProcStatus grayscale_filter_create(ImgProcFilterHandle* filter_handle, ImgProcConfigHandle filter_config_handle);//定義函式grayscale_filter_create，該函式用於{創建灰階濾鏡}的實例，並將其指針存儲在filter_handle中。filter_config_handle參數目前未使用。
ImgProcStatus grayscale_filter_destroy(ImgProcFilterHandle filter_handle);
ImgProcStatus grayscale_filter_transform(ImgProcFilterHandle filter_handle, ImgProcImage* input, ImgProcImage** output);//定義函式grayscale_filter_transform，該函式用於將輸入{圖像轉換}為灰階圖像，並將結果存儲在output中。

#ifdef __cplusplus
}
#endif

IMGPROC_FILTER_DECLARE_CREATE_FN(grayscale_filter_create)//定義函式指標grayscale_filter_create_fn，該指標指向函式grayscale_filter_create，用於創建灰階濾鏡的實例。
IMGPROC_FILTER_DECLARE_DESTROY_FN(grayscale_filter_destroy)
IMGPROC_FILTER_DECLARE_TRANSFORM_FN(grayscale_filter_transform)

//濾鏡建立函式
ImgProcStatus grayscale_filter_create(ImgProcFilterHandle* filter_handle, ImgProcConfigHandle filter_config_handle) {//定義函式grayscale_filter_create，該函式用於創建灰階濾鏡的實例，並將其指針存儲在filter_handle中。filter_config_handle參數目前未使用。
    (void)filter_config_handle;//將filter_config_handle標記為未使用，以避免編譯器警告。(確保函式簽名與接口一致，即使目前未使用該參數。)
    if (!filter_handle) {
        IMGPROC_LOG_ERROR("'filter_handle' is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }

    GrayscaleFilter* filter = new (std::nothrow) GrayscaleFilter;//使用new運算子創建GrayscaleFilter的實例，並使用std::nothrow確保在內存不足時不會拋出異常，而是返回nullptr。
    if (!filter) {
        IMGPROC_LOG_ERROR("Failed to allocate memory for GrayscaleFilter.");
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }

    *filter_handle = static_cast<ImgProcFilterHandle>(filter);//將創建的GrayscaleFilter實例的指針轉換為ImgProcFilterHandle類型，並存儲在filter_handle中，以便外部代碼可以使用該handle來操作灰階濾鏡。
    IMGPROC_LOG_INFO("GrayscaleFilter created successfully.");
    return IMGPROC_SUCCESS;
}

//濾鏡銷毀函式
ImgProcStatus grayscale_filter_destroy(ImgProcFilterHandle filter_handle) {
    if (filter_handle) {
        delete static_cast<GrayscaleFilter*>(filter_handle);
        IMGPROC_LOG_INFO("GrayscaleFilter destroyed.");
    }
    return IMGPROC_SUCCESS;
}

//濾鏡轉換函式
ImgProcStatus grayscale_filter_transform(ImgProcFilterHandle filter_handle, ImgProcImage* input, ImgProcImage** output) {
    if (!filter_handle) {
        IMGPROC_LOG_ERROR("'filter_handle' is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }
    if (!input || !input->data) {
        IMGPROC_LOG_ERROR("Input image or image data is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }
    if (!output) {
        IMGPROC_LOG_ERROR("'output' pointer is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }

    uint32_t bpp = (input->width > 0) ? (input->stride / input->width) : 0;//計算每個像素的字節數（bytes per pixel, bpp），通過將圖像的stride（每行的字節數）除以圖像的寬度來獲得。如果圖像寬度為0，則bpp設置為0，以避免除以零的錯誤。
    if (bpp < 3) {//檢查每個像素的字節數是否小於3，這意味著圖像格式不支持灰階轉換（例如，灰階圖像通常需要至少3個通道：紅、綠、藍）。如果bpp小於3，則記錄錯誤並返回IMGPROC_ERROR_INVALID_ARG狀態碼。
        IMGPROC_LOG_ERROR("Unsupported image format: bytes per pixel must be >= 3.");
        return IMGPROC_ERROR_INVALID_ARG;
    }

    ImgProcImage* out_img = new (std::nothrow) ImgProcImage;//使用new運算子創建一個新的ImgProcImage結構體實例，用於存儲灰階圖像的結果。使用std::nothrow確保在內存不足時不會拋出異常，而是返回nullptr。
    if (!out_img) {
        IMGPROC_LOG_ERROR("Failed to allocate ImgProcImage structure.");
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }

    out_img->data = malloc(input->data_size);//為灰階圖像分配內存，大小與輸入圖像的數據大小相同。這確保了灰階圖像有足夠的空間來存儲轉換後的像素數據。
    if (!out_img->data) {
        delete out_img;
        IMGPROC_LOG_ERROR("Failed to allocate memory for grayscale image data.");
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }

    out_img->width = input->width;
    out_img->height = input->height;
    out_img->stride = input->stride;
    out_img->data_size = input->data_size;
    out_img->release_fn = [](ImgProcImage* img) {//定義一個lambda函式，用於釋放灰階圖像的資源。這個函式會在需要釋放圖像時被調用。
        if (img) {
            if (img->data) {
                free(img->data);
                img->data = nullptr;
            }
        }
    };

    const uint8_t* raw_data = static_cast<const uint8_t*>(input->data);//將輸入圖像的數據指針轉換為const uint8_t*類型，以便按字節訪問圖像數據。這樣可以方便地對每個像素進行操作，因為每個像素的顏色通道通常以字節形式存儲。
    uint8_t* out_data = static_cast<uint8_t*>(out_img->data);//將輸出圖像的數據指針轉換為uint8_t*類型，以便按字節訪問和修改灰階圖像的數據。這樣可以方便地將計算出的灰階值寫入到輸出圖像中。

    for (uint32_t y = 0; y < input->height; ++y) {//遍歷輸入圖像的每一行（從0到height-1），以便對每個像素進行灰階轉換。這個外層循環控制行的索引y。
        const uint8_t* row = raw_data + (y * input->stride);//計算當前行的起始地址，通過將行索引y乘以圖像的stride（每行的字節數）來獲得。這樣可以正確地定位到每一行的像素數據，因為stride可能大於width * bpp，特別是在圖像有填充或對齊要求時。
        uint8_t* out_row = out_data + (y * input->stride);//計算輸出圖像當前行的起始地址，與輸入圖像類似，通過將行索引y乘以stride來獲得。這樣可以正確地將灰階值寫入到對應的輸出圖像行中。
        for (uint32_t x = 0; x < input->width; ++x) {//遍歷當前行的每一個像素（從0到width-1），以便對每個像素進行灰階轉換。這個內層循環控制列的索引x。
            const uint8_t* pixel = row + (x * bpp);//計算當前像素的起始地址，通過將列索引x乘以每個像素的字節數bpp來獲得。這樣可以正確地定位到每個像素的顏色通道數據，因為每個像素可能包含多個字節（例如RGB圖像通常有3個字節）。
            uint8_t* out_pixel = out_row + (x * bpp);//計算輸出圖像當前像素的起始地址，與輸入圖像類似，通過將列索引x乘以每個像素的字節數bpp來獲得。這樣可以正確地將灰階值寫入到對應的輸出圖像像素中。
            uint8_t r = pixel[0];
            uint8_t g = pixel[1];
            uint8_t b = pixel[2];

            uint8_t gray = static_cast<uint8_t>(0.299 * r + 0.587 * g + 0.114 * b);

            out_pixel[0] = gray;
            out_pixel[1] = gray;
            out_pixel[2] = gray;
            for (uint32_t c = 3; c < bpp; ++c) {//如果每個像素的字節數大於3，則將額外的通道（例如alpha通道）保持不變，直接從輸入圖像複製到輸出圖像。這樣可以確保灰階轉換不會影響其他通道的數據。
                out_pixel[c] = pixel[c];
            }
        }
    }

    if (input->release_fn) {
        input->release_fn(input);
    }

    *output = out_img;
    IMGPROC_LOG_INFO("Grayscale transformation complete.");
    return IMGPROC_SUCCESS;
}
