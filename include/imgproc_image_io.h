/**
 * @file imgproc_image_io.h
 * @brief Image file reading and writing utilities using stb_image and stb_image_write.
 */

#pragma once

#include "imgproc_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Read an image from file (JPG/PNG/BMP) and convert to RGBA 4-channel format.
 *
 * @param [in] filename The NULL-terminated path to the image file.
 * @param [out] image Pointer to receive the allocated @ref ImgProcImage object.
 *
 * @retval @ref IMGPROC_SUCCESS Operation completed successfully.
 * @retval @ref IMGPROC_ERROR_INVALID_ARG @a filename or @a image is NULL.
 * @retval @ref IMGPROC_ERROR_RUNTIME Failed to open or decode image file.
 * @retval @ref IMGPROC_ERROR_OUT_OF_MEMOERY Out of memory during allocation.
 *
 * @note The caller is responsible for destroying the returned image using @ref imgproc_image_destroy.
 */
ImgProcStatus imgproc_image_read(const char* filename, ImgProcImage** image);

/**
 * @brief Write an @ref ImgProcImage to a JPEG file on disk.
 *
 * @param [in] filename The NULL-terminated path to destination JPEG file.
 * @param [in] image Pointer to the @ref ImgProcImage object to be written.
 * @param [in] quality JPEG compression quality (1 to 100).
 *
 * @retval @ref IMGPROC_SUCCESS Operation completed successfully.
 * @retval @ref IMGPROC_ERROR_INVALID_ARG @a filename or @a image is NULL, or invalid quality.
 * @retval @ref IMGPROC_ERROR_RUNTIME Failed to write JPEG file.
 */
ImgProcStatus imgproc_image_write_jpg(const char* filename, ImgProcImage* image, int quality);

/**
 * @brief Write an @ref ImgProcImage to a PNG file on disk.
 *
 * @param [in] filename The NULL-terminated path to destination PNG file.
 * @param [in] image Pointer to the @ref ImgProcImage object to be written.
 *
 * @retval @ref IMGPROC_SUCCESS Operation completed successfully.
 * @retval @ref IMGPROC_ERROR_INVALID_ARG @a filename or @a image is NULL.
 * @retval @ref IMGPROC_ERROR_RUNTIME Failed to write PNG file.
 */
ImgProcStatus imgproc_image_write_png(const char* filename, ImgProcImage* image);

/**
 * @brief Release resources associated with an @ref ImgProcImage object.
 *
 * @param [in] image Pointer to the @ref ImgProcImage object to be destroyed.
 *
 * @retval @ref IMGPROC_SUCCESS Operation completed successfully.
 * @retval @ref IMGPROC_ERROR_INVALID_ARG @a image is NULL.
 */
ImgProcStatus imgproc_image_destroy(ImgProcImage* image);

#ifdef __cplusplus
}
#endif
