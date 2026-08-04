#include <imgproc_channel.h>
#include <imgproc_config.h>
#include <imgproc_filter_loader.h>
#include <imgproc_image_io.h>
#include <imgproc_logger.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

static void print_usage(const char* prog_name) {
    // 輸出命令列選項與使用說明。
    std::cout << "Usage: " << prog_name << " [options]\n"
              << "Options:\n"
              << "  -i, --input <file>     Path to input image (JPG/PNG)\n"
              << "  -o, --output <file>    Path to output image (JPG/PNG)\n"
              << "  -f, --filter <so_path> Path to filter dynamic plugin (.so) [Can be specified multiple times]\n"
              << "  -c, --config <toml>    Path to TOML config file [Matches corresponding -f filter]\n"
              << "  -ch, --channel <R|G|B> Extract RGB channel (R, G, or B)\n"
              << "  -h, --help             Show this help message\n\n"
              << "If no arguments are provided, automatically runs batch processing on inputs/ folder.\n";
}

static ImgProcStatus process_single_image(const std::string& input_path,
                                         const std::string& output_path,
                                         const std::vector<std::string>& filter_paths,
                                         const std::vector<std::string>& config_paths,
                                         const std::string& channel_str) {
    ImgProcStatus status = IMGPROC_SUCCESS;

    // 1. 入口參數邊界檢驗：若指定 -ch，檢查是否為合法通道 (R, G, B)。
    if (!channel_str.empty()) {
        if(channel_str.length() != 1) {
            IMGPROC_LOG_ERROR("Invalid channel parameter '%s'. Must be a single character: R, G, or B.", channel_str.c_str());
            return IMGPROC_ERROR_INVALID_ARG;
        }
        char ch_upper = static_cast<char>(std::toupper(channel_str[0]));
        if (ch_upper != 'R' && ch_upper != 'G' && ch_upper != 'B') {
            IMGPROC_LOG_ERROR("Invalid channel parameter '%s'. Must be R, G, or B.", channel_str.c_str());
            return IMGPROC_ERROR_INVALID_ARG;
        }
    }

    // 2. 檢驗輸出副檔名是否為支援的格式 (.jpg, .jpeg, .png)。
    bool is_jpeg = false;
    std::string_view sv(output_path);
    size_t dot_pos = sv.rfind('.');
    if (dot_pos != std::string_view::npos) {
        std::string_view ext = sv.substr(dot_pos);
        if (ext == ".jpg" || ext == ".JPG" || ext == ".jpeg" || ext == ".JPEG") {
            is_jpeg = true;
        } else if (ext != ".png" && ext != ".PNG") {
            IMGPROC_LOG_WARN("Unsupported output file extension '%.*s'. Fallback to default PNG format.",
                             static_cast<int>(ext.length()), ext.data());
        }
    }

    // 3. 透過 Image IO 模組讀取輸入圖片。
    ImgProcImage* current_img = nullptr;
    status = imgproc_image_read(input_path.c_str(), &current_img);
    if (status != IMGPROC_SUCCESS) {
        IMGPROC_LOG_ERROR("Failed to read input image '%s'.", input_path.c_str());
        return status;
    }

    // 追蹤所有已載入的外掛 API 與控制句柄，確保 `.so` 在圖片完全解構前維持有效。
    std::vector<ImgProcFilterApi> loaded_apis;
    std::vector<ImgProcFilterHandle> active_handles;
    loaded_apis.reserve(filter_paths.size());
    active_handles.reserve(filter_paths.size());

    // 4. 按順序鏈式執行多外掛流水線 (Filter Pipeline)。
    for (size_t i = 0; i < filter_paths.size(); ++i) {
        const std::string& f_path = filter_paths[i];
        static const std::string empty_str = "";
        const std::string& c_path = (i < config_paths.size()) ? config_paths[i] : empty_str;

        // 若提供配對的 TOML 設定檔，載入設定檔控制句柄。
        ImgProcConfigHandle config_handle = IMGPROC_INVALID_CONFIG_HANDLE;
        if (!c_path.empty()) {
            status = imgproc_config_load(&config_handle, c_path.c_str());
            if (status != IMGPROC_SUCCESS) {
                IMGPROC_LOG_ERROR("Failed to load config '%s' for filter [%zu].", c_path.c_str(), i);
                break;
            }
        }

        // 動態載入 .so 外掛的 API 函數指標（使用 C++11 {} 值初始化自動安全歸零）。
        ImgProcFilterApi filter_api{};
        status = imgproc_filter_load_api(&filter_api, f_path.c_str());
        if (status != IMGPROC_SUCCESS) {
            IMGPROC_LOG_ERROR("Failed to load filter plugin '%s' [%zu].", f_path.c_str(), i);
            // 載入失敗早退分支 (Early Exit)：必須銷毀已配置的 config_handle 防範記憶體洩漏。
            if (config_handle != IMGPROC_INVALID_CONFIG_HANDLE) imgproc_config_destroy(config_handle);
            break;
        }

        // 建立外掛控制句柄，創建完成後立即釋放單次使用的 Config 句柄。
        ImgProcFilterHandle filter_handle = IMGPROC_INVALID_FILTER_HANDLE;
        status = filter_api.create(&filter_handle, config_handle);
        if (config_handle != IMGPROC_INVALID_CONFIG_HANDLE) {
            imgproc_config_destroy(config_handle);
        }

        if (status != IMGPROC_SUCCESS) {
            IMGPROC_LOG_ERROR("Failed to create filter handle for plugin '%s' [%zu].", f_path.c_str(), i);
            imgproc_filter_destroy_api(&filter_api);
            break;
        }

        // 將成功的 API 與 Handle 存入陣列，維護動態庫代碼段 (Code Segment) 生命週期。
        loaded_apis.emplace_back(filter_api);
        active_handles.emplace_back(filter_handle);

        // 執行影像變換，將 current_img 作為輸入，取得產出的 next_img。
        ImgProcImage* next_img = nullptr;
        status = filter_api.transform(filter_handle, current_img, &next_img);

        if (status != IMGPROC_SUCCESS || !next_img) {
            IMGPROC_LOG_ERROR("Filter transformation failed for plugin '%s' [%zu].", f_path.c_str(), i);
            break;
        }

        ImgProcImage* prev_img = current_img;
        current_img = next_img;

        // 舊圖片資料記憶體已由 transform 內部呼叫 release_fn 釋放，此處僅刪除外層 struct 外殼。
        if (prev_img && prev_img != current_img) {
            delete prev_img;
        }
    }

    // 5. 若指定 -ch 參數，執行指定色彩通道提取。
    if (status == IMGPROC_SUCCESS && !channel_str.empty()) {
        ImgProcChannel ch = IMGPROC_CHANNEL_R;
        if (channel_str == "G" || channel_str == "g") ch = IMGPROC_CHANNEL_G;
        else if (channel_str == "B" || channel_str == "b") ch = IMGPROC_CHANNEL_B;

        imgproc_image_keep_channel(current_img, ch);
    }

    // 6. 依據副檔名透過 Image IO 模組將最終影像寫回硬碟。
    if (status == IMGPROC_SUCCESS) {
        if (is_jpeg) {
            status = imgproc_image_write_jpg(output_path.c_str(), current_img, 90);
        } else {
            status = imgproc_image_write_png(output_path.c_str(), current_img);
        }
    }

    // 7. 先銷毀最終影像物件（觸發 release_fn 閉包），必須在卸載 .so 動態庫之前完成！
    if (current_img) {
        imgproc_image_destroy(current_img);
    }

    // 8. 最後才銷毀外掛 Handle 並以 dlclose 卸載 .so 動態庫，確保無記憶體與 Handle 洩漏。
    for (size_t i = 0; i < loaded_apis.size(); ++i) {
        if (i < active_handles.size() && active_handles[i]) {
            loaded_apis[i].destroy(active_handles[i]);
        }
        imgproc_filter_destroy_api(&loaded_apis[i]);
    }

    return status;
}

int main(int argc, char* argv[]) {
    // 啟用控制台日誌輸出，設定預設 Log 層級。
    imgproc_logger_use_console();
    imgproc_logger_set_level(IMGPROC_LOGLEVEL_INFO);

    std::string input_path;
    std::string output_path;
    std::vector<std::string> filter_paths;
    std::vector<std::string> config_paths;
    std::string channel_str;

    // 解析 CLI 命令列參數。
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        } else if ((arg == "-i" || arg == "--input") && i + 1 < argc) {
            input_path = argv[++i];
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            output_path = argv[++i];
        } else if ((arg == "-f" || arg == "--filter") && i + 1 < argc) {
            filter_paths.push_back(argv[++i]);
        } else if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            config_paths.push_back(argv[++i]);
        } else if ((arg == "-ch" || arg == "--channel") && i + 1 < argc) {
            channel_str = argv[++i];
        }
    }

    // 單張圖片處理模式。
    if (!input_path.empty()) {
        if (output_path.empty()) {
            output_path = "outputs/app_output.png";
        }
        ImgProcStatus status = process_single_image(input_path, output_path, filter_paths, config_paths, channel_str);
        return (status == IMGPROC_SUCCESS) ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    // 自動批次處理模式 (Auto Batch Mode)：若無 CLI 參數，自動掃描 inputs/ 資料夾。
    std::string in_dir = "inputs";
    std::string out_dir = "outputs";

    if (!std::filesystem::exists(in_dir)) {
        IMGPROC_LOG_ERROR("Input directory '%s' does not exist.", in_dir.c_str());
        return EXIT_FAILURE;
    }

    IMGPROC_LOG_INFO("Running in Auto Batch Mode. Scanning '%s/' directory...", in_dir.c_str());

    bool processed_any = false;
    bool all_success = true;
    for (const auto& entry : std::filesystem::directory_iterator(in_dir)) {
        if (entry.is_regular_file()) {
            std::string in_file = entry.path().string();
            std::string out_file = out_dir + "/" + entry.path().stem().string() + "_processed.png";
            IMGPROC_LOG_INFO("Processing batch file: %s -> %s", in_file.c_str(), out_file.c_str());
            ImgProcStatus status = process_single_image(in_file, out_file, filter_paths, config_paths, channel_str);
            if (status != IMGPROC_SUCCESS) {
                IMGPROC_LOG_ERROR("Failed processing batch file: %s", in_file.c_str());
                all_success = false;
            }
            processed_any = true;
        }
    }

    if (!processed_any) {
        IMGPROC_LOG_INFO("No image files found in '%s/'.", in_dir.c_str());
    }

    // 批次 Exit Code 雙重防衛：必須同時保證有實質處理圖片且全數成功，防範空目錄假成功。
    return (processed_any && all_success) ? EXIT_SUCCESS : EXIT_FAILURE;
}
