#include "imgproc_logger.h"
#include "imgproc_config.h"
#include "imgproc_filter_loader.h"
#include "imgproc_image_io.h"

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

static const char* find_config_path(char* out_buf, size_t buf_size) {
    const char* candidates[] = {
        "resize_filter_config.toml",
        "./build/resize_filter_config.toml",
        "./tests/test_resize_config.toml",
        "../tests/test_resize_config.toml"
    };
    for (size_t i = 0; i < sizeof(candidates)/sizeof(candidates[0]); ++i) {
        FILE* f = fopen(candidates[i], "r");
        if (f) {
            fclose(f);
            snprintf(out_buf, buf_size, "%s", candidates[i]);
            return out_buf;
        }
    }
    snprintf(out_buf, buf_size, "resize_filter_config.toml");
    return out_buf;
}

int main(void) {
    imgproc_logger_use_console();
    imgproc_logger_set_level(IMGPROC_LOGLEVEL_DEBUG);

    char in_dir_buf[256];
    char out_dir_buf[256];
    char input_file[512];
    char output_file[512];
    char plugin_path_buf[256];
    char config_path_buf[256];

    const char* in_dir = get_target_dir("inputs", in_dir_buf, sizeof(in_dir_buf));
    const char* out_dir = get_target_dir("outputs", out_dir_buf, sizeof(out_dir_buf));
    const char* plugin_path = find_plugin_path(plugin_path_buf, sizeof(plugin_path_buf));
    const char* config_file = find_config_path(config_path_buf, sizeof(config_path_buf));

    if (!find_input_image(in_dir, input_file, sizeof(input_file))) {
        fprintf(stderr, "No image found in '%s/' directory.\n", in_dir);
        return EXIT_FAILURE;
    }

    snprintf(output_file, sizeof(output_file), "%s/integration_output.png", out_dir);

    ImgProcStatus status = IMGPROC_SUCCESS;
    ImgProcImage* input_image = NULL;
    ImgProcImage* output_image = NULL;
    ImgProcConfigHandle config_handle = IMGPROC_INVALID_CONFIG_HANDLE;
    ImgProcFilterApi filter_api;
    memset(&filter_api, 0, sizeof(filter_api));
    ImgProcFilterHandle filter_handle = IMGPROC_INVALID_FILTER_HANDLE;

    // 1. Read the input image from inputs/
    status = imgproc_image_read(input_file, &input_image);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_image_read, status);
        return EXIT_FAILURE;
    }

    // 2. Load config and filter API
    status = imgproc_config_load(&config_handle, config_file);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_config_load, status);
        imgproc_image_destroy(input_image);
        return EXIT_FAILURE;
    }

    status = imgproc_filter_load_api(&filter_api, plugin_path);
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
