#include <imgproc_image_io.h>
#include <imgproc_logger.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>

// 1. 定義顏色通道列舉
typedef enum {
    IMGPROC_CHANNEL_R = 0,
    IMGPROC_CHANNEL_G = 1,
    IMGPROC_CHANNEL_B = 2
} ImgProcChannel;

// 2. 實作新功能：直接修改影像資料陣列
static void imgproc_image_keep_channel(ImgProcImage* image, ImgProcChannel keep_ch) {
    if (!image || !image->data) return;
    
    uint8_t* pixels = (uint8_t*)image->data;
    for (uint32_t y = 0; y < image->height; ++y) {
        for (uint32_t x = 0; x < image->width; ++x) {
            size_t idx = (size_t)y * image->stride + (size_t)x * 3;
            // 根據選擇的通道，將另外兩個通道數值歸零
            if (keep_ch != IMGPROC_CHANNEL_R) pixels[idx + 0] = 0; 
            if (keep_ch != IMGPROC_CHANNEL_G) pixels[idx + 1] = 0; 
            if (keep_ch != IMGPROC_CHANNEL_B) pixels[idx + 2] = 0; 
        }
    }
}

// 輔助函式：取得目錄
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

int main(void) {
    imgproc_logger_use_console();
    imgproc_logger_set_level(IMGPROC_LOGLEVEL_INFO);

    char input_img_path[512];
    char in_dir_buf[256];
    char out_dir_buf[256];
    const char* in_dir = get_target_dir("inputs", in_dir_buf, sizeof(in_dir_buf));
    const char* out_dir = get_target_dir("outputs", out_dir_buf, sizeof(out_dir_buf));

    if (!find_existing_input_image(in_dir, input_img_path, sizeof(input_img_path))) {
        fprintf(stderr, "No image found in '%s/' folder!\n", in_dir);
        return EXIT_FAILURE;
    }

    printf("Found existing test image: '%s'. Processing...\n", input_img_path);

    // Test Red channel
    ImgProcImage* img_r = NULL;
    if (imgproc_image_read(input_img_path, &img_r) == IMGPROC_SUCCESS) {
        imgproc_image_keep_channel(img_r, IMGPROC_CHANNEL_R);
        char out_path[512];
        snprintf(out_path, sizeof(out_path), "%s/test_channel_R.png", out_dir);
        imgproc_image_write_png(out_path, img_r);
        imgproc_image_destroy(img_r);
    }

    // Test Green channel
    ImgProcImage* img_g = NULL;
    if (imgproc_image_read(input_img_path, &img_g) == IMGPROC_SUCCESS) {
        imgproc_image_keep_channel(img_g, IMGPROC_CHANNEL_G);
        char out_path[512];
        snprintf(out_path, sizeof(out_path), "%s/test_channel_G.png", out_dir);
        imgproc_image_write_png(out_path, img_g);
        imgproc_image_destroy(img_g);
    }

    // Test Blue channel
    ImgProcImage* img_b = NULL;
    if (imgproc_image_read(input_img_path, &img_b) == IMGPROC_SUCCESS) {
        imgproc_image_keep_channel(img_b, IMGPROC_CHANNEL_B);
        char out_path[512];
        snprintf(out_path, sizeof(out_path), "%s/test_channel_B.png", out_dir);
        imgproc_image_write_png(out_path, img_b);
        imgproc_image_destroy(img_b);
    }

    printf("Channel splitting test completed.\n");
    return EXIT_SUCCESS;
}
