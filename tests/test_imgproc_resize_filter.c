/**
 * @file test_imgproc_resize_filter.c
 * @brief Integration test for resize_filter plugin with imgproc_image_io.
 */

#include <imgproc_config.h>
#include <imgproc_filter_loader.h>
#include <imgproc_image_io.h>
#include <imgproc_logger.h>

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_PRINT_CALL_ERROR(expr, status) fprintf(stderr, "%s error: %s\n", #expr, imgproc_get_status_str((status)))

static const char* get_target_dir(const char* dir_name, char* out_buf, size_t buf_size) {
    DIR* dir = opendir("inputs");
    if (dir) {
        closedir(dir);
        snprintf(out_buf, buf_size, "%s", dir_name);
    } else {
        snprintf(out_buf, buf_size, "../%s", dir_name);
    }
    return out_buf;
}

static bool find_input_image(const char* in_dir, char* out_path, size_t path_size) {
    DIR* dir = opendir(in_dir);
    if (!dir) return false;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        const char* ext = strrchr(entry->d_name, '.');
        if (ext && (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0 || strcasecmp(ext, ".png") == 0)) {
            snprintf(out_path, path_size, "%s/%s", in_dir, entry->d_name);
            closedir(dir);
            return true;
        }
    }
    closedir(dir);
    return false;
}

static const char* find_plugin_path(char* out_buf, size_t buf_size) {
    FILE* f = fopen("./libresize_filter.so", "r");
    if (f) {
        fclose(f);
        snprintf(out_buf, buf_size, "./libresize_filter.so");
        return out_buf;
    }
    f = fopen("./build/libresize_filter.so", "r");
    if (f) {
        fclose(f);
        snprintf(out_buf, buf_size, "./build/libresize_filter.so");
        return out_buf;
    }
    snprintf(out_buf, buf_size, "./libresize_filter.so");
    return out_buf;
}

static bool test_resize_filter_integration(void) {
    ImgProcStatus status = IMGPROC_SUCCESS;

    char in_dir_buf[256];
    char out_dir_buf[256];
    char input_img_path[512];
    char output_jpg_path[512];
    char output_png_path[512];
    char plugin_path_buf[256];

    const char* in_dir = get_target_dir("inputs", in_dir_buf, sizeof(in_dir_buf));
    const char* out_dir = get_target_dir("outputs", out_dir_buf, sizeof(out_dir_buf));
    const char* plugin_path = find_plugin_path(plugin_path_buf, sizeof(plugin_path_buf));

    if (!find_input_image(in_dir, input_img_path, sizeof(input_img_path))) {
        fprintf(stderr, "No image found in '%s/' directory.\n", in_dir);
        return false;
    }

    snprintf(output_jpg_path, sizeof(output_jpg_path), "%s/test_resize_out.jpg", out_dir);
    snprintf(output_png_path, sizeof(output_png_path), "%s/test_resize_out.png", out_dir);

    // 1. Load Resize Filter Plugin API
    ImgProcFilterApi filter_api;
    memset(&filter_api, 0, sizeof(filter_api));
    status = imgproc_filter_load_api(&filter_api, plugin_path);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_filter_load_api, status);
        return false;
    }

    // 2. Instantiate Resize Filter Handle
    ImgProcFilterHandle filter_handle = IMGPROC_INVALID_FILTER_HANDLE;
    status = filter_api.create(&filter_handle, IMGPROC_INVALID_CONFIG_HANDLE);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(filter_api.create, status);
        imgproc_filter_destroy_api(&filter_api);
        return false;
    }

    // 3. Read image using imgproc_image_read
    ImgProcImage* input_img = NULL;
    status = imgproc_image_read(input_img_path, &input_img);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_image_read, status);
        filter_api.destroy(filter_handle);
        imgproc_filter_destroy_api(&filter_api);
        return false;
    }

    printf("Original Image: %ux%u (stride %u)\n", input_img->width, input_img->height, input_img->stride);

    // 4. Apply Resize Filter Transformation
    ImgProcImage* output_img = NULL;
    status = filter_api.transform(filter_handle, input_img, &output_img);
    if (status != IMGPROC_SUCCESS || !output_img) {
        TEST_PRINT_CALL_ERROR(filter_api.transform, status);
        imgproc_image_destroy(input_img);
        filter_api.destroy(filter_handle);
        imgproc_filter_destroy_api(&filter_api);
        return false;
    }

    printf("Resized Image:  %ux%u (stride %u)\n", output_img->width, output_img->height, output_img->stride);

    // Destroy input_img struct shell after transform
    imgproc_image_destroy(input_img);

    // 5. Save resized output image to outputs/ folder using imgproc_image_write
    status = imgproc_image_write_jpg(output_jpg_path, output_img, 90);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_image_write_jpg, status);
    }

    status = imgproc_image_write_png(output_png_path, output_img);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_image_write_png, status);
    }

    // 6. Destroy output image memory and unload filter
    status = imgproc_image_destroy(output_img);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_image_destroy, status);
    }

    filter_api.destroy(filter_handle);
    imgproc_filter_destroy_api(&filter_api);

    return true;
}

int main(void) {
    imgproc_logger_use_console();
    imgproc_logger_set_level(IMGPROC_LOGLEVEL_INFO);

    printf("Running Resize Filter Integration Test...\n");

    if (!test_resize_filter_integration()) {
        fprintf(stderr, "Resize Filter Integration Test Failed!\n");
        return EXIT_FAILURE;
    }

    printf("Resize Filter Integration Test Passed Successfully!\n");
    return EXIT_SUCCESS;
}
