#include <imgproc_filter_common.h>
#include <imgproc_logger.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <new>

#ifdef __cplusplus
extern "C" {
#endif

struct GrayscaleFilter {
    bool use_weighted = true;
};

ImgProcStatus grayscale_filter_create(ImgProcFilterHandle* filter_handle, ImgProcConfigHandle filter_config_handle);
ImgProcStatus grayscale_filter_destroy(ImgProcFilterHandle filter_handle);
ImgProcStatus grayscale_filter_transform(ImgProcFilterHandle filter_handle, ImgProcImage* input, ImgProcImage** output);

#ifdef __cplusplus
}
#endif

IMGPROC_FILTER_DECLARE_CREATE_FN(grayscale_filter_create)
IMGPROC_FILTER_DECLARE_DESTROY_FN(grayscale_filter_destroy)
IMGPROC_FILTER_DECLARE_TRANSFORM_FN(grayscale_filter_transform)

ImgProcStatus grayscale_filter_create(ImgProcFilterHandle* filter_handle, ImgProcConfigHandle filter_config_handle) {
    (void)filter_config_handle;
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

ImgProcStatus grayscale_filter_destroy(ImgProcFilterHandle filter_handle) {
    if (filter_handle) {
        delete static_cast<GrayscaleFilter*>(filter_handle);
        IMGPROC_LOG_INFO("GrayscaleFilter destroyed.");
    }
    return IMGPROC_SUCCESS;
}

ImgProcStatus grayscale_filter_transform(ImgProcFilterHandle filter_handle, ImgProcImage* input, ImgProcImage** output) {
    if (!filter_handle) {
        IMGPROC_LOG_ERROR("'filter_handle' is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }
    if (!input || !input->data) {
        IMGPROC_LOG_ERROR("Input image or image data is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }
    if (input->channels != 4) {
        IMGPROC_LOG_ERROR("GrayscaleFilter requires 4-channel RGBA image, got %u.", input->channels);
        return IMGPROC_ERROR_INVALID_ARG;
    }
    if (!output) {
        IMGPROC_LOG_ERROR("'output' pointer is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }

    uint32_t bpp = 4;

    ImgProcImage* out_img = new (std::nothrow) ImgProcImage;
    if (!out_img) {
        IMGPROC_LOG_ERROR("Failed to allocate ImgProcImage structure.");
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }

    out_img->data = malloc(input->data_size);
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
    out_img->release_fn = [](ImgProcImage* img) {
        if (img) {
            if (img->data) {
                free(img->data);
                img->data = nullptr;
            }
        }
    };

    const uint8_t* raw_data = static_cast<const uint8_t*>(input->data);
    uint8_t* out_data = static_cast<uint8_t*>(out_img->data);

    for (uint32_t y = 0; y < input->height; ++y) {
        const uint8_t* row = raw_data + (y * input->stride);
        uint8_t* out_row = out_data + (y * input->stride);
        for (uint32_t x = 0; x < input->width; ++x) {
            const uint8_t* pixel = row + (x * bpp);
            uint8_t* out_pixel = out_row + (x * bpp);
            uint8_t r = pixel[0];
            uint8_t g = pixel[1];
            uint8_t b = pixel[2];

            uint8_t gray = static_cast<uint8_t>(0.299 * r + 0.587 * g + 0.114 * b);

            out_pixel[0] = gray;
            out_pixel[1] = gray;
            out_pixel[2] = gray;
            for (uint32_t c = 3; c < bpp; ++c) {
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
