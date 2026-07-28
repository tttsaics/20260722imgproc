#include <imgproc_filter_common.h>
#include <imgproc_logger.h>

#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>

#ifdef __cplusplus
extern "C" {
#endif

struct RotateFilter {
    int32_t angle; // Allowed: 90, 180, 270, 360 (or 0)
};

#define ROTATE_FILTER_TO_HANDLE(filter) (static_cast<ImgProcFilterHandle>(filter))
#define ROTATE_FILTER_FROM_HANDLE(filter_handle) (static_cast<RotateFilter*>(filter_handle))

ImgProcStatus rotate_filter_create(ImgProcFilterHandle* filter_handle,
                                   ImgProcConfigHandle filter_config_handle);
ImgProcStatus rotate_filter_destroy(ImgProcFilterHandle filter_handle);
ImgProcStatus rotate_filter_transform(ImgProcFilterHandle filter_handle, ImgProcImage* input,
                                      ImgProcImage** output);

#ifdef __cplusplus
}
#endif

IMGPROC_FILTER_DECLARE_CREATE_FN(rotate_filter_create)
IMGPROC_FILTER_DECLARE_DESTROY_FN(rotate_filter_destroy)
IMGPROC_FILTER_DECLARE_TRANSFORM_FN(rotate_filter_transform)

ImgProcStatus rotate_filter_create(ImgProcFilterHandle* filter_handle,
                                   ImgProcConfigHandle filter_config_handle) {
    if (!filter_handle) {
        IMGPROC_LOG_ERROR("'filter_handle' is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }
    *filter_handle = IMGPROC_INVALID_FILTER_HANDLE;

    int32_t angle = 360; 

    if (filter_config_handle) {
        IMGPROC_LOG_INFO("Reading rotate_filter configuration.");
        int64_t config_angle = 0;
        ImgProcStatus status =
            imgproc_config_get_int64(filter_config_handle, "rotate_filter", "angle", &config_angle);
        if (status == IMGPROC_SUCCESS) {
            if (config_angle >= INT32_MIN && config_angle <= INT32_MAX) {   //保證數值轉換安全
                angle = static_cast<int32_t>(config_angle);
            } else {
                IMGPROC_LOG_WARN("Config angle %ld out of int32 bounds, using default (%d).", config_angle, angle);
            }
        } else {
            IMGPROC_LOG_WARN("Unable to get 'rotate_filter.angle', using default value (%d).", angle);
        }
    } else {
        IMGPROC_LOG_INFO("Configuration file is not specified, using default angle (%d).", angle);
    }

    //角度正規劃
    int normalized_angle = angle % 360; //同餘轉換
    if (normalized_angle < 0) normalized_angle += 360;//負數補正

    if (normalized_angle != 0 && normalized_angle != 90 && normalized_angle != 180 && normalized_angle != 270) {
        IMGPROC_LOG_ERROR("Invalid rotation angle: %d. Only 90, 180, 270, and 360 degrees are supported.", angle);
        return IMGPROC_ERROR_INVALID_ARG;
    }

    RotateFilter* filter = new (std::nothrow) RotateFilter;
    if (!filter) {
        IMGPROC_LOG_ERROR("Unable to allocate memory for 'RotateFilter'.");
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }

    filter->angle = normalized_angle;
    *filter_handle = ROTATE_FILTER_TO_HANDLE(filter);

    IMGPROC_LOG_INFO("RotateFilter created successfully with angle = %d degrees.", angle);
    return IMGPROC_SUCCESS;
}

ImgProcStatus rotate_filter_destroy(ImgProcFilterHandle filter_handle) {
    if (!filter_handle) {
        IMGPROC_LOG_ERROR("'filter_handle' is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }

    RotateFilter* filter = ROTATE_FILTER_FROM_HANDLE(filter_handle);
    if (filter) {
        delete filter;
        IMGPROC_LOG_INFO("RotateFilter destroyed.");
    }
    return IMGPROC_SUCCESS;
}

ImgProcStatus rotate_filter_transform(ImgProcFilterHandle filter_handle, ImgProcImage* input,
                                      ImgProcImage** output) {
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
        IMGPROC_LOG_ERROR("RotateFilter requires 4-channel RGBA image, got %u.", input->channels);
        return IMGPROC_ERROR_INVALID_ARG;
    }
    RotateFilter* filter = ROTATE_FILTER_FROM_HANDLE(filter_handle);
    int angle = filter->angle;

    constexpr uint32_t channels = 4;

    const uint32_t input_width = input->width;
    const uint32_t input_height = input->height;
    const uint32_t input_stride = input->stride;
    const size_t input_data_size = input->data_size;

    if (input_width == 0 || input_height == 0) {
        IMGPROC_LOG_ERROR("Image dimensions must be non-zero (got %ux%u).",
                          input_width, input_height);
        return IMGPROC_ERROR_INVALID_ARG;
    }

    const uint64_t min_input_row_bytes = static_cast<uint64_t>(input_width) * channels;
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

    if (angle == 90 || angle == 270) {//若角度是 90 or 270長寬要對調
        new_width = input_height;
        new_height = input_width;
    }

    const uint64_t row_bytes = static_cast<uint64_t>(new_width) * channels;
    const uint64_t new_stride_64 = (row_bytes + 3u) & ~uint64_t(3u);
    const uint64_t total_bytes = new_stride_64 * new_height;
    
    if (new_width > 100000 || new_height > 100000 ||
        new_stride_64 > std::numeric_limits<uint32_t>::max() ||
        total_bytes > 2000000000ULL ||
        total_bytes > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
        IMGPROC_LOG_ERROR("Rotate target dimensions (%ux%u) or buffer size (%llu bytes) exceed limits.",
                          new_width, new_height,
                          static_cast<unsigned long long>(total_bytes));
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }
    uint32_t new_stride = static_cast<uint32_t>(new_stride_64);
    size_t new_data_size = static_cast<size_t>(total_bytes);

    ImgProcImage* out_img = new (std::nothrow) ImgProcImage;
    if (!out_img) {
        IMGPROC_LOG_ERROR("Failed to allocate ImgProcImage structure.");
        if (input->release_fn) {
            input->release_fn(input);
        }
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }

    out_img->data = std::malloc(new_data_size);
    if (!out_img->data) {
        delete out_img;
        IMGPROC_LOG_ERROR("Failed to allocate image buffer.");
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
                std::free(img->data);
                img->data = nullptr;
            }
        }
    };

    const uint8_t* src = static_cast<const uint8_t*>(input->data); 
    uint8_t* dst = static_cast<uint8_t*>(out_img->data);

    for (uint32_t y = 0; y < new_height; ++y) {
        for (uint32_t x = 0; x < new_width; ++x) {
            uint32_t src_x = x;
            uint32_t src_y = y;

            if (angle == 90) {
                src_x = y;
                src_y = input->height - 1 - x;
            } else if (angle == 180) {
                src_x = input->width - 1 - x;
                src_y = input->height - 1 - y;
            } else if (angle == 270) {
                src_x = input->width - 1 - y;
                src_y = x;
            }


            const uint8_t* src_pixel = src + static_cast<size_t>(src_y) * input_stride +
                                        static_cast<size_t>(src_x) * channels;
            uint8_t* dst_pixel = dst + static_cast<size_t>(y) * new_stride +
                                 static_cast<size_t>(x) * channels;

            for (uint32_t c = 0; c < channels; ++c) {
                dst_pixel[c] = src_pixel[c];
            }
        }
    }

    
    IMGPROC_LOG_INFO("Image rotated by %d degrees successfully (%dx%d -> %dx%d).",
                     angle == 0 ? 360 : angle, input_width, input_height, new_width, new_height);

    if (input->release_fn) {
        input->release_fn(input);
    }

    *output = out_img;

    return IMGPROC_SUCCESS;
}
