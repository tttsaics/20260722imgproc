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
                           [[maybe_unused]] void* user_data) {
    std::printf("[%s] [%s] %s\n", get_level_string(level), source, msg);
}

void imgproc_logger_set_logger(ImgProcLogFn fn, void* user_data) {
    g_log_fn = fn;
    g_user_data = user_data;
}

void imgproc_logger_set_level(ImgProcLogLevel level) { g_level = level; }

void imgproc_logger_log_fmt(const char* source, ImgProcLogLevel level, const char* fmt, ...) {
    if (g_log_fn && level <= g_level.load()) {
        char msg_buf[IMGPROC_MAX_LOG_MSG_LEN + 1];
        va_list args;

        va_start(args, fmt);
        vsnprintf(msg_buf, sizeof(msg_buf), fmt, args);
        va_end(args);

        g_log_fn(source, level, msg_buf, g_user_data);
    }
}

void imgproc_logger_use_console() {
    g_log_fn = console_log_fn;
    g_user_data = nullptr;
}
