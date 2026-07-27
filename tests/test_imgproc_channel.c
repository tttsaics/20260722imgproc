#include <imgproc_image_io.h>
#include <imgproc_logger.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>

#include <imgproc_channel.h>

// 輔助函式：取得目錄
static const char* get_target_dir(const char* dir_name, char* out_buf, size_t buf_size) {//這個函式會嘗試在當前目錄和上一層目錄中尋找指定的資料夾，並將找到的路徑寫入 out_buf 中。如果找不到，則返回原始的 dir_name。
    DIR* dir = opendir(dir_name);// 嘗試打開當前目錄中的資料夾
    if (dir) {
        closedir(dir);
        snprintf(out_buf, buf_size, "%s", dir_name);// 如果在當前目錄中找到資料夾，則將其路徑寫入 out_buf 並返回
        return out_buf;
    }
    char parent_path[256];// 如果在當前目錄中找不到資料夾，則嘗試在上一層目錄中尋找
    snprintf(parent_path, sizeof(parent_path), "../%s", dir_name);// 構建上一層目錄的路徑
    dir = opendir(parent_path);// 嘗試打開上一層目錄中的資料夾
    if (dir) {
        closedir(dir);
        snprintf(out_buf, buf_size, "../%s", dir_name);
        return out_buf;
    }
    snprintf(out_buf, buf_size, "%s", dir_name);// 如果在當前目錄和上一層目錄中都找不到資料夾，則返回原始的 dir_name
    return out_buf;
}

// 輔助函式：尋找現有的輸入圖像
static bool find_existing_input_image(const char* in_dir, char* out_path, size_t path_size) {// 這個函式會在指定的資料夾中尋找第一個存在的圖像檔案，並將其路徑寫入 out_path 中。如果找不到，則返回 false。
    DIR* dir = opendir(in_dir);// 嘗試打開指定的資料夾
    if (!dir) return false;

    struct dirent* entry;// 迭代資料夾中的每個檔案
    while ((entry = readdir(dir)) != NULL) {// 讀取資料夾中的每個檔案
        const char* ext = strrchr(entry->d_name, '.');// 找到檔案名稱中最後一個點的位置，以獲取檔案的副檔名
        if (ext && (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0 || strcasecmp(ext, ".png") == 0)) {
            snprintf(out_path, path_size, "%s/%s", in_dir, entry->d_name);// 如果找到圖像檔案，則將其完整路徑寫入 out_path 並返回 true
            closedir(dir);
            return true;
        }
    }
    closedir(dir);
    return false;
}

int main(void) {
    imgproc_logger_use_console();
    imgproc_logger_set_level(IMGPROC_LOGLEVEL_INFO);// 設定日誌記錄器的日誌級別為資訊級別，這樣可以在控制台上看到資訊、警告和錯誤訊息。

    char input_img_path[512];
    char in_dir_buf[256];
    char out_dir_buf[256];
    const char* in_dir = get_target_dir("inputs", in_dir_buf, sizeof(in_dir_buf));// 嘗試在當前目錄和上一層目錄中尋找 "inputs" 資料夾，並將找到的路徑寫入 in_dir_buf 中。如果找不到，則返回原始的 "inputs"。
    const char* out_dir = get_target_dir("outputs", out_dir_buf, sizeof(out_dir_buf));// 嘗試在當前目錄和上一層目錄中尋找 "outputs" 資料夾，並將找到的路徑寫入 out_dir_buf 中。如果找不到，則返回原始的 "outputs"。

    if (!find_existing_input_image(in_dir, input_img_path, sizeof(input_img_path))) {// 嘗試在指定的輸入資料夾中尋找第一個存在的圖像檔案，並將其路徑寫入 input_img_path 中。如果找不到，則輸出錯誤訊息並返回失敗。
        fprintf(stderr, "No image found in '%s/' folder!\n", in_dir);
        return EXIT_FAILURE;
    }

    printf("Found existing test image: '%s'. Processing...\n", input_img_path);

    // Test Red channel
    ImgProcImage* img_r = NULL;// 嘗試讀取輸入圖像，並將其指針存儲在 img_r 中。
    if (imgproc_image_read(input_img_path, &img_r) == IMGPROC_SUCCESS) {// 如果成功讀取圖像，則保留紅色通道，將其他通道設置為零，並將結果保存為 PNG 檔案。
        imgproc_image_keep_channel(img_r, IMGPROC_CHANNEL_R);// 保留紅色通道，將其他通道設置為零
        char out_path[512];// 定義一個字元陣列來存儲輸出檔案的路徑
        snprintf(out_path, sizeof(out_path), "%s/test_channel_R.png", out_dir);// 將輸出檔案命名為 "test_channel_R.png"，並將其保存到指定的輸出資料夾中
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
