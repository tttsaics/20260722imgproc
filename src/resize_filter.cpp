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
    double scale = 1.0;
    int64_t width = 0;
    int64_t height = 0;
    const char* method_str = nullptr;

    if (filter_config_handle) {
        IMGPROC_LOG_INFO("Loading resize filter configuration.");

        double temp_scale = 0.0;
        ImgProcStatus status = imgproc_config_get_double(filter_config_handle, "resize_filter", "scale", &temp_scale);
        if (status == IMGPROC_SUCCESS) {
            if (std::isfinite(temp_scale) && temp_scale > 0.0 && temp_scale <= 100.0) {
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

    uint32_t new_width = input_width;
    uint32_t new_height = input_height;

    if (filter->width > 0 && filter->height > 0) {
        new_width = static_cast<uint32_t>(filter->width);
        new_height = static_cast<uint32_t>(filter->height);
    } else if (std::isfinite(filter->scale) && filter->scale > 0.0) {
        double calc_w = std::round(static_cast<double>(input_width) * filter->scale);
        double calc_h = std::round(static_cast<double>(input_height) * filter->scale);
        if (std::isfinite(calc_w) && std::isfinite(calc_h) && 
            calc_w >= 1.0 && calc_w <= 100000.0 && 
            calc_h >= 1.0 && calc_h <= 100000.0) {
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
    
    if (new_width == 0) new_width = 1;
    if (new_height == 0) new_height = 1;

    constexpr uint32_t channels = 4;
    const uint64_t row_bytes = static_cast<uint64_t>(new_width) * channels;
    const uint64_t new_stride_64 = (row_bytes + 3u) & ~uint64_t(3u);

    const uint64_t total_bytes = new_stride_64 * new_height;
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
    out_img->channels = channels;
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
        for (uint32_t y = 0; y < new_height; ++y) {
            uint32_t src_y = static_cast<uint32_t>((static_cast<uint64_t>(y) * input_height) / new_height);
            if (src_y >= input_height) src_y = input_height - 1;
            const uint8_t* src_row = src + static_cast<size_t>(src_y) * input_stride;
            uint8_t* dst_row = dst + static_cast<size_t>(y) * new_stride;
            for (uint32_t x = 0; x < new_width; ++x) {
                uint32_t src_x = static_cast<uint32_t>((static_cast<uint64_t>(x) * input_width) / new_width);
                if (src_x >= input_width) src_x = input_width - 1;
                for (uint32_t c = 0; c < channels; ++c) {
                    dst_row[static_cast<size_t>(x) * channels + c] =
                        src_row[static_cast<size_t>(src_x) * channels + c];
                }
            }
        }
    } else {
        double scale_x = static_cast<double>(input_width) / new_width;
        double scale_y = static_cast<double>(input_height) / new_height;

        for (uint32_t y = 0; y < new_height; ++y) {
            double src_yf = (y + 0.5) * scale_y - 0.5;
            int64_t y0 = static_cast<int64_t>(std::floor(src_yf));
            int64_t y1 = y0 + 1;
            double dy = src_yf - y0;

            if (y0 < 0) { y0 = 0; y1 = 0; }
            if (y1 >= static_cast<int64_t>(input_height)) { y0 = input_height - 1; y1 = input_height - 1; }

            uint8_t* dst_row = dst + static_cast<size_t>(y) * new_stride;

            for (uint32_t x = 0; x < new_width; ++x) {
                double src_xf = (x + 0.5) * scale_x - 0.5;
                int64_t x0 = static_cast<int64_t>(std::floor(src_xf));
                int64_t x1 = x0 + 1;
                double dx = src_xf - x0;

                if (x0 < 0) { x0 = 0; x1 = 0; }
                if (x1 >= static_cast<int64_t>(input_width)) { x0 = input_width - 1; x1 = input_width - 1; }

                for (uint32_t c = 0; c < channels; ++c) {
                    double val00 = src[static_cast<size_t>(y0) * input_stride +
                                       static_cast<size_t>(x0) * channels + c];
                    double val10 = src[static_cast<size_t>(y0) * input_stride +
                                       static_cast<size_t>(x1) * channels + c];
                    double val01 = src[static_cast<size_t>(y1) * input_stride +
                                       static_cast<size_t>(x0) * channels + c];
                    double val11 = src[static_cast<size_t>(y1) * input_stride +
                                       static_cast<size_t>(x1) * channels + c];

                    double val = val00 * (1.0 - dx) * (1.0 - dy) +
                                 val10 * dx * (1.0 - dy) +
                                 val01 * (1.0 - dx) * dy +
                                 val11 * dx * dy;

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
