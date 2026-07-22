#pragma once

#include "imgproc_filter_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The struct holds the filter's APIs.
 *
 * Caller can use these APIs to interact with the filter plugin.
 */
typedef struct {
    ImgProcFilterCreateFn create; /**< Holds the create function. */
    ImgProcFilterDestroyFn destroy; /**< Holds the destroy function. */
    ImgProcFilterTransformFn transform; /**< Holds the transform function. */
    void* internal_data; /**< Internal data used by the loader. */
} ImgProcFilterApi;

/**
 * @brief Load the filter's APIs from the shared library.
 *
 * @param [out] api The pointer to a @ref ImgProcFilterApi struct.
 * @param [in] lib_file The path to the shared library.
 *
 * @retval @ref IMGPROC_SUCCESS Operation success.
 * @retval @ref IMGPROC_ERROR_INVALID_ARG @a api or @a lib_file is NULL.
 * @retval @ref IMGPROC_ERROR_LOAD_FILTER Unable to load the shared library or
 * the library doesn't define the requried functions.
 */
ImgProcStatus imgproc_filter_load_api(ImgProcFilterApi* api, const char* lib_file);

/**
 * @brief Release the resources used by the loader.
 *
 * @param [in] api The pointer to a @ref ImgProcFilterApi struct.
 *
 * @retval @ref IMGPROC_SUCCESS Operation success.
 * @retval @ref IMGPROC_ERROR_INVALID_ARG @a api is NULL.
 */
ImgProcStatus imgproc_filter_destroy_api(ImgProcFilterApi* api);

#ifdef __cplusplus
}
#endif