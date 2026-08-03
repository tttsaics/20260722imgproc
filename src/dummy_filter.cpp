#include <imgproc_filter_common.h>
#include <imgproc_logger.h>

#include <limits>
#include <new>

#ifdef __cplusplus
extern "C" {
#endif

struct DummyFilter {
    int32_t value;
};

#define DUMMY_FILTER_TO_HANDLE(filter) (static_cast<ImgProcFilterHandle>(filter))
#define DUMMY_FILTER_FROM_HANDLE(filter_handle) (static_cast<DummyFilter*>(filter_handle))

ImgProcStatus dummy_filter_create(ImgProcFilterHandle* filter_handle,
                                  ImgProcConfigHandle filter_config_handle);
ImgProcStatus dummy_filter_destroy(ImgProcFilterHandle filter_handle);
ImgProcStatus dummy_filter_transform(ImgProcFilterHandle filter_handle, ImgProcImage* input,
                                     ImgProcImage** output);

#ifdef __cplusplus
}
#endif

IMGPROC_FILTER_DECLARE_CREATE_FN(dummy_filter_create)
IMGPROC_FILTER_DECLARE_DESTROY_FN(dummy_filter_destroy)
IMGPROC_FILTER_DECLARE_TRANSFORM_FN(dummy_filter_transform)

ImgProcStatus dummy_filter_create(ImgProcFilterHandle* filter_handle,
                                  ImgProcConfigHandle filter_config_handle) {
    if (!filter_handle) {
        IMGPROC_LOG_ERROR("'filter_handle' is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }

    int32_t value = 123;
    if (filter_config_handle) {
        IMGPROC_LOG_INFO("Use configuration file.");

        int64_t config_value = 0;
        ImgProcStatus status =
            imgproc_config_get_int64(filter_config_handle, "dummy_filter", "value", &config_value);
        if (status != IMGPROC_SUCCESS) {
            IMGPROC_LOG_ERROR("Unable to get 'dummy_filter.value'.");
            return status;
        }

        if (config_value <= std::numeric_limits<int32_t>::max()) {
            value = static_cast<int32_t>(config_value);
        } else {
            IMGPROC_LOG_WARN("'dummy_filter.value' overflow, use default value.");
        }
    } else {
        IMGPROC_LOG_INFO("Configuration file is not specified, use default value.");
    }

    DummyFilter* filter = new (std::nothrow) DummyFilter;
    if (!filter) {
        IMGPROC_LOG_ERROR("Unable to allocate memory for 'DummyFilter'.");
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }

    filter->value = value;

    *filter_handle = DUMMY_FILTER_TO_HANDLE(filter);
    IMGPROC_LOG_INFO("DummyFilter created.");

    return IMGPROC_SUCCESS;
}
ImgProcStatus dummy_filter_destroy(ImgProcFilterHandle filter_handle) {
    DummyFilter* filter = DUMMY_FILTER_FROM_HANDLE(filter_handle);
    if (filter) {
        delete filter;
        IMGPROC_LOG_INFO("DummyFilter destroied.");
    }

    return IMGPROC_SUCCESS;
}
ImgProcStatus dummy_filter_transform(ImgProcFilterHandle filter_handle, ImgProcImage* input,
                                     ImgProcImage** output) {
    if (!filter_handle) {
        IMGPROC_LOG_ERROR("'filter_handle' is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }

    DummyFilter* filter = DUMMY_FILTER_FROM_HANDLE(filter_handle);
    *output = input;

    IMGPROC_LOG_INFO("Transform image. value=%d", filter->value);

    return IMGPROC_SUCCESS;
}
