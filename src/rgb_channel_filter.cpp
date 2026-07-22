#include <imgproc_filter_common.h>
#include <imgproc_logger.h>

#include <cctype>
#include <cstring>
#include <limits>
#include <new>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

enum class RgbChannel {
    RED = 0,
    GREEN = 1,
    BLUE = 2
};

struct RgbChannelFilter {
    RgbChannel channel = RgbChannel::RED;
};

#define RGB_FILTER_TO_HANDLE(filter) (static_cast<ImgProcFilterHandle>(filter))
#define RGB_FILTER_FROM_HANDLE(filter_handle) (static_cast<RgbChannelFilter*>(filter_handle))

ImgProcStatus rgb_channel_filter_create(ImgProcFilterHandle* filter_handle,
                                        ImgProcConfigHandle filter_config_handle);
ImgProcStatus rgb_channel_filter_destroy(ImgProcFilterHandle filter_handle);
ImgProcStatus rgb_channel_filter_transform(ImgProcFilterHandle filter_handle, ImgProcImage* input,
                                           ImgProcImage** output);

#ifdef __cplusplus
}
#endif

IMGPROC_FILTER_DECLARE_CREATE_FN(rgb_channel_filter_create)
IMGPROC_FILTER_DECLARE_DESTROY_FN(rgb_channel_filter_destroy)
IMGPROC_FILTER_DECLARE_TRANSFORM_FN(rgb_channel_filter_transform)

static RgbChannel parse_channel_string(const char* str) {
    if (!str) return RgbChannel::RED;

    std::string s(str);
    for (auto& c : s) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    if (s == "G" || s == "GREEN") {
        return RgbChannel::GREEN;
    } else if (s == "B" || s == "BLUE") {
        return RgbChannel::BLUE;
    }

    return RgbChannel::RED;
}

ImgProcStatus rgb_channel_filter_create(ImgProcFilterHandle* filter_handle,
                                        ImgProcConfigHandle filter_config_handle) {
    if (!filter_handle) {
        IMGPROC_LOG_ERROR("'filter_handle' is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }

    RgbChannel selected_channel = RgbChannel::RED;

    if (filter_config_handle) {
        IMGPROC_LOG_INFO("Loading configuration for RGB channel filter.");
        const char* channel_str = nullptr;
        ImgProcStatus status =
            imgproc_config_get_string(filter_config_handle, "rgb_channel_filter", "channel", &channel_str);

        if (status == IMGPROC_SUCCESS && channel_str) {
            selected_channel = parse_channel_string(channel_str);
            IMGPROC_LOG_INFO("Selected RGB channel: %s", channel_str);
        } else {
            IMGPROC_LOG_WARN("Could not read 'rgb_channel_filter.channel', defaulting to RED.");
        }
    } else {
        IMGPROC_LOG_INFO("No configuration specified, defaulting to RED channel.");
    }

    RgbChannelFilter* filter = new (std::nothrow) RgbChannelFilter;
    if (!filter) {
        IMGPROC_LOG_ERROR("Unable to allocate memory for RgbChannelFilter.");
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }

    filter->channel = selected_channel;
    *filter_handle = RGB_FILTER_TO_HANDLE(filter);

    IMGPROC_LOG_INFO("RgbChannelFilter created successfully.");
    return IMGPROC_SUCCESS;
}

ImgProcStatus rgb_channel_filter_destroy(ImgProcFilterHandle filter_handle) {
    if (!filter_handle) {
        IMGPROC_LOG_ERROR("'filter_handle' is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }

    RgbChannelFilter* filter = RGB_FILTER_FROM_HANDLE(filter_handle);
    delete filter;

    IMGPROC_LOG_INFO("RgbChannelFilter destroyed.");
    return IMGPROC_SUCCESS;
}

ImgProcStatus rgb_channel_filter_transform(ImgProcFilterHandle filter_handle, ImgProcImage* input,
                                           ImgProcImage** output) {
    if (!filter_handle) {
        IMGPROC_LOG_ERROR("'filter_handle' is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }

    if (!input || !input->data || !output) {
        IMGPROC_LOG_ERROR("Invalid image parameters for transformation.");
        return IMGPROC_ERROR_INVALID_ARG;
    }

    RgbChannelFilter* filter = RGB_FILTER_FROM_HANDLE(filter_handle);

    uint32_t bpp = (input->width > 0) ? (input->stride / input->width) : 0;
    if (bpp < 3) {
        IMGPROC_LOG_ERROR("Unsupported image format: bytes per pixel must be >= 3.");
        return IMGPROC_ERROR_INVALID_ARG;
    }

    uint8_t* raw_data = static_cast<uint8_t*>(input->data);

    for (uint32_t y = 0; y < input->height; ++y) {
        uint8_t* row = raw_data + (y * input->stride);
        for (uint32_t x = 0; x < input->width; ++x) {
            uint8_t* pixel = row + (x * bpp);
            switch (filter->channel) {
                case RgbChannel::RED:
                    pixel[1] = 0; // Clear Green
                    pixel[2] = 0; // Clear Blue
                    break;
                case RgbChannel::GREEN:
                    pixel[0] = 0; // Clear Red
                    pixel[2] = 0; // Clear Blue
                    break;
                case RgbChannel::BLUE:
                    pixel[0] = 0; // Clear Red
                    pixel[1] = 0; // Clear Green
                    break;
            }
        }
    }

    *output = input;
    IMGPROC_LOG_INFO("Image RGB channel transformation complete.");

    return IMGPROC_SUCCESS;
}
