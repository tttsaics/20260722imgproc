#include <imgproc_logger.h>
#include <imgproc_filter_loader.h>

#include <stdlib.h>
#include <stdio.h>

#define TEST_PRINT_CALL_ERROR(expr, status) fprintf(stderr, "%s error: %s\n", #expr, imgproc_get_status_str((status)))

static void release_image(ImgProcImage* img) {//這個函式用於釋放圖像資源，檢查圖像的data指針是否為NULL，如果不是，則釋放其所指向的內存，並將data指針設置為NULL。
    if (img->data) {
        free(img->data);
        img->data = NULL;
    }
}

static bool init_image(ImgProcImage* img) {//這個函式用於初始化圖像結構體，設置其寬度、高度、步幅和數據大小，並分配內存給data指針。如果內存分配失敗，返回false，否則設置釋放函式指針並返回true。
    img->width = 1280;
    img->height = 720;
    img->stride = 4096;
    img->data_size = img->stride * img->height;

    img->data = malloc(img->data_size);
    if (!img->data) {
        return false;
    }

    img->release_fn = release_image;

    return true;
}

static bool test_transform(const ImgProcFilterApi* api, ImgProcFilterHandle filter_handle) {//這個函式用於測試濾鏡的圖像轉換功能，首先初始化一個輸入圖像，然後調用濾鏡的transform函式進行圖像轉換，最後檢查輸入圖像和輸出圖像是否為同一個實例。如果轉換成功且兩個圖像是同一個實例，返回true，否則返回false。
    ImgProcStatus status = IMGPROC_SUCCESS;
    ImgProcImage input_image;
    ImgProcImage* output_image = NULL;

    if (!init_image(&input_image)) {
        fprintf(stderr, "Unable to init an image.\n");
        return false;
    }

    status = api->transform(filter_handle, &input_image, &output_image);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(api->transform, status);
        input_image.release_fn(&input_image);
        return false;
    }

    if (&input_image != output_image) {
        fprintf(stderr, "'input_image' and 'output_image' are not the same instance.\n");
        input_image.release_fn(&input_image);
        output_image->release_fn(output_image);
        return false;
    }

    output_image->release_fn(output_image);
    return true;
}

int main() {
    imgproc_logger_use_console();
    imgproc_logger_set_level(IMGPROC_LOGLEVEL_DEBUG);//第四層及

    ImgProcStatus status = IMGPROC_SUCCESS;
    ImgProcConfigHandle config_handle = IMGPROC_INVALID_CONFIG_HANDLE;
    ImgProcFilterApi filter_api = {0};
    ImgProcFilterHandle filter_handle = IMGPROC_INVALID_FILTER_HANDLE;

    status = imgproc_config_load(&config_handle, "dummy_filter_config.toml");
    if (status != IMGPROC_SUCCESS) {
        return EXIT_FAILURE;
    }

    int ret = EXIT_FAILURE;

    status = imgproc_filter_load_api(&filter_api, "./libdummy_filter.so");
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_filter_load_api, status);
        goto destroy_config;
    }

    status = filter_api.create(&filter_handle, config_handle);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(filter_api.create, status);
        goto destroy_api;
    }

    if (!test_transform(&filter_api, filter_handle)) {
        goto destroy_filter;
    }

    ret = EXIT_SUCCESS;

destroy_filter:
    filter_api.destroy(filter_handle);

destroy_api:
    imgproc_filter_destroy_api(&filter_api);

destroy_config:
    imgproc_config_destroy(config_handle);

    return ret;
}