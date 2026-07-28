#pragma once 

#include <stdbool.h> 
#include <stddef.h> 
#include <stdint.h>  

#ifdef __cplusplus //這個是C++的語法，表示如果是C++編譯器，則使用extern "C"來告訴編譯器這些函數是C語言的函數，避免名稱改編。
extern "C" {
#endif

/**
 * @brief A status represents the operation result of a funciton. 
 */
typedef enum {
    IMGPROC_SUCCESS,
    IMGPROC_ERROR_RUNTIME,
    IMGPROC_ERROR_INVALID_ARG,
    IMGPROC_ERROR_INVALID_CONFIG_FILE,
    IMGPROC_ERROR_CONFIG_SECTION_NOT_FOUND,
    IMGPROC_ERROR_CONFIG_KEY_NOT_FOUND,
    IMGPROC_ERROR_CONFIG_TYPE_MISMATCH,
    IMGPROC_ERROR_OUT_OF_MEMOERY,
    IMGPROC_ERROR_LOAD_FILTER,
    IMGPROC_ERROR_FILTER_SPECIFIC
} ImgProcStatus; //函式回傳狀態

/** @brief The struct holds image data used by this library and plugins. */
typedef struct ImgProcImage ImgProcImage; //設計結構體存放

/**
 * @brief The signature of a releases function of @ref ImgProcImage.
 *
 * Releases the resources used by @a image.
 *
 * @param [in] image The pointer to the image to release. This function can also release the pointer
 * itself.
 */
typedef void (*ImgProcImageReleaseFn)(ImgProcImage* image); //函式指標，指向釋放圖像資源的函式

struct ImgProcImage { //原始資料處理 新增channels成員變數，表示每個像素的通道數（1=灰階，3=RGB，4=RGBA）。
    void* data;                       /**< Holds the raw data of image. */
    size_t data_size;                 /**< Holds the data size of @a data */
    uint32_t width;                   /**< Holds the width in pixels of image. */
    uint32_t height;                  /**< Holds the height in pixels of image. */
    uint32_t stride;                  /**< Holds the stride (or pitch) in bytes of image. */
    uint32_t channels;                /**< Holds the number of channels per pixel (1=Grayscale, 3=RGB, 4=RGBA). */
    ImgProcImageReleaseFn release_fn; /**< Release function of this image. This member can be NULL
                                         if there is no resources needs to release. */
};

/**
 * @brief Get a human-readable string describes the @a status.
 *
 * @param [in] status The status returned by a function.
 *
 * @return A NULL-terminated string. The data is owned by this function.
 */
const char* imgproc_get_status_str(ImgProcStatus status);//將狀態轉換為可讀的字串

#ifdef __cplusplus//將C++的函式包裝在extern "C"中，避免名稱改編。
}
#endif