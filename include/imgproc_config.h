#pragma once

#include "imgproc_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief A handle to the configuration. */
typedef struct ImgProcConfig* ImgProcConfigHandle;

/** @brief The value that represents an invalid configuration handle. */
#define IMGPROC_INVALID_CONFIG_HANDLE (NULL)

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
ImgProcStatus imgproc_config_load(ImgProcConfigHandle* config_handle, const char* file);

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
ImgProcStatus imgproc_config_destroy(ImgProcConfigHandle config_handle);

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
                                       const char* key, int64_t* value);

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
                                        const char* key, double* value);

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
                                         const char* key, bool* value);

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
                                        const char* key, const char** value);

#ifdef __cplusplus
}
#endif