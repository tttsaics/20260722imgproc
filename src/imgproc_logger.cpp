#include "imgproc_logger.h"

#include <atomic>
#include <cstdarg>
#include <cstdio>

static ImgProcLogFn g_log_fn = nullptr;
static void* g_user_data = nullptr;
static std::atomic<ImgProcLogLevel> g_level = IMGPROC_LOGLEVEL_NONE;

static const char* get_level_string(ImgProcLogLevel level) {
    switch (level) {
        case IMGPROC_LOGLEVEL_NONE:
            return nullptr;
        case IMGPROC_LOGLEVEL_ERROR:
            return "ERR";
        case IMGPROC_LOGLEVEL_WARN:
            return "WRN";
        case IMGPROC_LOGLEVEL_INFO:
            return "INF";
        case IMGPROC_LOGLEVEL_DEBUG:
            return "DBG";
    }

    return nullptr;
}

static void console_log_fn(const char* source, ImgProcLogLevel level, const char* msg,
                           [[maybe_unused]] void* user_data) {//為使用者能夠在控制台上查看日誌消息，這個函數將日誌消息格式化並輸出到標準輸出流。它使用printf函數將日誌級別、源和消息打印到控制台。
    std::printf("[%s] [%s] %s\n", get_level_string(level), source, msg);
}

void imgproc_logger_set_logger(ImgProcLogFn fn, void* user_data) { 
    g_log_fn = fn;
    g_user_data = user_data;
}

void imgproc_logger_set_level(ImgProcLogLevel level) { g_level = level; }//這個函式用於設置日誌級別，通過將傳入的level值存儲到g_level原子變數中，從而控制日誌消息的輸出級別。

void imgproc_logger_log_fmt(const char* source, ImgProcLogLevel level, const char* fmt, ...) {//紀錄日誌消息
    if (g_log_fn && level <= g_level.load()) {
        char msg_buf[IMGPROC_MAX_LOG_MSG_LEN + 1];
        va_list args;

        va_start(args, fmt);
        vsnprintf(msg_buf, sizeof(msg_buf), fmt, args);
        va_end(args);

        g_log_fn(source, level, msg_buf, g_user_data);
    }
}

void imgproc_logger_use_console() {//將日誌輸出設回控制台
    g_log_fn = console_log_fn;
    g_user_data = nullptr;
}
