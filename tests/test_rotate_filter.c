/**
 * @file test_rotate_filter.c
 * @brief Unit test for rotate_filter plugin with 4-channel RGBA support.
 */

#include <imgproc_filter_loader.h>
#include <imgproc_image_io.h>
#include <imgproc_logger.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void free_test_data(ImgProcImage* img) {
    if (img && img->data) {
        free(img->data);
        img->data = NULL;
    }
}

int main(void) {
    imgproc_logger_use_console();
    imgproc_logger_set_level(IMGPROC_LOGLEVEL_INFO);

    const char* plugin_path = "./librotate_filter.so";

    ImgProcFilterApi filter_api;
    memset(&filter_api, 0, sizeof(filter_api));

    ImgProcStatus status = imgproc_filter_load_api(&filter_api, plugin_path);
    if (status != IMGPROC_SUCCESS) {
        fprintf(stderr, "Failed to load rotate_filter API: %s\n", imgproc_get_status_str(status));
        return EXIT_FAILURE;
    }

    ImgProcFilterHandle filter_handle = IMGPROC_INVALID_FILTER_HANDLE;
    status = filter_api.create(&filter_handle, IMGPROC_INVALID_CONFIG_HANDLE);
    if (status != IMGPROC_SUCCESS) {
        fprintf(stderr, "Failed to create rotate_filter handle: %s\n", imgproc_get_status_str(status));
        imgproc_filter_destroy_api(&filter_api);
        return EXIT_FAILURE;
    }

    // Create synthetic 2x3 RGBA test image
    uint32_t width = 2;
    uint32_t height = 3;
    size_t data_size = width * height * 4;
    uint8_t* raw_data = (uint8_t*)malloc(data_size);
    assert(raw_data != NULL);

    memset(raw_data, 128, data_size);

    ImgProcImage input_img;
    memset(&input_img, 0, sizeof(input_img));
    input_img.data = raw_data;
    input_img.data_size = data_size;
    input_img.width = width;
    input_img.height = height;
    input_img.stride = width * 4;
    input_img.channels = 4;
    input_img.release_fn = free_test_data;

    ImgProcImage* output_img = NULL;
    status = filter_api.transform(filter_handle, &input_img, &output_img);
    if (status != IMGPROC_SUCCESS || output_img == NULL) {
        fprintf(stderr, "Rotate transform failed: %s\n", imgproc_get_status_str(status));
        filter_api.destroy(filter_handle);
        imgproc_filter_destroy_api(&filter_api);
        return EXIT_FAILURE;
    }

    assert(output_img->channels == 4);
    assert(output_img->data != NULL);

    imgproc_image_destroy(output_img);
    filter_api.destroy(filter_handle);
    imgproc_filter_destroy_api(&filter_api);

    printf("RotateFilter unit test PASSED successfully!\n");
    return EXIT_SUCCESS;
}
