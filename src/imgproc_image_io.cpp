#include "imgproc_image_io.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <filesystem>
#include <cstdlib>
#include <limits>
#include <new>
#include <string_view>

#include "imgproc_logger.h"
#include "priv/imgproc_helper.h"

static ImgProcStatus ensure_parent_dir_exists(const char* filename) {
    // 僅供本檔案使用的私有資料夾工具。
    if (!filename) return IMGPROC_ERROR_INVALID_ARG;

    // 先確認路徑是否包含父資料夾。
    std::filesystem::path p(filename);
    if (p.has_parent_path()) {
        std::error_code ec;
        // 遞迴建立不存在的資料夾，並以 ec 接收錯誤。
        std::filesystem::create_directories(p.parent_path(), ec);
        if (ec) {
            // 將 filesystem 錯誤轉換成專案統一的狀態碼。
            IMGPROC_LOG_ERROR("Failed to create parent directory for '%s': %s",
                              filename, ec.message().c_str());
            return IMGPROC_ERROR_RUNTIME;
        }
    }

    return IMGPROC_SUCCESS;
}

static void stbi_image_release_wrapper(ImgProcImage* image) {
    // stbi_load() 配置的資料必須使用 stbi_image_free() 釋放。
    if (image) {
        if (image->data) {
            stbi_image_free(image->data);
            // 釋放後清除指標，避免形成懸空指標或重複釋放。
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

    // 零拷貝檢測副檔名：若非標準 .jpg/.jpeg/.png，發出 Warning 警告，但仍嘗試呼叫 stbi_load 進行解碼！
    std::string_view sv(filename);
    size_t dot_pos = sv.rfind('.');
    if (dot_pos != std::string_view::npos) {
        std::string_view ext = sv.substr(dot_pos);
        bool is_standard = (ext == ".jpg"  || ext == ".JPG"  ||
                            ext == ".jpeg" || ext == ".JPEG" ||
                            ext == ".png"  || ext == ".PNG");
        if (!is_standard) {
            IMGPROC_LOG_WARN("Unusual input image extension '%.*s'. Attempting to decode image content anyway.",
                             static_cast<int>(ext.length()), ext.data());
        }
    }

    int width = 0;
    int height = 0;
    // 記錄來源檔案通道數，供診斷使用。
    int file_channels = 0;

    // 強制所有輸入統一為 RGBA 四通道，簡化後續處理並避免通道越界。
    unsigned char* pixels = stbi_load(filename, &width, &height, &file_channels, 4);
    if (!pixels) {
        IMGPROC_LOG_ERROR("Failed to load image '%s': %s", filename, stbi_failure_reason());
        return IMGPROC_ERROR_RUNTIME;
    }

    // nothrow 讓配置失敗回傳 nullptr，避免 exception 跨越 C-ABI。
    ImgProcImage* img = new (std::nothrow) ImgProcImage;
    if (!img) {
        // 結構配置失敗時釋放已由 stbi_load() 配置的 pixels。
        stbi_image_free(pixels);
        IMGPROC_LOG_ERROR("Failed to allocate memory for ImgProcImage.");
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }

    img->data = pixels;
    // 先轉成 size_t 計算記憶體大小，降低窄型別乘法溢位風險。
    img->data_size = static_cast<size_t>(width) * height * 4;
    img->width = static_cast<uint32_t>(width);
    img->height = static_cast<uint32_t>(height);
    img->stride = static_cast<uint32_t>(width * 4);
    img->channels = 4;
    // 記錄與資料配置方式相容的釋放 callback。
    img->release_fn = stbi_image_release_wrapper;

    // 所有欄位初始化完成後才將影像所有權交給呼叫端。
    *image = img;
    IMGPROC_LOG_INFO("Loaded image '%s' (%dx%d, 4 channels RGBA, file had %d channels).",
                     filename, width, height, file_channels);

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

    // 驗證內部 RGBA 契約，避免以錯誤 layout 解讀資料。
    if (image->channels != 4) {
        IMGPROC_LOG_ERROR("Invalid image channel count: %u. Required: 4 channels (RGBA).", image->channels);
        return IMGPROC_ERROR_INVALID_ARG;
    }

    if (quality < 1 || quality > 100) {
        IMGPROC_LOG_ERROR("Quality '%d' out of range (1-100).", quality);
        return IMGPROC_ERROR_INVALID_ARG;
    }

    uint32_t width = image->width;
    uint32_t height = image->height;
    // 零尺寸影像沒有可供編碼的有效像素資料。
    if (width == 0 || height == 0) {
        IMGPROC_LOG_ERROR("Image dimensions must be non-zero.");
        return IMGPROC_ERROR_INVALID_ARG;
    }
    // stb 的尺寸參數是 int，因此轉型前必須檢查上界。
    if (width > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
        height > static_cast<uint32_t>(std::numeric_limits<int>::max())) {
        IMGPROC_LOG_ERROR("Image dimensions exceed stb JPEG limits: %ux%u.", width, height);
        return IMGPROC_ERROR_INVALID_ARG;
    }

    // 先建立父資料夾，避免 writer 因路徑不存在而失敗。
    ImgProcStatus dir_status = ensure_parent_dir_exists(filename);
    if (dir_status != IMGPROC_SUCCESS) return dir_status;

    // 建立 RGB 暫存 buffer，保留原始 RGBA 影像不被修改。
    // 配置前檢查乘法，避免 rgb_size 計算溢位。
    if (height > std::numeric_limits<size_t>::max() / 3 ||
        width > std::numeric_limits<size_t>::max() / (static_cast<size_t>(height) * 3)) {
        IMGPROC_LOG_ERROR("RGB buffer size overflow for image dimensions %ux%u.", width, height);
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }
    size_t rgb_size = static_cast<size_t>(width) * height * 3;
    uint8_t* rgb_buf = static_cast<uint8_t*>(std::malloc(rgb_size));
    if (!rgb_buf) {
        IMGPROC_LOG_ERROR("Failed to allocate temporary RGB buffer for JPEG export.");
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }

    const uint8_t* src = static_cast<const uint8_t*>(image->data);
    for (size_t y = 0; y < height; ++y) {
        const uint8_t* src_row = src + y * image->stride;
        uint8_t* dst_row = rgb_buf + y * (width * 3);
        for (size_t x = 0; x < width; ++x) {
            const uint8_t* src_pix = src_row + x * 4;
            uint8_t* dst_pix = dst_row + x * 3;
            uint8_t r = src_pix[0];
            uint8_t g = src_pix[1];
            uint8_t b = src_pix[2];
            uint8_t a = src_pix[3];

            // 將 RGBA alpha 合成到白色背景，轉成 JPEG 可用的 RGB。
            dst_pix[0] = static_cast<uint8_t>((r * a + 255 * (255 - a)) / 255);
            dst_pix[1] = static_cast<uint8_t>((g * a + 255 * (255 - a)) / 255);
            dst_pix[2] = static_cast<uint8_t>((b * a + 255 * (255 - a)) / 255);
        }
    }

    // 經過前面的範圍檢查後，這裡才安全轉成 stb 要求的 int。
    int result = stbi_write_jpg(filename, static_cast<int>(width),
                                static_cast<int>(height), 3, rgb_buf, quality);
    // 寫檔完成後立即釋放暫存 buffer，確保所有路徑不洩漏。
    std::free(rgb_buf);

    if (!result) {
        IMGPROC_LOG_ERROR("Failed to write JPEG image to '%s'.", filename);
        return IMGPROC_ERROR_RUNTIME;
    }

    IMGPROC_LOG_INFO("Successfully saved JPEG image to '%s' with white background alpha blending.", filename);
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

    if (image->channels != 4) {
        IMGPROC_LOG_ERROR("Invalid image channel count: %u. Required: 4 channels (RGBA).", image->channels);
        return IMGPROC_ERROR_INVALID_ARG;
    }

    // 零尺寸影像沒有可供編碼的有效像素資料。
    if (image->width == 0 || image->height == 0) {
        IMGPROC_LOG_ERROR("Image dimensions must be non-zero.");
        return IMGPROC_ERROR_INVALID_ARG;
    }

    // 先建立父資料夾，避免 writer 因路徑不存在而失敗。
    ImgProcStatus dir_status = ensure_parent_dir_exists(filename);
    if (dir_status != IMGPROC_SUCCESS) return dir_status;

    // stb 的尺寸與 stride 參數是 int，因此轉型前必須檢查上界。
    if (image->width > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
        image->height > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
        image->stride > static_cast<uint32_t>(std::numeric_limits<int>::max())) {
        IMGPROC_LOG_ERROR("Image metadata exceeds stb PNG limits: %ux%u, stride %u.",
                          image->width, image->height, image->stride);
        return IMGPROC_ERROR_INVALID_ARG;
    }

    // PNG 保留 RGBA，並使用 stride 定位每一列資料。
    int stride_in_bytes = static_cast<int>(image->stride);
    int result = stbi_write_png(filename, static_cast<int>(image->width),
                                static_cast<int>(image->height), 4, image->data, stride_in_bytes);
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

    // 嚴格 C-ABI 契約：Producer 必須提供對應的 release_fn 釋放回調函數。
    if (!image->release_fn) {
        IMGPROC_LOG_ERROR("Image release_fn is null. Producer must specify a valid release callback.");
        return IMGPROC_ERROR_INVALID_ARG;
    }

    // 呼叫專屬的釋放函數清理像素資料。
    image->release_fn(image);

    delete image;
    return IMGPROC_SUCCESS;
}
