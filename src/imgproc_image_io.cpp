#include "imgproc_image_io.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <filesystem>
#include <new>

#include "imgproc_logger.h"
#include "priv/imgproc_helper.h"

static void ensure_parent_dir_exists(const char* filename) {
    if (!filename) return;
    std::filesystem::path p(filename);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }
}

static void stbi_image_release_wrapper(ImgProcImage* image) {
    if (image) {
        if (image->data) {
            stbi_image_free(image->data);
            image->data = nullptr;
        }
    }
}

ImgProcStatus imgproc_image_read(const char* filename, ImgProcImage** image) {
    if (!filename) {
        IMGPROC_LOG_ERROR("'%s' is null.", IMGPROC_STR(filename));
        return IMGPROC_ERROR_INVALID_ARG;
    }

    if (!image) {
        IMGPROC_LOG_ERROR("'%s' is null.", IMGPROC_STR(image));
        return IMGPROC_ERROR_INVALID_ARG;
    }

    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char* pixels = stbi_load(filename, &width, &height, &channels, 3);
    if (!pixels) {
        IMGPROC_LOG_ERROR("Failed to load image '%s': %s", filename, stbi_failure_reason());
        return IMGPROC_ERROR_RUNTIME;
    }

    ImgProcImage* img = new (std::nothrow) ImgProcImage;
    if (!img) {
        stbi_image_free(pixels);
        IMGPROC_LOG_ERROR("Failed to allocate memory for ImgProcImage.");
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }

    img->data = pixels;
    img->data_size = static_cast<size_t>(width) * height * 3;
    img->width = static_cast<uint32_t>(width);
    img->height = static_cast<uint32_t>(height);
    img->stride = static_cast<uint32_t>(width * 3);
    img->release_fn = stbi_image_release_wrapper;

    *image = img;
    IMGPROC_LOG_INFO("Loaded image '%s' (%dx%d, RGB).", filename, width, height);

    return IMGPROC_SUCCESS;
}

ImgProcStatus imgproc_image_write_jpg(const char* filename, ImgProcImage* image, int quality) {
    if (!filename) {
        IMGPROC_LOG_ERROR("'%s' is null.", IMGPROC_STR(filename));
        return IMGPROC_ERROR_INVALID_ARG;
    }

    if (!image || !image->data) {
        IMGPROC_LOG_ERROR("'%s' or image data is null.", IMGPROC_STR(image));
        return IMGPROC_ERROR_INVALID_ARG;
    }

    if (quality < 1 || quality > 100) {
        IMGPROC_LOG_ERROR("Quality '%d' out of range (1-100).", quality);
        return IMGPROC_ERROR_INVALID_ARG;
    }

    ensure_parent_dir_exists(filename);

    int result = stbi_write_jpg(filename, static_cast<int>(image->width),
                                static_cast<int>(image->height), 3, image->data, quality);
    if (!result) {
        IMGPROC_LOG_ERROR("Failed to write JPEG image to '%s'.", filename);
        return IMGPROC_ERROR_RUNTIME;
    }

    IMGPROC_LOG_INFO("Successfully saved JPEG image to '%s'.", filename);
    return IMGPROC_SUCCESS;
}

ImgProcStatus imgproc_image_write_png(const char* filename, ImgProcImage* image) {
    if (!filename) {
        IMGPROC_LOG_ERROR("'%s' is null.", IMGPROC_STR(filename));
        return IMGPROC_ERROR_INVALID_ARG;
    }

    if (!image || !image->data) {
        IMGPROC_LOG_ERROR("'%s' or image data is null.", IMGPROC_STR(image));
        return IMGPROC_ERROR_INVALID_ARG;
    }

    ensure_parent_dir_exists(filename);

    int stride_in_bytes = static_cast<int>(image->stride);
    int result = stbi_write_png(filename, static_cast<int>(image->width),
                                static_cast<int>(image->height), 3, image->data, stride_in_bytes);
    if (!result) {
        IMGPROC_LOG_ERROR("Failed to write PNG image to '%s'.", filename);
        return IMGPROC_ERROR_RUNTIME;
    }

    IMGPROC_LOG_INFO("Successfully saved PNG image to '%s'.", filename);
    return IMGPROC_SUCCESS;
}

ImgProcStatus imgproc_image_destroy(ImgProcImage* image) {
    if (!image) {
        IMGPROC_LOG_ERROR("'%s' is null.", IMGPROC_STR(image));
        return IMGPROC_ERROR_INVALID_ARG;
    }

    if (image->release_fn) {
        image->release_fn(image);
    } else if (image->data) {
        stbi_image_free(image->data);
        image->data = nullptr;
    }

    delete image;
    return IMGPROC_SUCCESS;
}
