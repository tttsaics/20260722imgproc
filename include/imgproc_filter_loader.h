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
} ImgProcFilterApi;//這個結構體包含了三個函式指標，分別指向創建、銷毀和轉換函式，以及一個內部數據指針，用於存儲加載器使用的內部數據。這些函式指標允許調用者與濾鏡插件進行交互，實現濾鏡的創建、銷毀和圖像轉換操作。

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
ImgProcStatus imgproc_filter_load_api(ImgProcFilterApi* api, const char* lib_file);//這個函式用於從共享庫中加載濾鏡的API。它接受一個指向ImgProcFilterApi結構體的指針和共享庫的路徑作為參數，並返回操作的狀態。該函式會檢查參數是否為NULL，並嘗試加載共享庫及其所需的函式。如果成功，它將填充ImgProcFilterApi結構體中的函式指標，否則返回相應的錯誤狀態。

/**
 * @brief Release the resources used by the loader.
 *
 * @param [in] api The pointer to a @ref ImgProcFilterApi struct.
 *
 * @retval @ref IMGPROC_SUCCESS Operation success.
 * @retval @ref IMGPROC_ERROR_INVALID_ARG @a api is NULL.
 */
ImgProcStatus imgproc_filter_destroy_api(ImgProcFilterApi* api);//這個函式用於釋放加載器使用的資源。它接受一個指向ImgProcFilterApi結構體的指針作為參數，並返回操作的狀態。該函式會檢查參數是否為NULL，如果不為NULL，它將釋放內部數據並將函式指標設置為nullptr，以清理資源。如果成功，返回IMGPROC_SUCCESS，否則返回相應的錯誤狀態。

#ifdef __cplusplus
}
#endif