#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The max characters of a log message.
 *
 * The logger use a stack buffer with this size and additional one byte (for terminal NULL char) to
 * format message. If the formatted  message is longer than this, the message will be trancated.
 */
#define IMGPROC_MAX_LOG_MSG_LEN (1023)//這個宏定義了日誌消息的最大字符數。日誌記錄器使用一個大小為IMGPROC_MAX_LOG_MSG_LEN的堆棧緩衝區來格式化消息，並額外分配一個字節用於終止NULL字符。如果格式化後的消息長度超過此限制，則消息將被截斷。

/**
 * @brief Represent the logging level.
 */
typedef enum {
    IMGPROC_LOGLEVEL_NONE = 0,  /**< No logging. */
    IMGPROC_LOGLEVEL_ERROR = 1, /**< Log only error-level messages. */
    IMGPROC_LOGLEVEL_WARN = 2,  /**< Log warning-level and above messages. */
    IMGPROC_LOGLEVEL_INFO = 3,  /**< Log information-level and above messages. */
    IMGPROC_LOGLEVEL_DEBUG = 4  /**< Log debug-level and above messages. */
} ImgProcLogLevel;//這個枚舉類型表示日誌級別。

/**
 * @brief The signature of a logging function.
 *
 * @param [in] source A NULL-terminated string represents the source who generated the message.
 * @param [in] level The logging level of this message.
 * @param [in] msg A NULL-terminated message.
 * @param [in] user_data The user-defined data passed to the logger.
 */
typedef void (*ImgProcLogFn)(const char* source, ImgProcLogLevel level, const char* msg,
                             void* user_data);//這個函數指針類型定義了日誌函數的簽名。它接受四個參數：source表示生成消息的源，level表示消息的日誌級別，msg是要記錄的消息，user_data是傳遞給日誌記錄器的用戶自定義數據。

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
void imgproc_logger_set_logger(ImgProcLogFn fn, void* user_data);//這個函數設置日誌函數，並傳遞用戶自定義數據。fn是日誌記錄器將調用的日誌函數，caller可以將其設置為NULL以重置日誌記錄器。user_data是將傳遞給日誌函數的用戶自定義數據，該數據由caller擁有，這是可選的。請注意，此函數不是多線程安全的。

/**
 * @brief Set the logging level.
 *
 * @param [in] level The logging level.
 *
 * @note This function is MT-safe.
 */
void imgproc_logger_set_level(ImgProcLogLevel level);//這個函數設置日誌級別。level是要設置的日誌級別。請注意，此函數是多線程安全的。

/**
 * @brief Format and log message.
 *
 * @param [in] source A NULL-terminated source who generated the log message.
 * @param [in] level The logging level of the message.
 * @param [in] fmt A NULL-terminated format string to format the message.
 *
 * @note This function is MT-safe.
 */
void imgproc_logger_log_fmt(const char* source, ImgProcLogLevel level, const char* fmt, ...);//這個函數格式化並記錄消息。source是生成日誌消息的源，level是消息的日誌級別，fmt是一個NULL終止的格式字符串，用於格式化消息。請注意，此函數是多線程安全的。

/**
 * @brief Use pre-defined console logger.
 *
 * Set the logging function to internal defined console logging function. The message format is
 * [<level>] [<source>] <message>.
 *
 * @note This function is NOT MT-safe.
 */
void imgproc_logger_use_console();//這個函數使用預定義的控制台日誌記錄器。它將日誌函數設置為內部定義的控制台日誌記錄函數。消息格式為[<level>] [<source>] <message>。請注意，此函數不是多線程安全的。

#define IMGPROC_LOGGER_STR(x) #x
#define IMGPROC_LOGGER_XSTR(x) IMGPROC_LOGGER_STR(x)
#define IMGPROC_LOGGER_LOCATION __FILE__ ":" IMGPROC_LOGGER_XSTR(__LINE__)
//這些宏定義用於生成日誌消息的源位置字符串。IMGPROC_LOGGER_STR(x)將參數x轉換為字符串，IMGPROC_LOGGER_XSTR(x)用於展開宏參數，IMGPROC_LOGGER_LOCATION生成當前文件名和行號的字符串表示形式，用於標識日誌消息的來源位置。
#define IMGPROC_LOG_ERROR(fmt, ...) \
    imgproc_logger_log_fmt(IMGPROC_LOGGER_LOCATION, IMGPROC_LOGLEVEL_ERROR, (fmt), ##__VA_ARGS__)
#define IMGPROC_LOG_WARN(fmt, ...) \
    imgproc_logger_log_fmt(IMGPROC_LOGGER_LOCATION, IMGPROC_LOGLEVEL_WARN, (fmt), ##__VA_ARGS__)
#define IMGPROC_LOG_INFO(fmt, ...) \
    imgproc_logger_log_fmt(IMGPROC_LOGGER_LOCATION, IMGPROC_LOGLEVEL_INFO, (fmt), ##__VA_ARGS__)
#define IMGPROC_LOG_DEBUG(fmt, ...) \
    imgproc_logger_log_fmt(IMGPROC_LOGGER_LOCATION, IMGPROC_LOGLEVEL_DEBUG, (fmt), ##__VA_ARGS__)
//這些宏定義用於方便地記錄不同級別的日誌消息。IMGPROC_LOG_ERROR、IMGPROC_LOG_WARN、IMGPROC_LOG_INFO和IMGPROC_LOG_DEBUG分別用於記錄錯誤、警告、信息和調試級別的日誌消息。它們使用imgproc_logger_log_fmt函數，並自動將源位置設置為當前文件名和行號。
#ifdef __cplusplus
}
#endif