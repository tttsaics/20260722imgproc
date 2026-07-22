#include "imgproc_logger.h"
#include "imgproc_filter_loader.h"
#include "imgproc_image_io.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define TEST_PRINT_CALL_ERROR(expr, status) fprintf(stderr, "%s error: %s\n", #expr, imgproc_get_status_str((status)))

static void release_image(ImgProcImage* img) {
    if (img->data) {
        free(img->data);
        img->data = NULL;
    }
}

static bool init_image(ImgProcImage* img) {
    img->width = 1280;
    img->height = 720;
    img->stride = 4096;
    img->data_size = img->stride * img->height;

    img->data = malloc(img->data_size);
    if (!img->data) {
        return false;
    }

    img->release_fn = release_image;

    // Fill with a gradient pattern to test pixel interpolation
    unsigned char* ptr = (unsigned char*)img->data;
    for (uint32_t y = 0; y < img->height; ++y) {
        for (uint32_t x = 0; x < img->width; ++x) {
            ptr[y * img->stride + x * 3 + 0] = (unsigned char)((x * 255) / img->width);
            ptr[y * img->stride + x * 3 + 1] = (unsigned char)((y * 255) / img->height);
            ptr[y * img->stride + x * 3 + 2] = (unsigned char)((x + y) % 256);
        }
    }

    return true;
}

static bool test_transform(const ImgProcFilterApi* api, ImgProcFilterHandle filter_handle) {
    ImgProcStatus status = IMGPROC_SUCCESS;
    ImgProcImage input_image;
    ImgProcImage* output_image = NULL;

    if (!init_image(&input_image)) {
        fprintf(stderr, "Unable to init an image.\n");
        return false;
    }

    uint32_t expected_width = 640;
    uint32_t expected_height = 360;

    status = api->transform(filter_handle, &input_image, &output_image);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(api->transform, status);
        return false;
    }

    if (!output_image) {
        fprintf(stderr, "Output image is NULL.\n");
        return false;
    }

    if (output_image->width != expected_width || output_image->height != expected_height) {
        fprintf(stderr, "Resizing dimensions incorrect! Expected: %dx%d, Actual: %dx%d\n",
                expected_width, expected_height, output_image->width, output_image->height);
        imgproc_image_destroy(output_image);
        return false;
    }

    printf("Successfully resized image from 1280x720 to %dx%d!\n", output_image->width, output_image->height);

    imgproc_image_destroy(output_image);
    return true;
}

int main() {
    imgproc_logger_use_console();
    imgproc_logger_set_level(IMGPROC_LOGLEVEL_DEBUG);

    ImgProcStatus status = IMGPROC_SUCCESS;
    ImgProcConfigHandle config_handle = IMGPROC_INVALID_CONFIG_HANDLE;
    ImgProcFilterApi filter_api = {0};
    ImgProcFilterHandle filter_handle = IMGPROC_INVALID_FILTER_HANDLE;

    status = imgproc_config_load(&config_handle, "resize_filter_config.toml");
    if (status != IMGPROC_SUCCESS) {
        return EXIT_FAILURE;
    }

    int ret = EXIT_FAILURE;

    status = imgproc_filter_load_api(&filter_api, "./libresize_filter.so");
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
