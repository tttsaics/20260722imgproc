/**
 * @file main.cpp
 * @brief High-level CLI orchestrator application for ImgProc SDK.
 *        Supports multi-filter plugin chaining (-f p1.so -c c1.toml -f p2.so -c c2.toml).
 */

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

static bool is_image_extension(const std::string& filename) {
    std::filesystem::path p(filename);
    std::string ext = p.extension().string();
    for (char& c : ext) c = static_cast<char>(std::tolower(c));
    return (ext == ".jpg" || ext == ".jpeg" || ext == ".png");
}

static ImgProcStatus process_single_image(const std::string& input_path,
                                         const std::string& output_path,
                                         const std::vector<std::string>& filter_paths,
                                         const std::vector<std::string>& config_paths,
                                         const std::string& channel_str) {
    ImgProcStatus status = IMGPROC_SUCCESS;

    // Validate channel_str if provided
    if (!channel_str.empty()) {
        std::string ch_upper = channel_str;
        for (char& c : ch_upper) c = static_cast<char>(std::toupper(c));
        if (ch_upper != "R" && ch_upper != "G" && ch_upper != "B") {
            IMGPROC_LOG_ERROR("Invalid channel parameter '%s'. Must be R, G, or B.", channel_str.c_str());
            return IMGPROC_ERROR_INVALID_ARG;
        }
    }

    // Validate output extension
    std::filesystem::path out_p(output_path);
    std::string out_ext = out_p.extension().string();
    for (char& c : out_ext) c = static_cast<char>(std::tolower(c));
    if (!out_ext.empty() && out_ext != ".jpg" && out_ext != ".jpeg" && out_ext != ".png") {
        IMGPROC_LOG_ERROR("Unsupported output file extension '%s'. Only .png, .jpg, and .jpeg are supported.", out_ext.c_str());
        return IMGPROC_ERROR_INVALID_ARG;
    }

    // 1. Read input image using IO module
    ImgProcImage* current_img = nullptr;
    status = imgproc_image_read(input_path.c_str(), &current_img);
    if (status != IMGPROC_SUCCESS) {
        IMGPROC_LOG_ERROR("Failed to read input image '%s'.", input_path.c_str());
        return status;
    }

    std::vector<ImgProcFilterApi> loaded_apis;
    std::vector<ImgProcFilterHandle> active_handles;

    // 2. Chain-execute filter plugins in sequence
    for (size_t i = 0; i < filter_paths.size(); ++i) {
        const std::string& f_path = filter_paths[i];
        std::string c_path = (i < config_paths.size()) ? config_paths[i] : "";

        ImgProcConfigHandle config_handle = IMGPROC_INVALID_CONFIG_HANDLE;
        if (!c_path.empty()) {
            status = imgproc_config_load(&config_handle, c_path.c_str());
            if (status != IMGPROC_SUCCESS) {
                IMGPROC_LOG_ERROR("Failed to load config '%s' for filter [%zu].", c_path.c_str(), i);
                break;
            }
        }

        ImgProcFilterApi filter_api;
        std::memset(&filter_api, 0, sizeof(filter_api));
        status = imgproc_filter_load_api(&filter_api, f_path.c_str());
        if (status != IMGPROC_SUCCESS) {
            IMGPROC_LOG_ERROR("Failed to load filter plugin '%s' [%zu].", f_path.c_str(), i);
            if (config_handle != IMGPROC_INVALID_CONFIG_HANDLE) imgproc_config_destroy(config_handle);
            break;
        }

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

        // Keep API and handle tracked so code segment remains valid until cleanup
        loaded_apis.push_back(filter_api);
        active_handles.push_back(filter_handle);

        ImgProcImage* next_img = nullptr;
        status = filter_api.transform(filter_handle, current_img, &next_img);

        if (status != IMGPROC_SUCCESS || !next_img) {
            IMGPROC_LOG_ERROR("Filter transformation failed for plugin '%s' [%zu].", f_path.c_str(), i);
            break;
        }

        ImgProcImage* prev_img = current_img;
        current_img = next_img;

        // Clean up previous image struct shell (data buffer was freed inside transform's release_fn)
        if (prev_img && prev_img != current_img) {
            delete prev_img;
        }
    }

    // 3. Handle RGB Channel if specified
    if (status == IMGPROC_SUCCESS && !channel_str.empty()) {
        ImgProcChannel ch = IMGPROC_CHANNEL_R;
        if (channel_str == "G" || channel_str == "g") ch = IMGPROC_CHANNEL_G;
        else if (channel_str == "B" || channel_str == "b") ch = IMGPROC_CHANNEL_B;

        imgproc_image_keep_channel(current_img, ch);
    }

    // 4. Save output image using IO module
    if (status == IMGPROC_SUCCESS) {
        if (out_ext == ".jpg" || out_ext == ".jpeg") {
            status = imgproc_image_write_jpg(output_path.c_str(), current_img, 90);
        } else {
            status = imgproc_image_write_png(output_path.c_str(), current_img);
        }
    }

    // 5. Clean up final transformed image BEFORE unloading plugin shared libraries
    if (current_img) {
        imgproc_image_destroy(current_img);
    }

    // 6. Safely destroy all loaded filter handles and unload dynamic .so APIs (Leak-proof cleanup)
    for (size_t i = 0; i < loaded_apis.size(); ++i) {
        if (i < active_handles.size() && active_handles[i]) {
            loaded_apis[i].destroy(active_handles[i]);
        }
        imgproc_filter_destroy_api(&loaded_apis[i]);
    }

    return status;
}

int main(int argc, char* argv[]) {
    imgproc_logger_use_console();
    imgproc_logger_set_level(IMGPROC_LOGLEVEL_INFO);

    std::string input_path;
    std::string output_path;
    std::vector<std::string> filter_paths;
    std::vector<std::string> config_paths;
    std::string channel_str;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
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

    if (!input_path.empty()) {
        if (output_path.empty()) {
            output_path = "outputs/app_output.png";
        }
        ImgProcStatus status = process_single_image(input_path, output_path, filter_paths, config_paths, channel_str);
        return (status == IMGPROC_SUCCESS) ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    // Auto Batch Mode: If no arguments, scan inputs/ and process to outputs/
    std::string in_dir = "inputs";
    std::string out_dir = "outputs";
    if (!std::filesystem::exists(in_dir) && std::filesystem::exists("../inputs")) {
        in_dir = "../inputs";
        out_dir = "../outputs";
    }

    if (!std::filesystem::exists(in_dir)) {
        IMGPROC_LOG_ERROR("Input directory '%s' does not exist.", in_dir.c_str());
        return EXIT_FAILURE;
    }

    IMGPROC_LOG_INFO("Running in Auto Batch Mode. Scanning '%s/' directory...", in_dir.c_str());

    bool processed_any = false;
    bool all_success = true;
    for (const auto& entry : std::filesystem::directory_iterator(in_dir)) {
        if (entry.is_regular_file() && is_image_extension(entry.path().string())) {
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

    return (processed_any && all_success) ? EXIT_SUCCESS : EXIT_FAILURE;
}
