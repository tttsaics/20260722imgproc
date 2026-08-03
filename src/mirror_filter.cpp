#include <imgproc_filter_common.h>
#include <imgproc_logger.h>

#include <cstdlib>
#include <cstring>  
#include <limits>
#include <new>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

enum class MirrorMode {
    HORIZONTAL,
    VERTICAL,
    BOTH
};

struct MirrorFilter {
    MirrorMode mode = MirrorMode::HORIZONTAL;
};

#define MIRROR_FILTER_TO_HANDLE(filter) (static_cast<ImgProcFilterHandle>(filter))
#define MIRROR_FILTER_FROM_HANDLE(filter_handle) (static_cast<MirrorFilter*>(filter_handle))

ImgProcStatus mirror_filter_create(ImgProcFilterHandle* filter_handle,
                                   ImgProcConfigHandle filter_config_handle);
ImgProcStatus mirror_filter_destroy(ImgProcFilterHandle filter_handle);
ImgProcStatus mirror_filter_transform(ImgProcFilterHandle filter_handle, ImgProcImage* input,
                                      ImgProcImage** output);

#ifdef __cplusplus
}
#endif

IMGPROC_FILTER_DECLARE_CREATE_FN(mirror_filter_create)
IMGPROC_FILTER_DECLARE_DESTROY_FN(mirror_filter_destroy)
IMGPROC_FILTER_DECLARE_TRANSFORM_FN(mirror_filter_transform)

ImgProcStatus mirror_filter_create(ImgProcFilterHandle* filter_handle,
                                   ImgProcConfigHandle filter_config_handle) {
    if (!filter_handle) {
        IMGPROC_LOG_ERROR("'filter_handle' is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }
    *filter_handle = IMGPROC_INVALID_FILTER_HANDLE;

    MirrorMode mode = MirrorMode::HORIZONTAL;
    const char* mode_str = nullptr;

    if (filter_config_handle) {
        IMGPROC_LOG_INFO("Reading mirror_filter configuration.");
        ImgProcStatus status =
            imgproc_config_get_string(filter_config_handle, "mirror_filter", "mode", &mode_str);
        if (status == IMGPROC_SUCCESS && mode_str) {
            std::string s(mode_str);
            if (s == "vertical") {
                mode = MirrorMode::VERTICAL;
            } else if (s == "both") {
                mode = MirrorMode::BOTH;
            } else if(s == "horizontal") {
                mode = MirrorMode::HORIZONTAL;
            } else{
                IMGPROC_LOG_WARN("Unknown mirror mode '%s', falling back to 'horizontal'.", s.c_str());
                mode = MirrorMode::HORIZONTAL;
            }
        } else {
            IMGPROC_LOG_WARN("Unable to get 'mirror_filter.mode', using default value ('horizontal').");
        }
    } else {
        IMGPROC_LOG_INFO("Configuration file is not specified, using default mirror mode ('horizontal').");
    }

    MirrorFilter* filter = new (std::nothrow) MirrorFilter;
    if (!filter) {  
        IMGPROC_LOG_ERROR("Unable to allocate memory for 'MirrorFilter'.");
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }

    filter->mode = mode;
    *filter_handle = MIRROR_FILTER_TO_HANDLE(filter);

    IMGPROC_LOG_INFO("MirrorFilter created successfully.");
    return IMGPROC_SUCCESS;
}

ImgProcStatus mirror_filter_destroy(ImgProcFilterHandle filter_handle) {
    if (!filter_handle) {
        IMGPROC_LOG_ERROR("'filter_handle' is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }

    MirrorFilter* filter = MIRROR_FILTER_FROM_HANDLE(filter_handle); 
    if (filter) {
        delete filter;
        IMGPROC_LOG_INFO("MirrorFilter destroyed.");
    }
    return IMGPROC_SUCCESS;
}

ImgProcStatus mirror_filter_transform(ImgProcFilterHandle filter_handle, ImgProcImage* input,
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
        IMGPROC_LOG_ERROR("MirrorFilter requires 4-channel RGBA image, got %u.", input->channels);
        return IMGPROC_ERROR_INVALID_ARG;
    }
    MirrorFilter* filter = MIRROR_FILTER_FROM_HANDLE(filter_handle);

    constexpr uint32_t channels = 4;

    uint32_t width = input->width;
    uint32_t height = input->height;
    uint32_t stride = input->stride;
    size_t data_size = input->data_size;

    
    if (width == 0 || height == 0) {
        IMGPROC_LOG_ERROR("Image dimensions must be non-zero (got %ux%u).", width, height);
        return IMGPROC_ERROR_INVALID_ARG;
    }  

    const uint64_t min_row_bytes = static_cast<uint64_t>(width) * channels;
    const uint64_t required_size = static_cast<uint64_t>(stride) * height;
    if (static_cast<uint64_t>(stride) < min_row_bytes ||
        required_size > static_cast<uint64_t>(data_size) ||
        required_size > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
        IMGPROC_LOG_ERROR("Invalid image layout: width=%u height=%u stride=%u data_size=%zu.",
                          width, height, stride, data_size);
        return IMGPROC_ERROR_INVALID_ARG;
    }

    ImgProcImage* out_img = new (std::nothrow) ImgProcImage;
    if (!out_img) {
        IMGPROC_LOG_ERROR("Failed to allocate ImgProcImage structure.");
        if (input->release_fn) {
            input->release_fn(input);
        }
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }

    out_img->data = std::malloc(data_size);
    if (!out_img->data) {
        delete out_img;
        IMGPROC_LOG_ERROR("Failed to allocate image buffer.");
        if (input->release_fn) {
            input->release_fn(input);
        }
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }

    out_img->width = width;
    out_img->height = height;
    out_img->stride = stride;
    out_img->channels = channels;
    out_img->data_size = data_size;
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

    
    std::memcpy(dst, src, data_size);

    bool flip_h = (filter->mode == MirrorMode::HORIZONTAL || filter->mode == MirrorMode::BOTH);
    bool flip_v = (filter->mode == MirrorMode::VERTICAL || filter->mode == MirrorMode::BOTH);

    for (uint32_t y = 0; y < height; ++y) {
        uint32_t src_y = flip_v ? (height - 1 - y) : y;
        const uint8_t* src_row = src + static_cast<size_t>(src_y) * stride;
        uint8_t* dst_row = dst + static_cast<size_t>(y) * stride;

        for (uint32_t x = 0; x < width; ++x) {
            uint32_t src_x = flip_h ? (width - 1 - x) : x;

            const uint8_t* src_pixel = src_row + static_cast<size_t>(src_x) * channels;
            uint8_t* dst_pixel = dst_row + static_cast<size_t>(x) * channels;

            for (uint32_t c = 0; c < channels; ++c) {
                dst_pixel[c] = src_pixel[c];
            }
        }
    }

    if (input->release_fn) {
        input->release_fn(input);
    }

    *output = out_img;
    IMGPROC_LOG_INFO("Image mirrored successfully (%ux%u).", width, height);

    return IMGPROC_SUCCESS;
}
