#include <imgproc_image_io.h>
#include <imgproc_logger.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#define TEST_PRINT_CALL_ERROR(expr, status) fprintf(stderr, "%s error: %s\n", #expr, imgproc_get_status_str((status)))

static const char* get_target_dir(const char* dir_name, char* out_buf, size_t buf_size) {
    DIR* dir = opendir(dir_name);
    if (dir) {
        closedir(dir);
        snprintf(out_buf, buf_size, "%s", dir_name);
        return out_buf;
    }
    char parent_path[256];
    snprintf(parent_path, sizeof(parent_path), "../%s", dir_name);
    dir = opendir(parent_path);
    if (dir) {
        closedir(dir);
        snprintf(out_buf, buf_size, "../%s", dir_name);
        return out_buf;
    }
    snprintf(out_buf, buf_size, "%s", dir_name);
    return out_buf;
}

// Find any existing image (.jpg, .png, .jpeg) in the target inputs directory
static bool find_existing_input_image(const char* in_dir, char* out_path, size_t path_size) {
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

static bool test_write_and_read_image(void) {
    ImgProcStatus status = IMGPROC_SUCCESS;

    char input_img_path[512];
    char output_png_path[512];
    char output_jpg_path[512];

    char in_dir_buf[256];
    char out_dir_buf[256];
    const char* in_dir = get_target_dir("inputs", in_dir_buf, sizeof(in_dir_buf));
    const char* out_dir = get_target_dir("outputs", out_dir_buf, sizeof(out_dir_buf));

    // Automatically scan inputs/ directory for ANY existing image!
    if (!find_existing_input_image(in_dir, input_img_path, sizeof(input_img_path))) {
        fprintf(stderr, "No image found in '%s/' folder! Please place a test image (.jpg/.png) in inputs/.\n", in_dir);
        return false;
    }

    printf("Found existing test image: '%s'. Testing IO read & write...\n", input_img_path);

    snprintf(output_png_path, sizeof(output_png_path), "%s/test_io_output.png", out_dir);
    snprintf(output_jpg_path, sizeof(output_jpg_path), "%s/test_io_output.jpg", out_dir);

    // 1. Read existing image from inputs/ directory directly
    ImgProcImage* loaded_img = NULL;
    status = imgproc_image_read(input_img_path, &loaded_img);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_image_read, status);
        return false;
    }

    if (!loaded_img || !loaded_img->data) {
        fprintf(stderr, "Loaded image or data pointer is NULL.\n");
        return false;
    }

    printf("Successfully read '%s' (Width=%u, Height=%u, Size=%zu bytes)\n",
           input_img_path, loaded_img->width, loaded_img->height, loaded_img->data_size);

    // 2. Test re-writing image to outputs/ test paths
    status = imgproc_image_write_png(output_png_path, loaded_img);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_image_write_png, status);
        imgproc_image_destroy(loaded_img);
        return false;
    }

    status = imgproc_image_write_jpg(output_jpg_path, loaded_img, 90);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_image_write_jpg, status);
        imgproc_image_destroy(loaded_img);
        return false;
    }

    // 3. Destroy image and release memory
    status = imgproc_image_destroy(loaded_img);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_image_destroy, status);
        return false;
    }

    return true;
}

static bool test_invalid_arguments(void) {
    ImgProcStatus status = IMGPROC_SUCCESS;
    ImgProcImage* img = NULL;

    // Test NULL filename on read
    status = imgproc_image_read(NULL, &img);
    if (status != IMGPROC_ERROR_INVALID_ARG) {
        fprintf(stderr, "test_invalid_arguments: expected INVALID_ARG for NULL filename in read.\n");
        return false;
    }

    // Test NULL image pointer on read
    status = imgproc_image_read("inputs/test.jpg", NULL);
    if (status != IMGPROC_ERROR_INVALID_ARG) {
        fprintf(stderr, "test_invalid_arguments: expected INVALID_ARG for NULL image pointer in read.\n");
        return false;
    }

    // Test NULL image on write_jpg
    status = imgproc_image_write_jpg("outputs/test.jpg", NULL, 90);
    if (status != IMGPROC_ERROR_INVALID_ARG) {
        fprintf(stderr, "test_invalid_arguments: expected INVALID_ARG for NULL image in write_jpg.\n");
        return false;
    }

    // Test invalid quality (< 1 or > 100)
    ImgProcImage dummy_img = {0};
    dummy_img.data = &img; // Non-null dummy data
    dummy_img.width = 10;
    dummy_img.height = 10;
    status = imgproc_image_write_jpg("outputs/test.jpg", &dummy_img, 0);
    if (status != IMGPROC_ERROR_INVALID_ARG) {
        fprintf(stderr, "test_invalid_arguments: expected INVALID_ARG for quality=0.\n");
        return false;
    }

    // Test NULL image on destroy
    status = imgproc_image_destroy(NULL);
    if (status != IMGPROC_ERROR_INVALID_ARG) {
        fprintf(stderr, "test_invalid_arguments: expected INVALID_ARG for NULL image in destroy.\n");
        return false;
    }

    return true;
}

int main(void) {
    imgproc_logger_use_console();
    imgproc_logger_set_level(IMGPROC_LOGLEVEL_INFO);

    if (!test_write_and_read_image()) {
        return EXIT_FAILURE;
    }

    if (!test_invalid_arguments()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
