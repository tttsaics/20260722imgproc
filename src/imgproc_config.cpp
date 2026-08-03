#include "imgproc_config.h"

#define TOML_EXCEPTIONS 0

#include <toml++/toml.hpp>
#include <type_traits>

#include "imgproc_logger.h"
#include "priv/imgproc_helper.h"

#define IMGRPOC_CONFIG_FROM_HANDLE(handle) ((ImgProcConfig*)(handle))

struct ImgProcConfig {//這個結構體用於封裝配置的內部資料結構，包含一個toml::table成員，用於存儲解析後的TOML配置數據。
    toml::table table;
};

template <typename T>//這個函式用於獲取配置中的值，根據傳入的section和key，從配置中查找對應的值，並將其存儲到value指針所指向的變數中。它會檢查傳入的參數是否有效，並返回相應的狀態碼。
static const char* get_human_readable_type_name() {
    if constexpr (std::is_same_v<T, int64_t>) {
        return "an integer";
    } else if constexpr (std::is_same_v<T, double>) {
        return "a floating point number";
    } else if constexpr (std::is_same_v<T, bool>) {
        return "a boolean";
    } else if constexpr (std::is_same_v<T, std::string>) {
        return "a string";
    } else {
        static_assert(false, "Unsupported type.");
    }
}

template <typename T>//(核心取得)這個函式用於獲取配置中的值，根據傳入的section和key，從配置中查找對應的值，並將其存儲到value指針所指向的變數中。它會檢查傳入的參數是否有效，並返回相應的狀態碼。
static ImgProcStatus imgproc_config_get(ImgProcConfigHandle config_handle, const char* section,
                                        const char* key, T* value) {
    if (config_handle == IMGPROC_INVALID_CONFIG_HANDLE) {
        IMGPROC_LOG_ERROR("Invalid config handle.");
        return IMGPROC_ERROR_INVALID_ARG;
    }

    if (!section) {
        IMGPROC_LOG_ERROR("'%s' is null.", IMGPROC_STR(section));
        return IMGPROC_ERROR_INVALID_ARG;
    }

    if (!key) {
        IMGPROC_LOG_ERROR("'%s' is null.", IMGPROC_STR(key));
        return IMGPROC_ERROR_INVALID_ARG;
    }

    if (!value) {
        IMGPROC_LOG_ERROR("'%s' is null.", IMGPROC_STR(value));
        return IMGPROC_ERROR_INVALID_ARG;
    }

    ImgProcConfig* config = IMGRPOC_CONFIG_FROM_HANDLE(config_handle);
    toml::node_view section_node = config->table[section];
    if (!section_node.node()) {
        IMGPROC_LOG_ERROR("Can't find section '%s'.", section);
        return IMGPROC_ERROR_CONFIG_SECTION_NOT_FOUND;
    }

    toml::node_view value_node = section_node[key];
    if (!value_node.node()) {
        IMGPROC_LOG_ERROR("Can't find key '%s'.", key);
        return IMGPROC_ERROR_CONFIG_KEY_NOT_FOUND;
    }

    if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, double> ||
                  std::is_same_v<T, bool> || std::is_same_v<T, std::string>) {//型別檢查，確保T是支持的類型
        const auto* wrap_node = value_node.as<T>();
        if (!wrap_node) {
            IMGPROC_LOG_ERROR("The value of '%s' in section '%s' doesn't represent %s.", key,
                              section, get_human_readable_type_name<T>());
            return IMGPROC_ERROR_CONFIG_TYPE_MISMATCH;
        }

        *value = wrap_node->get();
    } else {
        static_assert(false, "Unsupported type.");
    }

    return IMGPROC_SUCCESS;
}

ImgProcStatus imgproc_config_load(ImgProcConfigHandle* config_handle, const char* file) {//(載入配置)這個函式用於從指定的TOML配置文件中加載配置，並將其存儲到ImgProcConfig結構體中。它會檢查傳入的參數是否有效，並返回相應的狀態碼。
    if (config_handle == IMGPROC_INVALID_CONFIG_HANDLE) {
        IMGPROC_LOG_ERROR("Invalid config handle.");
        return IMGPROC_ERROR_INVALID_ARG;
    }

    if (!file) {
        IMGPROC_LOG_ERROR("'%s' is null.", IMGPROC_STR(file));
        return IMGPROC_ERROR_INVALID_ARG;
    }

    toml::parse_result result = toml::parse_file(std::string_view{file});
    if (result.failed()) {
        IMGPROC_LOG_ERROR("toml::parse_file error: %s",
                          std::string{result.error().description()}.c_str());
        return IMGPROC_ERROR_INVALID_CONFIG_FILE;
    }

    ImgProcConfig* config = new (std::nothrow) ImgProcConfig;
    if (!config) {
        IMGPROC_LOG_ERROR("Unable to allocate memory for '%s'.", IMGPROC_STR(ImgProcConfig));
        return IMGPROC_ERROR_OUT_OF_MEMOERY;
    }

    config->table = std::move(result.table());//將解析後的TOML表格移動到ImgProcConfig結構體的table成員中，完成配置的加載。
    IMGPROC_LOG_INFO("Load config from file \"%s\" successfully.", file);
    *config_handle = config;

    return IMGPROC_SUCCESS;
}

ImgProcStatus imgproc_config_destroy(ImgProcConfigHandle config_handle) {
    if (config_handle == IMGPROC_INVALID_CONFIG_HANDLE) {
        return IMGPROC_ERROR_INVALID_ARG;
    }

    delete config_handle;

    return IMGPROC_SUCCESS;
}
//以下函式，這些函式都是用於從配置中獲取不同類型的值，根據傳入的section和key，從配置中查找對應的值，並將其存儲到value指針所指向的變數中。它們會檢查傳入的參數是否有效，並返回相應的狀態碼。
ImgProcStatus imgproc_config_get_int64(ImgProcConfigHandle config_handle, const char* section,
                                       const char* key, int64_t* value) {
    return imgproc_config_get<int64_t>(config_handle, section, key, value);
}

ImgProcStatus imgproc_config_get_double(ImgProcConfigHandle config_handle, const char* section,
                                        const char* key, double* value) {
    return imgproc_config_get<double>(config_handle, section, key, value);
}

ImgProcStatus imgproc_config_get_boolean(ImgProcConfigHandle config_handle, const char* section,
                                         const char* key, bool* value) {
    return imgproc_config_get<bool>(config_handle, section, key, value);
}

ImgProcStatus imgproc_config_get_string(ImgProcConfigHandle config_handle, const char* section,
                                        const char* key, const char** value) {
    if (config_handle == IMGPROC_INVALID_CONFIG_HANDLE) {
        IMGPROC_LOG_ERROR("Invalid config handle.");
        return IMGPROC_ERROR_INVALID_ARG;
    }

    if (!section) {
        IMGPROC_LOG_ERROR("'%s' is null.", IMGPROC_STR(section));
        return IMGPROC_ERROR_INVALID_ARG;
    }

    if (!key) {
        IMGPROC_LOG_ERROR("'%s' is null.", IMGPROC_STR(key));
        return IMGPROC_ERROR_INVALID_ARG;
    }

    if (!value) {
        IMGPROC_LOG_ERROR("'%s' is null.", IMGPROC_STR(value));
        return IMGPROC_ERROR_INVALID_ARG;
    }

    ImgProcConfig* config = IMGRPOC_CONFIG_FROM_HANDLE(config_handle);//從table找到對應字串節點
    toml::node_view section_node = config->table[section];//檢查section是否存在
    if (!section_node.node()) {
        IMGPROC_LOG_ERROR("Can't find section '%s'.", section);
        return IMGPROC_ERROR_CONFIG_SECTION_NOT_FOUND;
    }

    toml::node_view value_node = section_node[key];//檢查key是否存在
    if (!value_node.node()) {
        IMGPROC_LOG_ERROR("Can't find key '%s'.", key);
        return IMGPROC_ERROR_CONFIG_KEY_NOT_FOUND;
    }

    const auto* wrap_node = value_node.as<std::string>();//檢查key對應的值是否為字串類型
    if (!wrap_node) {
        IMGPROC_LOG_ERROR("The value of '%s' in section '%s' doesn't represent a string.", key,
                          section);
        return IMGPROC_ERROR_CONFIG_TYPE_MISMATCH;
    }

    *value = wrap_node->get().c_str();//將字串值存儲到value指針所指向的變數中

    return IMGPROC_SUCCESS;
}