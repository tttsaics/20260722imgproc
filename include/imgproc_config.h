#pragma once

#include "imgproc_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief A handle to the configuration. */
typedef struct ImgProcConfig* ImgProcConfigHandle;//將ImgProcConfig結構體的指針定義為ImgProcConfigHandle，表示這個handle可以指向ImgProcConfig結構體的實例，通常用於封裝配置的內部資料結構。

/** @brief The value that represents an invalid configuration handle. */
#define IMGPROC_INVALID_CONFIG_HANDLE (NULL)//將NULL定義為IMGPROC_INVALID_CONFIG_HANDLE，表示這個handle是無效的，通常用於檢查配置是否正確初始化或釋放。

/**
 * @brief Load configuration from a file.
 *
 * Load a toml configuration file to the memory.
 *
 * @param [out] config_handle The pointer to a handle to the configuration.
 * @param [in] file The path to a toml file.
 *
 * @retval @ref IMGPROC_SUCCESS Operation success.
 * @retval @ref IMGPROC_ERROR_INVALID_ARG @a config_handle or @a file is NULL.
 * @retval @ref IMGPROC_ERROR_INVALID_CONFIG_FILE Unable to parse the file.
 * @retval @ref IMGPROC_ERROR_OUT_OF_MEMOERY Out of memory.
 */
ImgProcStatus imgproc_config_load(ImgProcConfigHandle* config_handle, const char* file);//宣告一個函式imgproc_config_load，用於從指定的toml配置文件中加載配置到內存中。該函式接受兩個參數：一個指向ImgProcConfigHandle的指針config_handle，用於返回加載後的配置句柄；另一個是指向字符的指針file，表示toml文件的路徑。函式返回ImgProcStatus類型的狀態碼，表示操作是否成功或失敗，以及失敗的原因。

/**
 * @brief Destroy the configuration.
 *
 * Release the resources used by the configuration.
 *
 * @param [in] config_handle The handle to the configuration.
 *
 * @retval @ref IMGPROC_SUCCESS Operation success.
 * @retval @ref IMGPROC_ERROR_INVALID_ARG @a config_handle is @ref IMGPROC_INVALID_CONFIG_HANDLE.
 */
ImgProcStatus imgproc_config_destroy(ImgProcConfigHandle config_handle);//宣告一個函式imgproc_config_destroy，用於釋放配置使用的資源。該函式接受一個參數：ImgProcConfigHandle類型的config_handle，表示要釋放的配置句柄。函式返回ImgProcStatus類型的狀態碼，表示操作是否成功或失敗，以及失敗的原因。

/**
 * @brief Get an integer in the configuration.
 *
 * Get an integer of @a key at @a section in the configuration.
 *
 * @param [in] config_handle The handle to the configuration.
 * @param [in] section A NULL-terminated string represents the section in the configuration.
 * A section is represented by [<section_name>].
 * @param [in] key A NULL-terminated string represents the key of value. A key is represented by
 * <key>=<value>.
 * @param [out] value The pointer to an int64_t value.
 *
 * @retval @ref IMGPROC_SUCCESS Operation success.
 * @retval @ref IMGPROC_ERROR_INVALID_ARG @a config_handle is @ref IMGPROC_INVALID_CONFIG_HANDLE
 * Or one of arguments is NULL.
 * @retval @ref IMGPROC_ERROR_CONFIG_SECTION_NOT_FOUND Can't find @a section in the configuration.
 * @retval @ref IMGPROC_ERROR_CONFIG_KEY_NOT_FOUND Can't find @a key in @a section.
 * @retval @ref IMGPROC_ERROR_CONFIG_TYPE_MISMATCH The value doesn't represent an integer.
 */
ImgProcStatus imgproc_config_get_int64(ImgProcConfigHandle config_handle, const char* section,
                                       const char* key, int64_t* value);//宣告一個函式imgproc_config_get_int64，用於從配置中獲取指定節(section)和鍵(key)對應的整數值。該函式接受四個參數：ImgProcConfigHandle類型的config_handle，表示配置句柄；指向字符的指針section，表示配置中的節名稱；指向字符的指針key，表示鍵名稱；以及指向int64_t的指針value，用於返回獲取到的整數值。函式返回ImgProcStatus類型的狀態碼，表示操作是否成功或失敗，以及失敗的原因。

/**
 * @brief Get a floating-point number in the configuration.
 *
 * Get a floating-point number of @a key at @a section from the configuration.
 *
 * @param [in] config_handle The handle to the configuration.
 * @param [in] section A NULL-terminated string represents the section in the configuration.
 * A section is represented by [<section_name>].
 * @param [in] key A NULL-terminated string represents the key of value. A key is represented by
 * <key>=<value>.
 * @param [out] value The pointer to a double value.
 *
 * @retval @ref IMGPROC_SUCCESS Operation success.
 * @retval @ref IMGPROC_ERROR_INVALID_ARG @a config_handle is @ref IMGPROC_INVALID_CONFIG_HANDLE
 * Or one of arguments is NULL.
 * @retval @ref IMGPROC_ERROR_CONFIG_SECTION_NOT_FOUND Can't find @a section in the configuration.
 * @retval @ref IMGPROC_ERROR_CONFIG_KEY_NOT_FOUND Can't find @a key in @a section.
 * @retval @ref IMGPROC_ERROR_CONFIG_TYPE_MISMATCH The value doesn't represent a floating-point
 * number.
 */
ImgProcStatus imgproc_config_get_double(ImgProcConfigHandle config_handle, const char* section,
                                        const char* key, double* value);//宣告一個函式imgproc_config_get_double，用於從配置中獲取指定節(section)和鍵(key)對應的浮點數值。該函式接受四個參數：ImgProcConfigHandle類型的config_handle，表示配置句柄；指向字符的指針section，表示配置中的節名稱；指向字符的指針key，表示鍵名稱；以及指向double的指針value，用於返回獲取到的浮點數值。函式返回ImgProcStatus類型的狀態碼，表示操作是否成功或失敗，以及失敗的原因。

/**
 * @brief Get a boolean in the configuration.
 *
 * Get a boolean of @a key at @a section from the configuration.
 *
 * @param [in] config_handle The handle to the configuration.
 * @param [in] section A NULL-terminated string represents the section in the configuration.
 * A section is represented by [<section_name>].
 * @param [in] key A NULL-terminated string represents the key of value. A key is represented by
 * <key>=<value>.
 * @param [out] value The pointer to a bool value.
 *
 * @retval @ref IMGPROC_SUCCESS Operation success.
 * @retval @ref IMGPROC_ERROR_INVALID_ARG @a config_handle is @ref IMGPROC_INVALID_CONFIG_HANDLE
 * Or one of arguments is NULL.
 * @retval @ref IMGPROC_ERROR_CONFIG_SECTION_NOT_FOUND Can't find @a section in the configuration.
 * @retval @ref IMGPROC_ERROR_CONFIG_KEY_NOT_FOUND Can't find @a key in @a section.
 * @retval @ref IMGPROC_ERROR_CONFIG_TYPE_MISMATCH The value doesn't represent a boolean.
 */
ImgProcStatus imgproc_config_get_boolean(ImgProcConfigHandle config_handle, const char* section,
                                         const char* key, bool* value);//宣告一個函式imgproc_config_get_boolean，用於從配置中獲取指定節(section)和鍵(key)對應的布林值。該函式接受四個參數：ImgProcConfigHandle類型的config_handle，表示配置句柄；指向字符的指針section，表示配置中的節名稱；指向字符的指針key，表示鍵名稱；以及指向bool的指針value，用於返回獲取到的布林值。函式返回ImgProcStatus類型的狀態碼，表示操作是否成功或失敗，以及失敗的原因。

/**
 * @brief Get a string in the configuration.
 *
 * Get a NULL-terminated string of @a key at @a section from the configuration.
 *
 * @param [in] config_handle The handle to the configuration.
 * @param [in] section A NULL-terminated string represents the section in the configuration.
 * A section is represented by [<section_name>].
 * @param [in] key A NULL-terminated string represents the key of value. A key is represented by
 * <key>=<value>.
 * @param [out] value The pointer to a const char* value.
 *
 * @retval @ref IMGPROC_SUCCESS Operation success.
 * @retval @ref IMGPROC_ERROR_INVALID_ARG @a config_handle is @ref IMGPROC_INVALID_CONFIG_HANDLE
 * Or one of arguments is NULL.
 * @retval @ref IMGPROC_ERROR_CONFIG_SECTION_NOT_FOUND Can't find @a section in the configuration.
 * @retval @ref IMGPROC_ERROR_CONFIG_KEY_NOT_FOUND Can't find @a key in @a section.
 * @retval @ref IMGPROC_ERROR_CONFIG_TYPE_MISMATCH The value doesn't represent a string.
 */
ImgProcStatus imgproc_config_get_string(ImgProcConfigHandle config_handle, const char* section,
                                        const char* key, const char** value);//宣告一個函式imgproc_config_get_string，用於從配置中獲取指定節(section)和鍵(key)對應的字串值。該函式接受四個參數：ImgProcConfigHandle類型的config_handle，表示配置句柄；指向字符的指針section，表示配置中的節名稱；指向字符的指針key，表示鍵名稱；以及指向const char*的指針value，用於返回獲取到的字串值。函式返回ImgProcStatus類型的狀態碼，表示操作是否成功或失敗，以及失敗的原因。

#ifdef __cplusplus
}
#endif