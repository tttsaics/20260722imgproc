/**
 * @file main.cpp
 * @brief High-level CLI orchestrator application for ImgProc SDK.
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

static void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options]\n"
              << "Options:\n"
              << "  -i, --input <file>     Path to input image (JPG/PNG)\n"
              << "  -o, --output <file>    Path to output image (JPG/PNG)\n"
              << "  -f, --filter <so_path> Path to filter dynamic plugin (.so)\n"
              << "  -c, --config <toml>    Path to TOML config file\n"
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
                                         const std::string& filter_path,
                                         const std::string& config_path,
                                         const std::string& channel_str) {
    ImgProcStatus status = IMGPROC_SUCCESS;

    // 1. Read input image using IO module
    ImgProcImage* src_img = nullptr;
    status = imgproc_image_read(input_path.c_str(), &src_img);
    if (status != IMGPROC_SUCCESS) {
        IMGPROC_LOG_ERROR("Failed to read input image '%s'.", input_path.c_str());
        return status;
    }

    ImgProcImage* dst_img = src_img;
    bool filter_applied = false;

    // 2. Load Config & Filter plugin if specified
    ImgProcFilterApi filter_api;
    std::memset(&filter_api, 0, sizeof(filter_api));
    ImgProcFilterHandle filter_handle = IMGPROC_INVALID_FILTER_HANDLE;
    ImgProcConfigHandle config_handle = IMGPROC_INVALID_CONFIG_HANDLE;

    if (!config_path.empty()) {
        status = imgproc_config_load(&config_handle, config_path.c_str());
        if (status != IMGPROC_SUCCESS) {
            IMGPROC_LOG_ERROR("Failed to load config '%s'.", config_path.c_str());
            imgproc_image_destroy(src_img);
            return status;
        }
    }

    if (!filter_path.empty()) {
        status = imgproc_filter_load_api(&filter_api, filter_path.c_str());
        if (status != IMGPROC_SUCCESS) {
            IMGPROC_LOG_ERROR("Failed to load filter plugin '%s'.", filter_path.c_str());
            if (config_handle != IMGPROC_INVALID_CONFIG_HANDLE) imgproc_config_destroy(config_handle);
            imgproc_image_destroy(src_img);
            return status;
        }

        status = filter_api.create(&filter_handle, config_handle);
        if (status != IMGPROC_SUCCESS) {
            IMGPROC_LOG_ERROR("Failed to create filter handle.");
            imgproc_filter_destroy_api(&filter_api);
            if (config_handle != IMGPROC_INVALID_CONFIG_HANDLE) imgproc_config_destroy(config_handle);
            imgproc_image_destroy(src_img);
            return status;
        }

        status = filter_api.transform(filter_handle, src_img, &dst_img);
        if (status != IMGPROC_SUCCESS || !dst_img) {
            IMGPROC_LOG_ERROR("Filter transformation failed.");
            filter_api.destroy(filter_handle);
            imgproc_filter_destroy_api(&filter_api);
            if (config_handle != IMGPROC_INVALID_CONFIG_HANDLE) imgproc_config_destroy(config_handle);
            imgproc_image_destroy(src_img);
            return status;
        }

        imgproc_image_destroy(src_img);
        filter_applied = true;
    }

    // 3. Handle RGB Channel if specified
    if (!channel_str.empty()) {
        ImgProcChannel ch = IMGPROC_CHANNEL_R;
        if (channel_str == "G" || channel_str == "g") ch = IMGPROC_CHANNEL_G;
        else if (channel_str == "B" || channel_str == "b") ch = IMGPROC_CHANNEL_B;

        imgproc_image_keep_channel(dst_img, ch);
    }

    // 4. Save output image using IO module
    std::filesystem::path out_p(output_path);
    std::string out_ext = out_p.extension().string();
    for (char& c : out_ext) c = static_cast<char>(std::tolower(c));

    if (out_ext == ".jpg" || out_ext == ".jpeg") {
        status = imgproc_image_write_jpg(output_path.c_str(), dst_img, 90);
    } else {
        status = imgproc_image_write_png(output_path.c_str(), dst_img);
    }

    // 5. Cleanup
    imgproc_image_destroy(dst_img);
    if (filter_applied) {
        filter_api.destroy(filter_handle);
        imgproc_filter_destroy_api(&filter_api);
    }
    if (config_handle != IMGPROC_INVALID_CONFIG_HANDLE) {
        imgproc_config_destroy(config_handle);
    }

    return status;
}

int main(int argc, char* argv[]) {
    imgproc_logger_use_console();
    imgproc_logger_set_level(IMGPROC_LOGLEVEL_INFO);

    std::string input_path;
    std::string output_path;
    std::string filter_path;
    std::string config_path;
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
            filter_path = argv[++i];
        } else if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            config_path = argv[++i];
        } else if ((arg == "-ch" || arg == "--channel") && i + 1 < argc) {
            channel_str = argv[++i];
        }
    }

    if (!input_path.empty()) {
        if (output_path.empty()) {
            output_path = "outputs/app_output.png";
        }
        ImgProcStatus status = process_single_image(input_path, output_path, filter_path, config_path, channel_str);
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
    for (const auto& entry : std::filesystem::directory_iterator(in_dir)) {
        if (entry.is_regular_file() && is_image_extension(entry.path().string())) {
            std::string in_file = entry.path().string();
            std::string out_file = out_dir + "/" + entry.path().stem().string() + "_processed.png";
            IMGPROC_LOG_INFO("Processing batch file: %s -> %s", in_file.c_str(), out_file.c_str());
            process_single_image(in_file, out_file, filter_path, config_path, channel_str);
            processed_any = true;
        }
    }

    if (!processed_any) {
        IMGPROC_LOG_INFO("No image files found in '%s/'.", in_dir.c_str());
    }

    return EXIT_SUCCESS;
}
