#include <imgproc_config.h>
#include <imgproc_logger.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define TEST_GET_VALUE_SECTION "test_get_value"

#define TEST_PRINT_CALL_ERROR(expr, status) fprintf(stderr, "%s error: %s\n", #expr, imgproc_get_status_str((status)))//用於格式化輸出函式調用錯誤訊息，將函式名稱和狀態碼轉換為可讀的字串

static bool test_get_int(ImgProcConfigHandle config_handle) {//測試讀取整數
    ImgProcStatus status = IMGPROC_SUCCESS;
    const int64_t expected = 123;
    int64_t value = 0;

    status = imgproc_config_get_int64(config_handle, TEST_GET_VALUE_SECTION, "int_value", &value);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_config_get_int64, status);
        return false;
    }

    if (value != expected) {
        fprintf(stderr, "%s failed, expected: %ld, actual: %ld\n", __func__, expected, value);
        return false;
    }

    return true;
}

static bool test_get_double(ImgProcConfigHandle config_handle) {//測試讀取浮點數
    ImgProcStatus status = IMGPROC_SUCCESS;
    const double expected = 12.3;
    double value = 0;

    status = imgproc_config_get_double(config_handle, TEST_GET_VALUE_SECTION, "dbl_value", &value);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_config_get_double, status);
        return false;
    }

    if (fabs(value - expected) > 0.000001) {
        fprintf(stderr, "%s failed, expected: %.6f, actual: %.6f\n", __func__, expected, value);
        return false;
    }

    return true;
}

static bool test_get_boolean(ImgProcConfigHandle config_handle) {//測試讀取布林值
    ImgProcStatus status = IMGPROC_SUCCESS;
    bool value = true;

    status = imgproc_config_get_boolean(config_handle, TEST_GET_VALUE_SECTION, "bool_value_false", &value);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_config_get_boolean, status);
        return false;
    }

    if (value != false) {
        fprintf(stderr, "%s failed, expected: false\n", __func__);
        return false;
    }

    status = imgproc_config_get_boolean(config_handle, TEST_GET_VALUE_SECTION, "bool_value_true", &value);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_config_get_boolean, status);
        return false;
    }

    if (value != true) {
        fprintf(stderr, "%s failed, expected: true\n", __func__);
        return false;
    }

    return true;
}

static bool test_get_string(ImgProcConfigHandle config_handle) {//測試讀取字串
    ImgProcStatus status = IMGPROC_SUCCESS;
    const char* expected = "hello world";
    const char* value = "";

    status = imgproc_config_get_string(config_handle, TEST_GET_VALUE_SECTION, "str_value", &value);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_config_get_string, status);
        return false;
    }

    if (strcmp(value, expected) != 0) {
        fprintf(stderr, "%s failed, expected: %s, actual: %s\n", __func__, expected, value);
        return false;
    }

    return true;
}

int main() {
    imgproc_logger_use_console();
    imgproc_logger_set_level(IMGPROC_LOGLEVEL_INFO);//設置日誌級別為INFO

    ImgProcStatus status = IMGPROC_SUCCESS;
    ImgProcConfigHandle config_handle = IMGPROC_INVALID_CONFIG_HANDLE;

    status = imgproc_config_load(&config_handle, "test_config.toml");
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_config_load, status);
        return EXIT_FAILURE;
    }

    int ret = EXIT_FAILURE;

    if (!test_get_int(config_handle)) {
        goto clean;
    }

    if (!test_get_double(config_handle)) {
        goto clean;
    }

    if (!test_get_boolean(config_handle)) {
        goto clean;
    }

    if (!test_get_string(config_handle)) {
        goto clean;
    }

    ret = EXIT_SUCCESS;

clean:
    status = imgproc_config_destroy(config_handle);
    if (status != IMGPROC_SUCCESS) {
        TEST_PRINT_CALL_ERROR(imgproc_config_destroy, status);
    }

    return ret;
}