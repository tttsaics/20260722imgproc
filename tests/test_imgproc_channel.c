#include <imgproc_image_io.h>
#include <imgproc_logger.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>

#include <imgproc_channel.h>

// 輔助函式：自動判斷目標目錄相對於當前工作目錄的路徑（支援從根目錄或 build/ 目錄執行）
static const char* get_target_dir(const char* dir_name, char* out_buf, size_t buf_size) {// 判斷目錄是否存在，並回傳相對路徑
    DIR* dir = opendir("inputs");// 嘗試開啟目錄
    if (dir) {
        closedir(dir);
        snprintf(out_buf, buf_size, "%s", dir_name);// 若目錄存在，直接回傳目錄名稱
    } else {
        snprintf(out_buf, buf_size, "../%s", dir_name);// 若目錄不存在，回傳相對於上層目錄的路徑
    }
    return out_buf;
}

// 輔助函式：搜尋輸入目錄中首張符合格式 (.jpg, .jpeg, .png) 的測試影像
static bool find_existing_input_image(const char* in_dir, char* out_path, size_t path_size) {// 搜尋輸入目錄中首張符合格式 (.jpg, .jpeg, .png) 的測試影像
    DIR* dir = opendir(in_dir);
    if (!dir) return false;

    struct dirent* entry;// 讀取目錄中的每個檔案
    while ((entry = readdir(dir)) != NULL) {// 逐一檢查檔案名稱
        const char* ext = strrchr(entry->d_name, '.');// 取得檔案副檔名
        if (ext && (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0 || strcasecmp(ext, ".png") == 0)) {// 若副檔名符合 .jpg, .jpeg, .png
            snprintf(out_path, path_size, "%s/%s", in_dir, entry->d_name);// 組合完整檔案路徑
            closedir(dir);
            return true;
        }
    }
    closedir(dir);
    return false;
}

int main(void) {
    // 1. 初始化日誌輸出至主控台，並設定日誌等級為 INFO
    imgproc_logger_use_console();
    imgproc_logger_set_level(IMGPROC_LOGLEVEL_INFO);

    char input_img_path[512];
    char in_dir_buf[256];
    char out_dir_buf[256];

    // 2. 取得輸入與輸出目錄位址
    const char* in_dir = get_target_dir("inputs", in_dir_buf, sizeof(in_dir_buf));
    const char* out_dir = get_target_dir("outputs", out_dir_buf, sizeof(out_dir_buf));

    // 3. 搜尋可用的測試影像檔
    if (!find_existing_input_image(in_dir, input_img_path, sizeof(input_img_path))) {// 若找不到任何測試影像，則輸出錯誤訊息並結束程式
        fprintf(stderr, "No image found in '%s/' folder!\n", in_dir);
        return EXIT_FAILURE;
    }

    printf("Found existing test image: '%s'. Processing...\n", input_img_path);

    // 4. 測試紅通道 (Red Channel) 拆分處理
    ImgProcImage* img_r = NULL;
    if (imgproc_image_read(input_img_path, &img_r) == IMGPROC_SUCCESS) {
        imgproc_image_keep_channel(img_r, IMGPROC_CHANNEL_R); // 僅保留 R 通道
        char out_path[512];// 組合輸出檔案路徑
        snprintf(out_path, sizeof(out_path), "%s/test_channel_R.png", out_dir);// 寫出測試結果至 PNG
        imgproc_image_write_png(out_path, img_r);             // 寫出測試結果至 PNG
        imgproc_image_destroy(img_r);                        // 釋放影像記憶體
    }

    // 5. 測試綠通道 (Green Channel) 拆分處理
    ImgProcImage* img_g = NULL;
    if (imgproc_image_read(input_img_path, &img_g) == IMGPROC_SUCCESS) {
        imgproc_image_keep_channel(img_g, IMGPROC_CHANNEL_G); // 僅保留 G 通道
        char out_path[512];
        snprintf(out_path, sizeof(out_path), "%s/test_channel_G.png", out_dir);
        imgproc_image_write_png(out_path, img_g);             // 寫出測試結果至 PNG
        imgproc_image_destroy(img_g);                        // 釋放影像記憶體
    }

    // 6. 測試藍通道 (Blue Channel) 拆分處理
    ImgProcImage* img_b = NULL;
    if (imgproc_image_read(input_img_path, &img_b) == IMGPROC_SUCCESS) {
        imgproc_image_keep_channel(img_b, IMGPROC_CHANNEL_B); // 僅保留 B 通道
        char out_path[512];
        snprintf(out_path, sizeof(out_path), "%s/test_channel_B.png", out_dir);
        imgproc_image_write_png(out_path, img_b);             // 寫出測試結果至 PNG
        imgproc_image_destroy(img_b);                        // 釋放影像記憶體
    }

    printf("Channel splitting test completed.\n");
    return EXIT_SUCCESS;
}

