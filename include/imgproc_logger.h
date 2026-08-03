#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The max characters of a log message.
 *
 * The logger use a stack buffer with this size and additional one byte (for terminal NULL char) to
 * format message. If the formatted  message is longer than this, the message will be trancated.
 */
#define IMGPROC_MAX_LOG_MSG_LEN (1023)

/**
 * @brief Represent the logging level.
 */
typedef enum {
    IMGPROC_LOGLEVEL_NONE = 0,  /**< No logging. */
    IMGPROC_LOGLEVEL_ERROR = 1, /**< Log only error-level messages. */
    IMGPROC_LOGLEVEL_WARN = 2,  /**< Log warning-level and above messages. */
    IMGPROC_LOGLEVEL_INFO = 3,  /**< Log information-level and above messages. */
    IMGPROC_LOGLEVEL_DEBUG = 4  /**< Log debug-level and above messages. */
} ImgProcLogLevel;

/**
 * @brief The signature of a logging function.
 *
 * @param [in] source A NULL-terminated string represents the source who generated the message.
 * @param [in] level The logging level of this message.
 * @param [in] msg A NULL-terminated message.
 * @param [in] user_data The user-defined data passed to the logger.
 */
typedef void (*ImgProcLogFn)(const char* source, ImgProcLogLevel level, const char* msg,
                             void* user_data);

/**
 * @brief Set the logging function.
 *
 * Set the logging function and pass user-defined data.
 *
 * @param [in] fn The logging function will call by the logger. Caller can set to NULL to reset the
 * logger.
 * @param [in] user_data The user-defined data will be passed to the logging function. The data is
 * owned by the caller. This is optional.
 *
 * @note This function is NOT MT-safe.
 */
void imgproc_logger_set_logger(ImgProcLogFn fn, void* user_data);

/**
 * @brief Set the logging level.
 *
 * @param [in] level The logging level.
 *
 * @note This function is MT-safe.
 */
void imgproc_logger_set_level(ImgProcLogLevel level);

/**
 * @brief Format and log message.
 *
 * @param [in] source A NULL-terminated source who generated the log message.
 * @param [in] level The logging level of the message.
 * @param [in] fmt A NULL-terminated format string to format the message.
 *
 * @note This function is MT-safe.
 */
void imgproc_logger_log_fmt(const char* source, ImgProcLogLevel level, const char* fmt, ...);

/**
 * @brief Use pre-defined console logger.
 *
 * Set the logging function to internal defined console logging function. The message format is
 * [<level>] [<source>] <message>.
 *
 * @note This function is NOT MT-safe.
 */
void imgproc_logger_use_console();

#define IMGPROC_LOGGER_STR(x) #x
#define IMGPROC_LOGGER_XSTR(x) IMGPROC_LOGGER_STR(x)
#define IMGPROC_LOGGER_LOCATION __FILE__ ":" IMGPROC_LOGGER_XSTR(__LINE__)
#define IMGPROC_LOG_ERROR(fmt, ...) \
    imgproc_logger_log_fmt(IMGPROC_LOGGER_LOCATION, IMGPROC_LOGLEVEL_ERROR, (fmt), ##__VA_ARGS__)
#define IMGPROC_LOG_WARN(fmt, ...) \
    imgproc_logger_log_fmt(IMGPROC_LOGGER_LOCATION, IMGPROC_LOGLEVEL_WARN, (fmt), ##__VA_ARGS__)
#define IMGPROC_LOG_INFO(fmt, ...) \
    imgproc_logger_log_fmt(IMGPROC_LOGGER_LOCATION, IMGPROC_LOGLEVEL_INFO, (fmt), ##__VA_ARGS__)
#define IMGPROC_LOG_DEBUG(fmt, ...) \
    imgproc_logger_log_fmt(IMGPROC_LOGGER_LOCATION, IMGPROC_LOGLEVEL_DEBUG, (fmt), ##__VA_ARGS__)
#ifdef __cplusplus
}
#endif