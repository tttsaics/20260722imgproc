/**
 * @file test_mirror_filter.c
 * @brief Unit test for mirror_filter plugin.
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

    printf("Running Mirror Filter Unit Test...\n");

    const char* plugin_path = "./libmirror_filter.so";

    ImgProcFilterApi filter_api;
    memset(&filter_api, 0, sizeof(filter_api));
    ImgProcStatus status = imgproc_filter_load_api(&filter_api, plugin_path);
    if (status != IMGPROC_SUCCESS) {
        fprintf(stderr, "Failed to load mirror_filter API: %s\n", imgproc_get_status_str(status));
        return EXIT_FAILURE;
    }

    ImgProcFilterHandle filter_handle = IMGPROC_INVALID_FILTER_HANDLE;
    status = filter_api.create(&filter_handle, IMGPROC_INVALID_CONFIG_HANDLE);
    if (status != IMGPROC_SUCCESS) {
        fprintf(stderr, "Failed to create mirror_filter handle: %s\n", imgproc_get_status_str(status));
        imgproc_filter_destroy_api(&filter_api);
        return EXIT_FAILURE;
    }

    // Create a 2x2 RGB image with unique pixel values
    // Pixel (0,0): (10, 20, 30)   Pixel (1,0): (40, 50, 60)
    // Pixel (0,1): (70, 80, 90)   Pixel (1,1): (100, 110, 120)
    uint32_t width = 2;
    uint32_t height = 2;
    size_t data_size = width * height * 3;
    uint8_t* raw_data = (uint8_t*)malloc(data_size);
    assert(raw_data != NULL);

    uint8_t init_pixels[4][3] = {
        {10, 20, 30},    {40, 50, 60},
        {70, 80, 90},    {100, 110, 120}
    };
    memcpy(raw_data, init_pixels, sizeof(init_pixels));

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
        fprintf(stderr, "Mirror transform failed: %s\n", imgproc_get_status_str(status));
        free(raw_data);
        filter_api.destroy(filter_handle);
        imgproc_filter_destroy_api(&filter_api);
        return EXIT_FAILURE;
    }

    // Default mode is horizontal mirror (flip_h)
    // Expected result:
    // Pixel (0,0) should be original (1,0) -> (40, 50, 60)
    // Pixel (1,0) should be original (0,0) -> (10, 20, 30)
    // Pixel (0,1) should be original (1,1) -> (100, 110, 120)
    // Pixel (1,1) should be original (0,1) -> (70, 80, 90)
    uint8_t expected[4][3] = {
        {40, 50, 60},    {10, 20, 30},
        {100, 110, 120},  {70, 80, 90}
    };

    uint8_t* out_data = (uint8_t*)output_img->data;
    bool check_passed = true;
    for (size_t i = 0; i < 4; ++i) {
        if (out_data[i * 3 + 0] != expected[i][0] ||
            out_data[i * 3 + 1] != expected[i][1] ||
            out_data[i * 3 + 2] != expected[i][2]) {
            fprintf(stderr, "Pixel mismatch at idx %zu: got (%d,%d,%d), expected (%d,%d,%d)\n",
                    i, out_data[i * 3 + 0], out_data[i * 3 + 1], out_data[i * 3 + 2],
                    expected[i][0], expected[i][1], expected[i][2]);
            check_passed = false;
            break;
        }
    }

    imgproc_image_destroy(output_img);
    free_test_data(&input_img);
    filter_api.destroy(filter_handle);
    imgproc_filter_destroy_api(&filter_api);

    if (check_passed) {
        printf("Mirror Filter Unit Test Passed Successfully!\n");
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}
