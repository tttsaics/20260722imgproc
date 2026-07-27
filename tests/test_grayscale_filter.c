/**
 * @file test_grayscale_filter.c
 * @brief Unit test for grayscale_filter plugin.
 */

#include <imgproc_config.h>
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

    printf("Running Grayscale Filter Unit Test...\n");

    const char* plugin_path = "./libgrayscale_filter.so";

    ImgProcFilterApi filter_api;
    memset(&filter_api, 0, sizeof(filter_api));
    ImgProcStatus status = imgproc_filter_load_api(&filter_api, plugin_path);
    if (status != IMGPROC_SUCCESS) {
        fprintf(stderr, "Failed to load grayscale_filter API: %s\n", imgproc_get_status_str(status));
        return EXIT_FAILURE;
    }

    ImgProcFilterHandle filter_handle = IMGPROC_INVALID_FILTER_HANDLE;
    status = filter_api.create(&filter_handle, IMGPROC_INVALID_CONFIG_HANDLE);
    if (status != IMGPROC_SUCCESS) {
        fprintf(stderr, "Failed to create grayscale_filter handle: %s\n", imgproc_get_status_str(status));
        imgproc_filter_destroy_api(&filter_api);
        return EXIT_FAILURE;
    }

    // Create synthetic test image (4x4 RGB image with colored pixels)
    uint32_t width = 4;
    uint32_t height = 4;
    size_t data_size = width * height * 3;
    uint8_t* raw_data = (uint8_t*)malloc(data_size);
    assert(raw_data != NULL);

    for (size_t i = 0; i < width * height; ++i) {
        raw_data[i * 3 + 0] = 200; // R
        raw_data[i * 3 + 1] = 100; // G
        raw_data[i * 3 + 2] = 50;  // B
    }

    ImgProcImage input_img;
    memset(&input_img, 0, sizeof(input_img));
    input_img.data = raw_data;
    input_img.data_size = data_size;
    input_img.width = width;
    input_img.height = height;
    input_img.stride = width * 3;
    input_img.release_fn = free_test_data;

    ImgProcImage* output_img = NULL;
    status = filter_api.transform(filter_handle, &input_img, &output_img);
    if (status != IMGPROC_SUCCESS || output_img == NULL) {
        fprintf(stderr, "Grayscale transform failed: %s\n", imgproc_get_status_str(status));
        free(raw_data);
        filter_api.destroy(filter_handle);
        imgproc_filter_destroy_api(&filter_api);
        return EXIT_FAILURE;
    }

    // Verify grayscale calculation: 0.299 * 200 + 0.587 * 100 + 0.114 * 50 = 59.8 + 58.7 + 5.7 = 124.2 -> 124
    uint8_t* out_data = (uint8_t*)output_img->data;
    bool check_passed = true;
    for (size_t i = 0; i < width * height; ++i) {
        uint8_t r = out_data[i * 3 + 0];
        uint8_t g = out_data[i * 3 + 1];
        uint8_t b = out_data[i * 3 + 2];
        if (r != g || g != b || r != 124) {
            fprintf(stderr, "Pixel mismatch at index %zu: R=%d, G=%d, B=%d (expected 124,124,124)\n", i, r, g, b);
            check_passed = false;
            break;
        }
    }

    imgproc_image_destroy(output_img);
    filter_api.destroy(filter_handle);
    imgproc_filter_destroy_api(&filter_api);

    if (check_passed) {
        printf("Grayscale Filter Unit Test Passed Successfully!\n");
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}
