#include "imgproc_filter_loader.h"

#include <dlfcn.h>

#include <functional>
#include <memory>

#include "imgproc_logger.h"
#include "priv/imgproc_helper.h"

struct DlHandleDeleter {
    void operator()(void* dl_handle) {
        if (dl_handle) {
            dlclose(dl_handle);
        }
    }
};

typedef const char* (*ImgProcGetFilterFuncNameFn)();

template <typename T>
static T dlsym_typed(void* dl_handle, const char* func_name) {
    static_assert(std::is_pointer_v<T>);
    void* fn_ptr = dlsym(dl_handle, func_name);
    return reinterpret_cast<T>(fn_ptr);
}

template <typename T>
static T load_filter_func(void* dl_handle, const char* func_name_getter_name) {
    static_assert(std::is_pointer_v<T>);

    auto func_name_getter =
        dlsym_typed<ImgProcGetFilterFuncNameFn>(dl_handle, func_name_getter_name);
    if (!func_name_getter) {
        IMGPROC_LOG_ERROR("Filter function name getter is not declared.");
        return nullptr;
    }

    auto target_func = dlsym_typed<T>(dl_handle, func_name_getter());
    if (!target_func) {
        IMGPROC_LOG_ERROR("Filter function is not declared.");
        return nullptr;
    }

    return target_func;
}

ImgProcStatus imgproc_filter_load_api(ImgProcFilterApi* api, const char* lib_file) {
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
                                                        IMGPROC_FILTER_FN_NAME_STR(create));
    if (!create_fn) {
        IMGPROC_LOG_ERROR("Unable to load filter create function.");
        return IMGPROC_ERROR_LOAD_FILTER;
    }

    destroy_fn = load_filter_func<ImgProcFilterDestroyFn>(dl_handle_ptr.get(),
                                                          IMGPROC_FILTER_FN_NAME_STR(destroy));
    if (!destroy_fn) {
        IMGPROC_LOG_ERROR("Unable to load filter destroy function.");
        return IMGPROC_ERROR_LOAD_FILTER;
    }

    transform_fn = load_filter_func<ImgProcFilterTransformFn>(
        dl_handle_ptr.get(), IMGPROC_FILTER_FN_NAME_STR(transform));
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

ImgProcStatus imgproc_filter_destroy_api(ImgProcFilterApi* api) {
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