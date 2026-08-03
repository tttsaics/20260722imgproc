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
    std::string method = "bilinear";
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
    if (!filter_handle) {  
        IMGPROC_LOG_ERROR("'filter_handle' is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }
    *filter_handle = IMGPROC_INVALID_FILTER_HANDLE;
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
    if (method_str && (std::strcmp(method_str, "nearest") == 0 ||
                       std::strcmp(method_str, "bilinear") == 0)) {
        filter->method = method_str;
    } else {
        if (method_str) {
            IMGPROC_LOG_WARN("Unknown resize method '%s', using default ('bilinear').", method_str);
        }
        filter->method = "bilinear";
    }

    *filter_handle = static_cast<ImgProcFilterHandle>(filter);
    IMGPROC_LOG_INFO("ResizeFilter created. scale=%f, width=%d, height=%d, method=%s",
                     filter->scale, filter->width, filter->height, filter->method.c_str());

    return IMGPROC_SUCCESS;
}

ImgProcStatus resize_filter_destroy(ImgProcFilterHandle filter_handle) {
    if (!filter_handle) {
        IMGPROC_LOG_ERROR("'filter_handle' is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }

    ResizeFilter* filter = static_cast<ResizeFilter*>(filter_handle);   
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
    if (!output) {
        IMGPROC_LOG_ERROR("'output' is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }
    *output = nullptr;
    if (!input || !input->data) {   
        IMGPROC_LOG_ERROR("'input' or input data is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }
    if (input->channels != 4) { 
        IMGPROC_LOG_ERROR("ResizeFilter requires 4-channel RGBA image, got %u.", input->channels);
        return IMGPROC_ERROR_INVALID_ARG;
    }
    ResizeFilter* filter = static_cast<ResizeFilter*>(filter_handle);

    const uint32_t input_width = input->width;
    const uint32_t input_height = input->height;
    const uint32_t input_stride = input->stride;
    const size_t input_data_size = input->data_size;
    if (input_width == 0 || input_height == 0) {
        IMGPROC_LOG_ERROR("Image dimensions must be non-zero (got %ux%u).",
                          input_width, input_height);
        return IMGPROC_ERROR_INVALID_ARG;
    }
    const uint64_t min_input_row_bytes = static_cast<uint64_t>(input_width) * 4u;
    const uint64_t input_required_size = static_cast<uint64_t>(input_stride) * input_height;
    if (static_cast<uint64_t>(input_stride) < min_input_row_bytes ||
        input_required_size > static_cast<uint64_t>(input_data_size) ||
        input_required_size > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
        IMGPROC_LOG_ERROR("Invalid image layout: width=%u height=%u stride=%u data_size=%zu.",
                          input_width, input_height, input_stride, input_data_size);
        return IMGPROC_ERROR_INVALID_ARG;
    }

    //將新尺寸預設原長和寬
    uint32_t new_width = input_width;
    uint32_t new_height = input_height;

    if (filter->width > 0 && filter->height > 0) {  //如果設定了寬和高，則使用設定的值
        new_width = static_cast<uint32_t>(filter->width);
        new_height = static_cast<uint32_t>(filter->height);
    } else if (std::isfinite(filter->scale) && filter->scale > 0.0) {  
        //使用 std::round 進行四捨五入至最接近的整數像素，避免直接轉整數(無條件捨去)導致 1 像素的失真
        double calc_w = std::round(static_cast<double>(input_width) * filter->scale);
        double calc_h = std::round(static_cast<double>(input_height) * filter->scale);
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

    constexpr uint32_t channels = 4;
    const uint64_t row_bytes = static_cast<uint64_t>(new_width) * channels;
    const uint64_t new_stride_64 = (row_bytes + 3u) & ~uint64_t(3u);
    //~3(取反) 最後兩個位元強制為 00。任何整數與 ~3 進行 &（AND）位元運算後，最低兩位都會被清零，結果保證能被 4 整除

    const uint64_t total_bytes = new_stride_64 * new_height;
    //強制轉為64位元避免發生溢位
    if (new_width > 100000 || new_height > 100000 ||
        new_stride_64 > std::numeric_limits<uint32_t>::max() ||
        total_bytes > 2000000000ULL ||
        total_bytes > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
        IMGPROC_LOG_ERROR("Resize target dimensions (%ux%u) or buffer size (%llu bytes) exceed limits.",
                          new_width, new_height,
                          static_cast<unsigned long long>(total_bytes));
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }

    const uint32_t new_stride = static_cast<uint32_t>(new_stride_64);
    size_t new_data_size = static_cast<size_t>(total_bytes);

    
    ImgProcImage* out_img = new (std::nothrow) ImgProcImage;
    if (!out_img) {
        IMGPROC_LOG_ERROR("Failed to allocate ImgProcImage structure.");
        if (input->release_fn) {
            input->release_fn(input);
        }
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }

    out_img->data = malloc(new_data_size);
    if (!out_img->data) {
        delete out_img;
        IMGPROC_LOG_ERROR("Failed to allocate memory for image data.");
        if (input->release_fn) {
            input->release_fn(input);
        }
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }

    out_img->width = new_width;
    out_img->height = new_height;
    out_img->stride = new_stride;
    out_img->channels = channels;//設定通道數為 4，對應 RGBA 圖像
    out_img->data_size = new_data_size;
    out_img->release_fn = [](ImgProcImage* img) { 
        if (img) {  
            if (img->data) {
                free(img->data);
                img->data = nullptr;
            }
        }
    };

    IMGPROC_LOG_INFO("Resizing image: %dx%d (stride %d) -> %dx%d (stride %d) using method %s", 
       
                     input_width, input_height, input_stride,
                     new_width, new_height, new_stride, filter->method.c_str());

    const uint8_t* src = static_cast<const uint8_t*>(input->data);
    uint8_t* dst = static_cast<uint8_t*>(out_img->data);

    if (filter->method == "nearest") {
        //最鄰近插值法:掃描縮放後的「新影像」每一個像素點，算出它對應回「原圖」的相對位置，並直接把距離最近的像素顏色拿過來用
        for (uint32_t y = 0; y < new_height; ++y) {
            uint32_t src_y = static_cast<uint32_t>((static_cast<uint64_t>(y) * input_height) / new_height); //計算對應回原圖的 Y 座標

            if (src_y >= input_height) src_y = input_height - 1;    //避免超出原圖範圍
            //計算來源和目標行的記憶體位置起始位置
            const uint8_t* src_row = src + static_cast<size_t>(src_y) * input_stride;
            uint8_t* dst_row = dst + static_cast<size_t>(y) * new_stride;

            for (uint32_t x = 0; x < new_width; ++x) {  //計算對應回原圖的 X 座標
                uint32_t src_x = static_cast<uint32_t>((static_cast<uint64_t>(x) * input_width) / new_width);

                if (src_x >= input_width) src_x = input_width - 1;

                for (uint32_t c = 0; c < channels; ++c) {
                    dst_row[static_cast<size_t>(x) * channels + c] =
                        src_row[static_cast<size_t>(src_x) * channels + c];
                }
            }
        }
    } else {
        // Bilinear (雙線性插值法)線性插值則會抓取點周圍的 4 個相鄰像素，根據距離計算出權重後進行加權平均
        //計算縮放比例
        double scale_x = static_cast<double>(input_width) / new_width;
        double scale_y = static_cast<double>(input_height) / new_height;

        for (uint32_t y = 0; y < new_height; ++y) { 
            //將新舊圖像的幾何中心重合
            double src_yf = (y + 0.5) * scale_y - 0.5;
            int64_t y0 = static_cast<int64_t>(std::floor(src_yf)); //取左上/上方相鄰像素 Y 座標
            int64_t y1 = y0 + 1;    //取右下/下方相鄰像素 Y 座標
            double dy = src_yf - y0;
            
            //邊界處理，避免超出原圖範圍
            if (y0 < 0) { y0 = 0; y1 = 0; }
            if (y1 >= static_cast<int64_t>(input_height)) { y0 = input_height - 1; y1 = input_height - 1; }

            uint8_t* dst_row = dst + static_cast<size_t>(y) * new_stride;

            for (uint32_t x = 0; x < new_width; ++x) {
                double src_xf = (x + 0.5) * scale_x - 0.5;
                int64_t x0 = static_cast<int64_t>(std::floor(src_xf));  //取左側像素 X 座標
                int64_t x1 = x0 + 1;       //取右側像素 X 座標
                double dx = src_xf - x0;
                
                //邊界處理，避免超出原圖範圍
                if (x0 < 0) { x0 = 0; x1 = 0; }
                if (x1 >= static_cast<int64_t>(input_width)) { x0 = input_width - 1; x1 = input_width - 1; }

                for (uint32_t c = 0; c < channels; ++c) {
                    double val00 = src[static_cast<size_t>(y0) * input_stride +
                                       static_cast<size_t>(x0) * channels + c];//val00 :左上(x0, y0)
                    double val10 = src[static_cast<size_t>(y0) * input_stride +
                                       static_cast<size_t>(x1) * channels + c];//val10 :右上(x1, y0)
                    double val01 = src[static_cast<size_t>(y1) * input_stride +
                                       static_cast<size_t>(x0) * channels + c];//val01 :左下(x0, y1)
                    double val11 = src[static_cast<size_t>(y1) * input_stride +
                                       static_cast<size_t>(x1) * channels + c];//val11 :右下(x1, y1)
                    //雙線性差值公式
                    double val = val00 * (1.0 - dx) * (1.0 - dy) +
                                 val10 * dx * (1.0 - dy) +
                                 val01 * (1.0 - dx) * dy +
                                 val11 * dx * dy;
                    //使用 std::clamp 確保結果在 0~255 範圍內，避免溢位
                    dst_row[static_cast<size_t>(x) * channels + c] =
                        static_cast<uint8_t>(std::clamp(val, 0.0, 255.0));
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
