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
    //設定預設值為123，若有配置檔案則使用配置檔案的值
    if (filter_config_handle) {
        IMGPROC_LOG_INFO("Use configuration file.");

        int64_t config_value = 0;
        ImgProcStatus status =
            imgproc_config_get_int64(filter_config_handle, "dummy_filter", "value", &config_value);
        if (status != IMGPROC_SUCCESS) {
            IMGPROC_LOG_ERROR("Unable to get 'dummy_filter.value'.");
            return status;
        }

        if (config_value <= std::numeric_limits<int32_t>::max()) {//檢查配置檔案的值是否超過32位元整數的最大值
            value = static_cast<int32_t>(config_value);//轉換成32位元整數
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

    *filter_handle = DUMMY_FILTER_TO_HANDLE(filter);//將 filter 指標透過 DUMMY_FILTER_TO_HANDLE 巨集轉型為ImgProcFilterHandle（void*），並解引用賦值給 *filter_handle 傳回給呼叫者。
    IMGPROC_LOG_INFO("DummyFilter created.");

    return IMGPROC_SUCCESS;
}
//防止記憶體洩漏，釋放 DummyFilter 物件的記憶體
ImgProcStatus dummy_filter_destroy(ImgProcFilterHandle filter_handle) {//檢查 filter_handle 是否為空指標，若是則回傳錯誤狀態碼 IMGPROC_ERROR_INVALID_ARG。
    DummyFilter* filter = DUMMY_FILTER_FROM_HANDLE(filter_handle);//將 filter_handle 透過 DUMMY_FILTER_FROM_HANDLE 巨集轉型為 DummyFilter*，並賦值給 filter。
    if (filter) {
        delete filter;
        IMGPROC_LOG_INFO("DummyFilter destroied.");
    }

    return IMGPROC_SUCCESS;
}
//圖像轉換，將輸入圖像 input 直接賦值給輸出圖像 output，並記錄轉換過程中的資訊。
ImgProcStatus dummy_filter_transform(ImgProcFilterHandle filter_handle, ImgProcImage* input,
                                     ImgProcImage** output) {//檢查 filter_handle 是否為空指標，若是則回傳錯誤狀態碼 IMGPROC_ERROR_INVALID_ARG。
    if (!filter_handle) {
        IMGPROC_LOG_ERROR("'filter_handle' is null.");
        return IMGPROC_ERROR_INVALID_ARG;
    }

    DummyFilter* filter = DUMMY_FILTER_FROM_HANDLE(filter_handle);
    *output = input;

    IMGPROC_LOG_INFO("Transform image. value=%d", filter->value);

    return IMGPROC_SUCCESS;
}
