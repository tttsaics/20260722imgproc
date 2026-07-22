#include "imgproc_logger.h"
#include "imgproc_filter_loader.h"
#include "imgproc_image_io.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TEST_PRINT_CALL_ERROR(expr, status) fprintf(stderr, "%s error: %s\n", #expr, imgproc_get_status_str((status)))

int main() {
    imgproc_logger_use_console();
    imgproc_logger_set_level(IMGPROC_LOGLEVEL_DEBUG);

    const char* input_file  = "../inputs/tw00012141.png";
    const char* output_file = "../outputs/integration_output.png";

    ImgProcStatus status = IMGPROC_SUCCESS;
    ImgProcImage* input_image = NULL;
    ImgProcImage* output_image = NULL;
    ImgProcConfigHandle config_handle = IMGPROC_INVALID_CONFIG_HANDLE;
    ImgProcFilterApi filter_api = {0};
    ImgProcFilterHandle filter_handle = IMGPROC_INVALID_FILTER_HANDLE;

    // 1. Read the input image from inputs/
    status = imgproc_image_read(input_file, &input_image);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_image_read, status);
        return EXIT_FAILURE;
    }

    // 2. Load config and filter API
    status = imgproc_config_load(&config_handle, "resize_filter_config.toml");
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_config_load, status);
        imgproc_image_destroy(input_image);
        return EXIT_FAILURE;
    }

    status = imgproc_filter_load_api(&filter_api, "./libresize_filter.so");
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_filter_load_api, status);
        imgproc_image_destroy(input_image);
        imgproc_config_destroy(config_handle);
        return EXIT_FAILURE;
    }

    status = filter_api.create(&filter_handle, config_handle);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(filter_api.create, status);
        imgproc_filter_destroy_api(&filter_api);
        imgproc_image_destroy(input_image);
        imgproc_config_destroy(config_handle);
        return EXIT_FAILURE;
    }

    // 3. Run transform
    status = filter_api.transform(filter_handle, input_image, &output_image);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(filter_api.transform, status);
        imgproc_image_destroy(input_image);
        filter_api.destroy(filter_handle);
        imgproc_filter_destroy_api(&filter_api);
        imgproc_config_destroy(config_handle);
        return EXIT_FAILURE;
    }

    // Since transform frees input_image->data but not input_image struct itself, we destroy it now
    imgproc_image_destroy(input_image);

    // 4. Write the output image to outputs/
    status = imgproc_image_write_png(output_file, output_image);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_image_write_png, status);
        imgproc_image_destroy(output_image);
        filter_api.destroy(filter_handle);
        imgproc_filter_destroy_api(&filter_api);
        imgproc_config_destroy(config_handle);
        return EXIT_FAILURE;
    }

    // 5. Clean up
    imgproc_image_destroy(output_image);
    filter_api.destroy(filter_handle);
    imgproc_filter_destroy_api(&filter_api);
    imgproc_config_destroy(config_handle);

    printf("Integration test passed successfully!\n");
    return EXIT_SUCCESS;
}
