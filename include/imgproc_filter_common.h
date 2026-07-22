#pragma once

#include "imgproc_config.h"
#include "imgproc_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief A handle to the plugin defined data. */
typedef void* ImgProcFilterHandle;

/** @brief The value represents an invalid handle. */
#define IMGPROC_INVALID_FILTER_HANDLE (NULL)

/**
 * @brief The signature of a filter create function defined by the plugin.
 *
 * Create the resources used by the filter.
 *
 * @param [out] filter_handle The pointer to a handle to the filter.
 * @param [in] filter_config_handle The handle to the configuration for setting up the filter. This
 * is optional.
 *
 * @retval @ref IMGPROC_SUCCESS Operation success.
 * @retval @ref IMGPROC_ERROR_INVALID_ARG @a filter_handle is NULL. @a filter_config_handle is @ref
 * IMGPROC_INVALID_CONFIG_HANDLE.
 * @retval @ref IMGPROC_ERROR_OUT_OF_MEMOERY Out of memory.
 * @retval @ref IMGPROC_ERROR_FILTER_SPECIFIC All other errors in the plugin.
 * */
typedef ImgProcStatus (*ImgProcFilterCreateFn)(ImgProcFilterHandle* filter_handle,
                                               ImgProcConfigHandle filter_config_handle);

/**
 * @brief The signature of a filter destroy function defined by the plugin.
 *
 * Release the resources used by the filter.
 *
 * @param [in] filter_handle The handle to the filter.
 *
 * @retval @ref IMGPROC_SUCCESS Operation success.
 * @retval @ref IMGPROC_ERROR_INVALID_ARG @a filter_handle is @ref IMGPROC_INVALID_FILTER_HANDLE.
 * @retval @ref IMGPROC_ERROR_FILTER_SPECIFIC All other errors in the plugin.
 * */
typedef ImgProcStatus (*ImgProcFilterDestroyFn)(ImgProcFilterHandle filter_handle);

/**
 * @brief The signature of a filter transform function defined by the plugin.
 *
 * Transform the input image to the output image.
 *
 * @param [in] filter_handle The handle to the filter.
 * @param [in] input The pointer to the input image. The data is transfer to this function and
 * this function has the responsibility to release it.
 * @param [out] output The pointer to the pointer to the output image. The caller has responsibility
 * to release it.
 *
 * @retval @ref IMGPROC_SUCCESS Operation success.
 * @retval @ref IMGPROC_ERROR_INVALID_ARG @a filter_handle is @ref IMGPROC_INVALID_FILTER_HANDLE.
 * @a input or @a output is NULL. @a input has invalid data.
 * @retval @ref IMGPROC_ERROR_OUT_OF_MEMOERY Out of memory.
 * @retval @ref IMGPROC_ERROR_FILTER_SPECIFIC All other errors in the plugin.
 * */
typedef ImgProcStatus (*ImgProcFilterTransformFn)(ImgProcFilterHandle filter_handle,
                                                  ImgProcImage* input, ImgProcImage** output);

#ifdef __cplusplus
}
#endif

#define IMGPROC_FILTER_STR(x) #x
#define IMGPROC_FILTER_XSTR(x) IMGPROC_FILTER_STR(x)
#define IMGPROC_FILTER_FN_NAME(type) imgproc_filter_get_##type##_fn_name
#define IMGPROC_FILTER_FN_NAME_STR(type) IMGPROC_FILTER_XSTR(IMGPROC_FILTER_FN_NAME(type))
#define IMGPROC_FILTER_DECLARE_FN(func, type) \
    extern "C" const char* IMGPROC_FILTER_FN_NAME(type)() { return #func; }

#define IMGPROC_FILTER_DECLARE_CREATE_FN(func) IMGPROC_FILTER_DECLARE_FN(func, create)
#define IMGPROC_FILTER_DECLARE_DESTROY_FN(func) IMGPROC_FILTER_DECLARE_FN(func, destroy)
#define IMGPROC_FILTER_DECLARE_TRANSFORM_FN(func) IMGPROC_FILTER_DECLARE_FN(func, transform)