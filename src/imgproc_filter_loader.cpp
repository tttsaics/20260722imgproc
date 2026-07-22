#include "imgproc_filter_loader.h"

#include <dlfcn.h>

#include <functional>
#include <memory>

#include "imgproc_logger.h"
#include "priv/imgproc_helper.h"

struct DlHandleDeleter { //運用 RAII（資源獲取即初始化）原則。如果在載入函數的過程中發生錯誤提前 return，unique_ptr 會自動呼叫 dlclose，避免記憶體或動態庫控制代碼（Handle）洩漏。
    void operator()(void* dl_handle) {
        if (dl_handle) {
            dlclose(dl_handle);
        }
    }
};

typedef const char* (*ImgProcGetFilterFuncNameFn)(); //定義了一個函式指標類型 ImgProcGetFilterFuncNameFn，該函式指標指向一個返回 const char* 的函式，用於獲取濾鏡函式的名稱。

template <typename T>//定義了一個模板函式 dlsym_typed，用於從動態庫中獲取指定名稱的函式指標，並將其轉換為指定的函式指標類型 T。
static T dlsym_typed(void* dl_handle, const char* func_name) {
    static_assert(std::is_pointer_v<T>);//使用 static_assert 來檢查 T 是否為指標類型，確保函式指標的正確性。
    void* fn_ptr = dlsym(dl_handle, func_name);//使用 dlsym 函式從動態庫中獲取指定名稱的函式指標，返回值為 void*，需要進行類型轉換。
    return reinterpret_cast<T>(fn_ptr); //將 void* 類型的函式指標轉換為指定的函式指標類型 T，並返回。
}

template <typename T> //兩階段濾鏡函式載入樣板函式 
static T load_filter_func(void* dl_handle, const char* func_name_getter_name) {
    static_assert(std::is_pointer_v<T>);//使用 static_assert 來檢查 T 是否為指標類型，確保函式指標的正確性。

    auto func_name_getter =
        dlsym_typed<ImgProcGetFilterFuncNameFn>(dl_handle, func_name_getter_name);//使用 dlsym_typed 函式從動態庫中獲取指定名稱的函式指標，返回值為 ImgProcGetFilterFuncNameFn 類型的函式指標 func_name_getter。
    if (!func_name_getter) {
        IMGPROC_LOG_ERROR("Filter function name getter is not declared.");
        return nullptr;
    }

    auto target_func = dlsym_typed<T>(dl_handle, func_name_getter());//使用 func_name_getter 函式指標調用函式，獲取濾鏡函式的名稱，然後使用 dlsym_typed 函式從動態庫中獲取指定名稱的濾鏡函式指標，返回值為 T 類型的函式指標 target_func。
    if (!target_func) {
        IMGPROC_LOG_ERROR("Filter function is not declared.");
        return nullptr;
    }

    return target_func;
}

ImgProcStatus imgproc_filter_load_api(ImgProcFilterApi* api, const char* lib_file) {//這個函式負責從指定的共享庫文件中加載濾鏡插件的 API，並將其存儲在 ImgProcFilterApi 結構體中。它首先檢查輸入參數是否為空，然後使用 dlopen 函式打開共享庫文件。如果打開失敗，則返回錯誤狀態。接著，它使用 load_filter_func 模板函式分別加載濾鏡的創建、銷毀和轉換函式指標。如果任何一個函式加載失敗，則返回錯誤狀態。最後，它將加載的函式指標存儲在 ImgProcFilterApi 結構體中，並返回成功狀態。
    if (!api) {
        IMGPROC_LOG_ERROR("'%s' is null", IMGPROC_STR(api));
        return IMGPROC_ERROR_INVALID_ARG;
    }

    if (!lib_file) {
        IMGPROC_LOG_ERROR("'%s' is null", IMGPROC_STR(lib_file));
        return IMGPROC_ERROR_INVALID_ARG;
    }

    void* dl_handle = dlopen(lib_file, RTLD_LAZY);
    if (!dl_handle) {
        IMGPROC_LOG_ERROR("Unable to load shared library '%s': %s", lib_file, dlerror());
        return IMGPROC_ERROR_LOAD_FILTER;
    }

    std::unique_ptr<void, DlHandleDeleter> dl_handle_ptr;
    ImgProcFilterCreateFn create_fn = nullptr;
    ImgProcFilterDestroyFn destroy_fn = nullptr;
    ImgProcFilterTransformFn transform_fn = nullptr;

    dl_handle_ptr.reset(dl_handle);
    dl_handle = nullptr;

    create_fn = load_filter_func<ImgProcFilterCreateFn>(dl_handle_ptr.get(),
                                                        IMGPROC_FILTER_FN_NAME_STR(create));//(載入濾鏡create)此行代碼使用 load_filter_func 模板函式從動態庫中加載濾鏡的創建函式指標，並將其存儲在 create_fn 變數中。它傳遞了 dl_handle_ptr.get() 作為動態庫句柄，以及 IMGPROC_FILTER_FN_NAME_STR(create) 作為函式名稱獲取器的名稱。如果加載失敗，則返回錯誤狀態。
    if (!create_fn) {
        IMGPROC_LOG_ERROR("Unable to load filter create function.");
        return IMGPROC_ERROR_LOAD_FILTER;
    }

    destroy_fn = load_filter_func<ImgProcFilterDestroyFn>(dl_handle_ptr.get(),
                                                          IMGPROC_FILTER_FN_NAME_STR(destroy));//(載入濾鏡destroy)此行代碼使用 load_filter_func 模板函式從動態庫中加載濾鏡的銷毀函式指標，並將其存儲在 destroy_fn 變數中。它傳遞了 dl_handle_ptr.get() 作為動態庫句柄，以及 IMGPROC_FILTER_FN_NAME_STR(destroy) 作為函式名稱獲取器的名稱。如果加載失敗，則返回錯誤狀態。
    if (!destroy_fn) {
        IMGPROC_LOG_ERROR("Unable to load filter destroy function.");
        return IMGPROC_ERROR_LOAD_FILTER;
    }

    transform_fn = load_filter_func<ImgProcFilterTransformFn>(
        dl_handle_ptr.get(), IMGPROC_FILTER_FN_NAME_STR(transform));//(載入濾鏡transform)此行代碼使用 load_filter_func 模板函式從動態庫中加載濾鏡的轉換函式指標，並將其存儲在 transform_fn 變數中。它傳遞了 dl_handle_ptr.get() 作為動態庫句柄，以及 IMGPROC_FILTER_FN_NAME_STR(transform) 作為函式名稱獲取器的名稱。如果加載失敗，則返回錯誤狀態。
    if (!transform_fn) {
        IMGPROC_LOG_ERROR("Unable to load filter transform function.");
        return IMGPROC_ERROR_LOAD_FILTER;
    }

    api->create = create_fn;
    api->destroy = destroy_fn;
    api->transform = transform_fn;
    api->internal_data = dl_handle_ptr.release();

    return IMGPROC_SUCCESS;
}

ImgProcStatus imgproc_filter_destroy_api(ImgProcFilterApi* api) {//資源釋放函式，負責釋放 ImgProcFilterApi 結構體中存儲的濾鏡插件 API。它首先檢查輸入參數是否為空，如果為空則返回錯誤狀態。接著，它檢查 internal_data 是否為空，如果不為空，則使用 dlclose 函式關閉動態庫，並將 create、destroy、transform 函式指標設置為 nullptr，最後將 internal_data 設置為 nullptr。最後，它返回成功狀態。
    if (!api) {
        IMGPROC_LOG_ERROR("'%s' is null.", IMGPROC_STR(api));
        return IMGPROC_ERROR_INVALID_ARG;
    }

    if (api->internal_data) {
        dlclose(api->internal_data);
        api->create = nullptr;
        api->destroy = nullptr;
        api->transform = nullptr;
        api->internal_data = nullptr;
    }

    return IMGPROC_SUCCESS;
}