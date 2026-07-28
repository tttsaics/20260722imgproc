#include <imgproc_filter_common.h>
#include <imgproc_logger.h>

#include <limits>
#include <new>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

struct ResizeFilter {
    double scale = 1.0;
    int32_t width = 0;
    int32_t height = 0;
    std::string method = "bilinear";//雙線性插值法:透過周圍四個鄰近的像素點來算出新位置的顏色或數值
};

ImgProcStatus resize_filter_create(ImgProcFilterHandle* filter_handle, ImgProcConfigHandle filter_config_handle);
ImgProcStatus resize_filter_destroy(ImgProcFilterHandle filter_handle);
ImgProcStatus resize_filter_transform(ImgProcFilterHandle filter_handle, ImgProcImage* input, ImgProcImage** output);

#ifdef __cplusplus
}
#endif

IMGPROC_FILTER_DECLARE_CREATE_FN(resize_filter_create)
IMGPROC_FILTER_DECLARE_DESTROY_FN(resize_filter_destroy)
IMGPROC_FILTER_DECLARE_TRANSFORM_FN(resize_filter_transform)

ImgProcStatus resize_filter_create(ImgProcFilterHandle* filter_handle, ImgProcConfigHandle filter_config_handle) {
    if (!filter_handle) {   //確認 filter_handle 是否為空指針
        IMGPROC_LOG_ERROR("'filter_handle' is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }
    //int64_t 避免有溢位風險
    double scale = 1.0;
    int64_t width = 0;
    int64_t height = 0;
    const char* method_str = nullptr;

    if (filter_config_handle) {
        IMGPROC_LOG_INFO("Loading resize filter configuration.");

        double temp_scale = 0.0;
        ImgProcStatus status = imgproc_config_get_double(filter_config_handle, "resize_filter", "scale", &temp_scale);
        if (status == IMGPROC_SUCCESS) {
            if (std::isfinite(temp_scale) && temp_scale > 0.0 && temp_scale <= 100.0) { //檢查 temp_scale 是否為有限且大於 0 的數值，並且不超過 100
                scale = temp_scale;
            } else {
                IMGPROC_LOG_WARN("Configured scale (%f) is invalid or non-finite. Using default (1.0).", temp_scale);
            }
        }

        int64_t temp_width = 0;
        status = imgproc_config_get_int64(filter_config_handle, "resize_filter", "width", &temp_width);
        if (status == IMGPROC_SUCCESS) {
            width = temp_width;
        }

        int64_t temp_height = 0;
        status = imgproc_config_get_int64(filter_config_handle, "resize_filter", "height", &temp_height);
        if (status == IMGPROC_SUCCESS) {
            height = temp_height;
        }

        status = imgproc_config_get_string(filter_config_handle, "resize_filter", "method", &method_str);
    } else {
        IMGPROC_LOG_INFO("Configuration file is not specified, using defaults.");
    }

    ResizeFilter* filter = new (std::nothrow) ResizeFilter;
    if (!filter) {
        IMGPROC_LOG_ERROR("Unable to allocate memory for 'ResizeFilter'.");
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }

    filter->scale = scale;
    filter->width = (width >= INT32_MIN && width <= INT32_MAX) ? static_cast<int32_t>(width) : 0;
    //將int64_t類型的width轉換為int32_t類型，並檢查是否在int32_t的範圍內，如果超出範圍則設置為0
    filter->height = (height >= INT32_MIN && height <= INT32_MAX) ? static_cast<int32_t>(height) : 0;
    if (method_str) {
        filter->method = method_str;
    } else {
        filter->method = "bilinear";
    }

    *filter_handle = static_cast<ImgProcFilterHandle>(filter);
    IMGPROC_LOG_INFO("ResizeFilter created. scale=%f, width=%d, height=%d, method=%s",
                     filter->scale, filter->width, filter->height, filter->method.c_str());

    return IMGPROC_SUCCESS;
}

ImgProcStatus resize_filter_destroy(ImgProcFilterHandle filter_handle) {
    ResizeFilter* filter = static_cast<ResizeFilter*>(filter_handle);   //將 filter_handle 轉換為 ResizeFilter* 類型的指針
    if (filter) {
        delete filter;
        IMGPROC_LOG_INFO("ResizeFilter destroyed.");
    }
    return IMGPROC_SUCCESS;
}

ImgProcStatus resize_filter_transform(ImgProcFilterHandle filter_handle, ImgProcImage* input, ImgProcImage** output) {
    if (!filter_handle) {
        IMGPROC_LOG_ERROR("'filter_handle' is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }
    if (!input || !input->data) {   
        IMGPROC_LOG_ERROR("'input' or input data is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }
    if (input->channels != 4) { 
        IMGPROC_LOG_ERROR("ResizeFilter requires 4-channel RGBA image, got %u.", input->channels);
        return IMGPROC_ERROR_INVALID_ARG;
    }
    if (!output) {  
        IMGPROC_LOG_ERROR("'output' is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }

    ResizeFilter* filter = static_cast<ResizeFilter*>(filter_handle);

    //將新尺寸預設原長和寬
    uint32_t new_width = input->width;
    uint32_t new_height = input->height;

    if (filter->width > 0 && filter->height > 0) {  //如果設定了寬和高，則使用設定的值
        new_width = static_cast<uint32_t>(filter->width);
        new_height = static_cast<uint32_t>(filter->height);
    } else if (std::isfinite(filter->scale) && filter->scale > 0.0) {   //如果設定了縮放比例，則使用縮放比例計算新尺寸
        //使用 std::round 進行四捨五入至最接近的整數像素，避免直接轉整數(無條件捨去)導致 1 像素的失真
        double calc_w = std::round(input->width * filter->scale);
        double calc_h = std::round(input->height * filter->scale);
        if (std::isfinite(calc_w) && std::isfinite(calc_h) && 
            calc_w >= 1.0 && calc_w <= 100000.0 && 
            calc_h >= 1.0 && calc_h <= 100000.0) {  //設定最大和最小像素，防止極端異常
            new_width = static_cast<uint32_t>(calc_w);
            new_height = static_cast<uint32_t>(calc_h);
        } else {
            IMGPROC_LOG_ERROR("Calculated scale dimensions (%f, %f) out of safe range [1, 100000].", calc_w, calc_h);
            return IMGPROC_ERROR_INVALID_ARG;
        }
    } else {
        IMGPROC_LOG_ERROR("Invalid scale value: %f. Must be a finite positive number.", filter->scale);
        return IMGPROC_ERROR_INVALID_ARG;
    }
    
    //至少有1x1像素，避免出現 0 尺寸的圖像
    if (new_width == 0) new_width = 1;
    if (new_height == 0) new_height = 1;

    uint32_t channels = 4;
    uint32_t new_stride = new_width * channels;
   
    new_stride = (new_stride + 3) & ~3; 
    //~3(取反) 最後兩個位元強制為 00。任何整數與 ~3 進行 &（AND）位元運算後，最低兩位都會被清零，結果保證能被 4 整除

    uint64_t total_bytes = static_cast<uint64_t>(new_stride) * new_height;
    //強制轉為64位元避免發生溢位
    if (new_width > 100000 || new_height > 100000 || total_bytes > 2000000000ULL) { //ULL:unsigned long long(無符號 64 位元整數常數)，避免溢位
        IMGPROC_LOG_ERROR("Resize target dimensions (%ux%u) or buffer size (%lu bytes) overflow limits.",
                          new_width, new_height, total_bytes);
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }

    size_t new_data_size = static_cast<size_t>(total_bytes);

    
    ImgProcImage* out_img = new (std::nothrow) ImgProcImage;
    if (!out_img) {
        IMGPROC_LOG_ERROR("Failed to allocate ImgProcImage structure.");
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }

    out_img->data = malloc(new_data_size);
    if (!out_img->data) {
        delete out_img;
        IMGPROC_LOG_ERROR("Failed to allocate memory for image data.");
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }

    out_img->width = new_width;
    out_img->height = new_height;
    out_img->stride = new_stride;
    out_img->channels = channels;//設定通道數為 4，對應 RGBA 圖像
    out_img->data_size = new_data_size;
    out_img->release_fn = [](ImgProcImage* img) {   //釋放圖像資源的函式
        if (img) {  
            if (img->data) {
                free(img->data);
                img->data = nullptr;
            }
        }
    };

    IMGPROC_LOG_INFO("Resizing image: %dx%d (stride %d) -> %dx%d (stride %d) using method %s", 
       
                     input->width, input->height, input->stride,
                     new_width, new_height, new_stride, filter->method.c_str());

    const uint8_t* src = static_cast<const uint8_t*>(input->data);
    uint8_t* dst = static_cast<uint8_t*>(out_img->data);

    if (filter->method == "nearest") {
        //最鄰近插值法:掃描縮放後的「新影像」每一個像素點，算出它對應回「原圖」的相對位置，並直接把距離最近的像素顏色拿過來用
        for (uint32_t y = 0; y < new_height; ++y) {
            uint32_t src_y = (y * input->height) / new_height;
            if (src_y >= input->height) src_y = input->height - 1;
            const uint8_t* src_row = src + src_y * input->stride;
            uint8_t* dst_row = dst + y * new_stride;
            for (uint32_t x = 0; x < new_width; ++x) {
                uint32_t src_x = (x * input->width) / new_width;
                if (src_x >= input->width) src_x = input->width - 1;
                for (uint32_t c = 0; c < channels; ++c) {
                    dst_row[x * channels + c] = src_row[src_x * channels + c];
                }
            }
        }
    } else {
        // Bilinear (雙線性插值法)線性插值則會抓取點周圍的 4 個相鄰像素，根據距離計算出權重後進行加權平均
        double scale_x = static_cast<double>(input->width) / new_width;
        double scale_y = static_cast<double>(input->height) / new_height;

        for (uint32_t y = 0; y < new_height; ++y) {
            double src_yf = (y + 0.5) * scale_y - 0.5;
            int32_t y0 = static_cast<int32_t>(std::floor(src_yf));
            int32_t y1 = y0 + 1;
            double dy = src_yf - y0;

            if (y0 < 0) { y0 = 0; y1 = 0; }
            if (y1 >= static_cast<int32_t>(input->height)) { y0 = input->height - 1; y1 = input->height - 1; }

            uint8_t* dst_row = dst + y * new_stride;

            for (uint32_t x = 0; x < new_width; ++x) {
                double src_xf = (x + 0.5) * scale_x - 0.5;
                int32_t x0 = static_cast<int32_t>(std::floor(src_xf));
                int32_t x1 = x0 + 1;
                double dx = src_xf - x0;

                if (x0 < 0) { x0 = 0; x1 = 0; }
                if (x1 >= static_cast<int32_t>(input->width)) { x0 = input->width - 1; x1 = input->width - 1; }

                for (uint32_t c = 0; c < channels; ++c) {
                    double val00 = src[y0 * input->stride + x0 * channels + c];
                    double val10 = src[y0 * input->stride + x1 * channels + c];
                    double val01 = src[y1 * input->stride + x0 * channels + c];
                    double val11 = src[y1 * input->stride + x1 * channels + c];

                    double val = val00 * (1.0 - dx) * (1.0 - dy) +
                                 val10 * dx * (1.0 - dy) +
                                 val01 * (1.0 - dx) * dy +
                                 val11 * dx * dy;

                    dst_row[x * channels + c] = static_cast<uint8_t>(std::clamp(val, 0.0, 255.0));
                }
            }
        }
    }

    if (input->release_fn) {
        input->release_fn(input);
    }

    *output = out_img;
    return IMGPROC_SUCCESS;
}
