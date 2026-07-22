#include "imgproc_utils.h"

const char* imgproc_get_status_str(ImgProcStatus status) {
    switch (status) {
        case IMGPROC_SUCCESS:
            return "Success";
        case IMGPROC_ERROR_RUNTIME:
            return "Runtime error";
        case IMGPROC_ERROR_INVALID_ARG:
            return "Invalid argument";
        case IMGPROC_ERROR_INVALID_CONFIG_FILE:
            return "Invalid configuration file";
        case IMGPROC_ERROR_CONFIG_SECTION_NOT_FOUND:
            return "Section is not found in the configuration";
        case IMGPROC_ERROR_CONFIG_KEY_NOT_FOUND:
            return "Key is not found in the configuration";
        case IMGPROC_ERROR_CONFIG_TYPE_MISMATCH:
            return "The type of value mismatches to the calling function";
        case IMGPROC_ERROR_OUT_OF_MEMOERY:
            return "Out of memory";
        case IMGPROC_ERROR_LOAD_FILTER:
            return "Failed to load a filter from the given file";
        case IMGPROC_ERROR_FILTER_SPECIFIC:
            return "Filter specific error";
        default:
            return "Unkown";
    }
}