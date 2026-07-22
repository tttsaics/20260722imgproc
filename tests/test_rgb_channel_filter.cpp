#include <imgproc_config.h>
#include <imgproc_filter_loader.h>
#include <imgproc_logger.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <vector>

static void free_stb_image(ImgProcImage* image) {
    if (image && image->data) {
        stbi_image_free(image->data);
        image->data = nullptr;
    }
}

int main() {
    imgproc_logger_use_console();
    imgproc_logger_set_level(IMGPROC_LOGLEVEL_INFO);

    const char* input_filename = "test_input.png";
    const char* output_filename = "test_output_R.png";
    const int width = 64;
    const int height = 64;

    // 1. Generate a synthetic input image (64x64 RGB)
    std::vector<uint8_t> synthetic_data(width * height * 3);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x) * 3;
            synthetic_data[idx + 0] = 200; // Red channel
            synthetic_data[idx + 1] = 150; // Green channel
            synthetic_data[idx + 2] = 100; // Blue channel
        }
    }

    if (!stbi_write_png(input_filename, width, height, 3, synthetic_data.data(), width * 3)) {
        std::fprintf(stderr, "Failed to write synthetic test image.\n");
        return EXIT_FAILURE;
    }
    std::printf("Successfully created synthetic input image '%s'.\n", input_filename);

    // 2. Load the input image using stb_image
    int img_w = 0, img_h = 0, img_comp = 0;
    stbi_uc* pixels = stbi_load(input_filename, &img_w, &img_h, &img_comp, 3);
    if (!pixels) {
        std::fprintf(stderr, "Failed to load image '%s'.\n", input_filename);
        return EXIT_FAILURE;
    }

    ImgProcImage input_image{};
    input_image.data = pixels;
    input_image.data_size = static_cast<size_t>(img_w * img_h * 3);
    input_image.width = static_cast<uint32_t>(img_w);
    input_image.height = static_cast<uint32_t>(img_h);
    input_image.stride = static_cast<uint32_t>(img_w * 3);
    input_image.release_fn = free_stb_image;

    // 3. Load configuration
    ImgProcConfigHandle config_handle = IMGPROC_INVALID_CONFIG_HANDLE;
    ImgProcStatus status = imgproc_config_load(&config_handle, "rgb_filter_config.toml");
    if (status != IMGPROC_SUCCESS) {
        std::fprintf(stderr, "Failed to load 'rgb_filter_config.toml': %s\n", imgproc_get_status_str(status));
        stbi_image_free(pixels);
        return EXIT_FAILURE;
    }

    // 4. Load filter plugin
    ImgProcFilterApi filter_api{};
#if defined(_WIN32)
    const char* plugin_lib = "librgb_channel_filter.dll";
#elif defined(__APPLE__)
    const char* plugin_lib = "./librgb_channel_filter.dylib";
#else
    const char* plugin_lib = "./librgb_channel_filter.so";
#endif

    status = imgproc_filter_load_api(&filter_api, plugin_lib);
    if (status != IMGPROC_SUCCESS) {
        std::fprintf(stderr, "Failed to load filter API from '%s': %s\n", plugin_lib, imgproc_get_status_str(status));
        imgproc_config_destroy(config_handle);
        stbi_image_free(pixels);
        return EXIT_FAILURE;
    }

    // 5. Create filter instance
    ImgProcFilterHandle filter_handle = IMGPROC_INVALID_FILTER_HANDLE;
    status = filter_api.create(&filter_handle, config_handle);
    if (status != IMGPROC_SUCCESS) {
        std::fprintf(stderr, "Failed to create filter instance: %s\n", imgproc_get_status_str(status));
        imgproc_filter_destroy_api(&filter_api);
        imgproc_config_destroy(config_handle);
        stbi_image_free(pixels);
        return EXIT_FAILURE;
    }

    // 6. Execute transform
    ImgProcImage* output_image_ptr = nullptr;
    status = filter_api.transform(filter_handle, &input_image, &output_image_ptr);
    if (status != IMGPROC_SUCCESS || !output_image_ptr) {
        std::fprintf(stderr, "Filter transform failed: %s\n", imgproc_get_status_str(status));
        filter_api.destroy(filter_handle);
        imgproc_filter_destroy_api(&filter_api);
        imgproc_config_destroy(config_handle);
        stbi_image_free(pixels);
        return EXIT_FAILURE;
    }

    // 7. Verify pixel channels (R should be 200, G should be 0, B should be 0)
    uint8_t* out_data = static_cast<uint8_t*>(output_image_ptr->data);
    bool check_passed = true;
    for (uint32_t i = 0; i < output_image_ptr->width * output_image_ptr->height; ++i) {
        uint8_t r = out_data[i * 3 + 0];
        uint8_t g = out_data[i * 3 + 1];
        uint8_t b = out_data[i * 3 + 2];
        if (r != 200 || g != 0 || b != 0) {
            check_passed = false;
            std::fprintf(stderr, "Pixel check failed at index %u: R=%d, G=%d, B=%d\n", i, r, g, b);
            break;
        }
    }

    if (check_passed) {
        std::printf("Pixel verification passed: Green and Blue channels successfully zeroed out!\n");
    }

    // 8. Save output image
    if (stbi_write_png(output_filename, output_image_ptr->width, output_image_ptr->height, 3,
                       output_image_ptr->data, output_image_ptr->stride)) {
        std::printf("Successfully saved transformed image to '%s'.\n", output_filename);
    } else {
        std::fprintf(stderr, "Failed to save output image '%s'.\n", output_filename);
    }

    // 9. Resource cleanup
    if (output_image_ptr->release_fn) {
        output_image_ptr->release_fn(output_image_ptr);
    }

    filter_api.destroy(filter_handle);
    imgproc_filter_destroy_api(&filter_api);
    imgproc_config_destroy(config_handle);

    return check_passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
