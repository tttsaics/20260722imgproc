# ImgProc - C/C++ 圖像處理框架與動態插件系統說明書

`ImgProc` 是一個高效率、模組化且可擴充的 C/C++ 圖像處理 SDK 框架與 CLI 命令列工具。專案採用 C-ABI 外掛插件架構，支援透過動態鏈結庫（`.so` / `.dll`）進行運行時（Runtime）濾鏡載入、多濾鏡串接處理（Filter Chaining）、TOML 格式設定檔讀取以及圖像 RGB 通道提取。

---

## 1. 專案特點與架構總覽 (Overview & Features)

* **動態插件架構 (Dynamic Plugin Architecture)**：
  濾鏡以動態共享庫獨立開發與編譯，利用 `dlopen` / `dlsym` (Linux) 於運行時動態載入，具備高度解耦與擴充性。
* **多濾鏡鏈接處理 (Multi-Filter Chaining)**：
  CLI 主程式支援依序鏈接多個濾鏡插件（如 `-f scale.so -c scale.toml -f rotate.so -c rotate.toml`），流暢執行影像處理 pipeline。
* **強大的 TOML 設定解析**：
  整合 `tomlplusplus` 解析器，提供 C-ABI 包裝函式，讓插件能靈活讀取整數位、浮點數、布林值與字串等設定。
* **完整圖像 I/O 支援**：
  整合 `stb_image` 與 `stb_image_write`，支援主流 JPEG, PNG, BMP 檔案解碼與編碼。
* **記憶體安全與自訂釋放機制**：
  `ImgProcImage` 結構包含 `release_fn` 資源釋放回呼函式，確保插件轉譯產生的記憶體能被安全正確釋放，避免 Memory Leak。
* **自動批次處理模式 (Auto Batch Mode)**：
  未傳入參數時自動掃描 `inputs/` 目錄，將批次處理後的結果輸出至 `outputs/` 目錄。

---

## 2. 系統架構圖 (System Architecture)

```mermaid
flowchart TD
    subgraph CLI Orchestrator [CLI 主程式 (imgproc_app)]
        Main[main.cpp CLI]
        Batch[自動批次模組]
    end

    subgraph Core SDK [ImgProc 核心庫 (libimgproc.so)]
        Config[TOML 設定模組\nimgproc_config]
        Loader[動態載入器\nimgproc_filter_loader]
        IO[圖像 I/O 模組\nimgproc_image_io]
        Channel[通道處理模組\nimgproc_channel]
        Logger[日誌模組\nimgproc_logger]
        Utils[通用基礎結構\nimgproc_utils]
    end

    subgraph Dynamic Filters [獨立動態濾鏡插件 (.so)]
        Dummy[dummy_filter.so]
        Grayscale[grayscale_filter.so]
        Mirror[mirror_filter.so]
        Resize[resize_filter.so]
        Rotate[rotate_filter.so]
    end

    Main --> IO
    Main --> Loader
    Main --> Config
    Main --> Channel
    Loader -->|dlopen / dlsym| Dynamic Filters
    Config -->|tomlplusplus| TOML[TOML 設定檔]
    IO -->|stb_image| FileInput[輸入圖片 JPEG/PNG]
    IO -->|stb_image_write| FileOutput[輸出圖片 JPEG/PNG]
```

---

## 3. 專案目錄結構 (Directory Structure)

```text
20260722imgproc-main-5/
├── CMakeLists.txt                # CMake 主構建檔案
├── README.md                     # 專案說明文件
├── main.cpp                      # CLI 高階調度器應用程式
├── include/                      # 公開 C API 標頭檔
│   ├── imgproc_channel.h         # 通道處理模組 API
│   ├── imgproc_config.h          # TOML 設定檔解析 API
│   ├── imgproc_filter_common.h   # 濾鏡插件 C-ABI 介面定義
│   ├── imgproc_filter_loader.h   # 濾鏡動態庫載入器 API
│   ├── imgproc_image_io.h        # 圖像讀寫與資源釋放 API
│   ├── imgproc_logger.h         # 日誌輸出系統 API
│   ├── imgproc_utils.h          # 狀態碼與 ImgProcImage 結構
│   ├── stb_image.h               # stb 圖像解碼第三方標頭檔
│   └── stb_image_write.h         # stb 圖像編碼第三方標頭檔
├── src/                          # 核心庫與濾鏡插件實現
│   ├── dummy_filter.cpp          # 測試用 Dummy 濾鏡實現
│   ├── grayscale_filter.cpp      # 灰階化濾鏡實現
│   ├── mirror_filter.cpp         # 鏡像翻轉濾鏡實現
│   ├── resize_filter.cpp         # 圖像縮放濾鏡實現
│   ├── rotate_filter.cpp         # 圖像旋轉濾鏡實現
│   ├── imgproc_channel.cpp       # 核心通道提取邏輯
│   ├── imgproc_config.cpp        # 核心 TOML 設定讀取邏輯
│   ├── imgproc_filter_loader.cpp # 核心動態庫 dlopen 調度邏輯
│   ├── imgproc_image_io.cpp      # 核心圖像解碼與編碼邏輯
│   ├── imgproc_logger.cpp        # 核心 Log 系統實現
│   └── imgproc_utils.cpp         # 狀態碼字串轉換
├── tests/                        # 自動化測試套件
│   ├── test_config.toml          # 設定測試檔
│   ├── test_grayscale_filter.c   # 灰階濾鏡單元測試
│   ├── test_imgproc_channel.c    # 通道處理單元測試
│   ├── test_imgproc_config.c     # 設定模組單元測試
│   ├── test_imgproc_filter_loader.c # 載入器單元測試
│   ├── test_imgproc_image_io.c   # 圖像 I/O 單元測試
│   ├── test_imgproc_integration.c# 整合測試
│   ├── test_imgproc_resize_filter.c # 縮放濾鏡單元測試
│   └── test_mirror_filter.c     # 鏡像濾鏡單元測試
├── inputs/                       # 測試/批次輸入圖片目錄
└── outputs/                      # 批次輸出圖片目錄
```

---

## 4. 核心模組與 C API 規格 (Core API Reference)

### 4.1 通用資料結構 (`imgproc_utils.h`)

#### `ImgProcImage`
```c
struct ImgProcImage {
    void* data;                       // 圖像原始 RGB/灰階 像素數據緩衝區
    size_t data_size;                 // 緩衝區位元組大小
    uint32_t width;                   // 圖像寬度 (像素)
    uint32_t height;                  // 圖像高度 (像素)
    uint32_t stride;                  // 每列像素位元組數 (Stride / Pitch)
    ImgProcImageReleaseFn release_fn; // 圖像釋放自訂 callback 函式
};
```

#### 狀態碼 `ImgProcStatus`
* `IMGPROC_SUCCESS`: 操作成功 (0)
* `IMGPROC_ERROR_RUNTIME`: 執行階段錯誤
* `IMGPROC_ERROR_INVALID_ARG`: 無效參數 (如傳入 NULL 指針)
* `IMGPROC_ERROR_INVALID_CONFIG_FILE`: 無法解析 TOML 設定檔
* `IMGPROC_ERROR_CONFIG_SECTION_NOT_FOUND`: 找不到 TOML Section
* `IMGPROC_ERROR_CONFIG_KEY_NOT_FOUND`: 找不到 TOML Key
* `IMGPROC_ERROR_CONFIG_TYPE_MISMATCH`: 設定型態不符合
* `IMGPROC_ERROR_OUT_OF_MEMOERY`: 記憶體不足
* `IMGPROC_ERROR_LOAD_FILTER`: 無法載入動態庫或未定義導出函式
* `IMGPROC_ERROR_FILTER_SPECIFIC`: 插件內部特定錯誤

---

### 4.2 設定讀取模組 (`imgproc_config.h`)

解析 TOML 設定檔並擷取內容：
- `imgproc_config_load(config_handle, filepath)`：載入並解析 TOML 檔案。
- `imgproc_config_destroy(config_handle)`：釋放設定檔佔用記憶體。
- `imgproc_config_get_int64(handle, section, key, &value)`：讀取整數。
- `imgproc_config_get_double(handle, section, key, &value)`：讀取雙精度浮點數。
- `imgproc_config_get_boolean(handle, section, key, &value)`：讀取布林值。
- `imgproc_config_get_string(handle, section, key, &value)`：讀取字串。

---

### 4.3 濾鏡動態載入器 (`imgproc_filter_loader.h`)

管理共享庫（`.so`）的動態載入與生命週期：
- `imgproc_filter_load_api(api, lib_file)`：使用 `dlopen` 載入共享庫，並透過 `dlsym` 獲取 `create`、`destroy` 與 `transform` 函式指標。
- `imgproc_filter_destroy_api(api)`：釋放共享庫控制權（`dlclose`）。

---

### 4.4 圖像 I/O 模組 (`imgproc_image_io.h`)

- `imgproc_image_read(filename, &image)`：讀取圖片並解碼為 RGB 3 通道格式。
- `imgproc_image_write_jpg(filename, image, quality)`：將圖像寫入為 JPEG（品質 1-100）。
- `imgproc_image_write_png(filename, image)`：將圖像寫入為 PNG 格式。
- `imgproc_image_destroy(image)`：調用 `release_fn` 安全釋放圖像記憶體。

---

### 4.5 通道提取模組 (`imgproc_channel.h`)

- `imgproc_image_keep_channel(image, keep_ch)`：保留指定之 RGB 通道（`IMGPROC_CHANNEL_R`, `IMGPROC_CHANNEL_G`, `IMGPROC_CHANNEL_B`），將其他通道數值設為 0。

---

## 5. 內建濾鏡插件與設定 (Built-in Filters)

| 濾鏡名稱 | 產出庫名稱 | 設定 Section | 設定範例與參數說明 |
| :--- | :--- | :--- | :--- |
| **Grayscale** | `libgrayscale_filter.so` | `[grayscale_filter]` | `mode = "luminance"` (或 `"average"`) |
| **Mirror** | `libmirror_filter.so` | `[mirror_filter]` | `mode = "horizontal"` (或 `"vertical"`, `"both"`) |
| **Rotate** | `librotate_filter.so` | `[rotate_filter]` | `angle = 90` (支援 90, 180, 270) |
| **Resize** | `libresize_filter.so` | `[resize_filter]` | `scale = 0.5`, `method = "bilinear"` (或 `"nearest"`) |
| **Dummy** | `libdummy_filter.so` | `[dummy_filter]` | 無需特定參數（用於測試架構與傳輸） |

---

## 6. 自訂濾鏡插件開發指南 (Filter Plugin Development)

要開發新的濾鏡插件，請遵循 C-ABI 導出介面規格：

```cpp
#include <imgproc_filter_common.h>
#include <cstdlib>
#include <cstring>

// 插件內部上下文結構
typedef struct {
    double factor;
} CustomFilterContext;

// 1. 建立濾鏡資源與讀取設定
extern "C" ImgProcStatus custom_filter_create(ImgProcFilterHandle* handle, ImgProcConfigHandle config) {
    CustomFilterContext* ctx = new CustomFilterContext{ 1.0 };
    if (config != IMGPROC_INVALID_CONFIG_HANDLE) {
        imgproc_config_get_double(config, "custom_filter", "factor", &ctx->factor);
    }
    *handle = (ImgProcFilterHandle)ctx;
    return IMGPROC_SUCCESS;
}

// 2. 釋放濾鏡資源
extern "C" ImgProcStatus custom_filter_destroy(ImgProcFilterHandle handle) {
    if (!handle) return IMGPROC_ERROR_INVALID_ARG;
    delete (CustomFilterContext*)handle;
    return IMGPROC_SUCCESS;
}

// 3. 圖像轉譯處理
extern "C" ImgProcStatus custom_filter_transform(ImgProcFilterHandle handle, ImgProcImage* input, ImgProcImage** output) {
    if (!handle || !input || !output) return IMGPROC_ERROR_INVALID_ARG;
    
    ImgProcImage* out = new ImgProcImage();
    out->width = input->width;
    out->height = input->height;
    out->stride = input->stride;
    out->data_size = input->data_size;
    out->data = std::malloc(out->data_size);
    out->release_fn = [](ImgProcImage* img) {
        if (img) {
            std::free(img->data);
            img->data = nullptr;
        }
    };

    // 進行圖像像素處理...
    std::memcpy(out->data, input->data, input->data_size);

    *output = out;
    return IMGPROC_SUCCESS;
}

// 4. 宣告插件導出函式名稱導出巨集
IMGPROC_FILTER_DECLARE_CREATE_FN(custom_filter_create)
IMGPROC_FILTER_DECLARE_DESTROY_FN(custom_filter_destroy)
IMGPROC_FILTER_DECLARE_TRANSFORM_FN(custom_filter_transform)
```

---

## 7. 編譯與測試說明 (Building & Testing)

### 系統需求
* **CMake**: 3.20 或更高版本
* **C/C++ 編譯器**: 支援 C99 與 C++17（GCC 9+ / Clang 10+ / MSVC 2019+）
* **建置工具**: Make / Ninja

### 編譯步驟
```bash
# 1. 建立建置目錄
mkdir -p build && cd build

# 2. 執行 CMake 構建組態
cmake ..

# 3. 編譯所有目標 (包含核心庫、濾鏡插件、CLI 與測試專案)
make -j$(nproc)
```

### 執行單元測試
```bash
# 執行所有單元測試與整合測試
ctest --output-on-failure
```

測試涵蓋以下範疇：
1. `ConfigTest` (`test_imgproc_config`)：測試 TOML 各種型態解析。
2. `FilterLoaderTest` (`test_imgproc_filter_loader`)：測試 `.so` 動態載入與 API 指標掛載。
3. `ImageIOTest` (`test_imgproc_image_io`)：測試 JPEG/PNG 解碼與寫入。
4. `ImageChannelTest` (`test_imgproc_channel`)：測試 R/G/B 色彩通道過濾。
5. `GrayscaleFilterTest` (`test_grayscale_filter`)：灰階濾鏡轉譯正確性驗證。
6. `MirrorFilterTest` (`test_mirror_filter`)：鏡像翻轉演算法驗證。

---

## 8. CLI 應用程式使用手冊 (CLI Guide)

編譯後產生的 CLI 執行檔為 `build/imgproc_app`。

### 語法說明
```text
imgproc_app [options]

選項說明:
  -i, --input <file>     輸入圖像檔案路徑 (JPG/PNG)
  -o, --output <file>    輸出圖像檔案路徑 (JPG/PNG)
  -f, --filter <so_path> 濾鏡動態庫 (.so) [可指定多次進行串接]
  -c, --config <toml>    對應的 TOML 設定檔 [需與 -f 順序對應]
  -ch, --channel <R|G|B> 提取 RGB 單一色彩通道 (R, G, 或 B)
  -h, --help             顯示說明訊息
```

### 使用範例

#### 範例 1：單一濾鏡處理 (灰階化)
```bash
./build/imgproc_app -i inputs/test.jpg -o outputs/grayscale.png -f ./build/libgrayscale_filter.so
```

#### 範例 2：多濾鏡串接處理 (縮放 -> 旋轉 -> 鏡像)
```bash
./build/imgproc_app \
  -i inputs/input.png \
  -o outputs/chained_result.png \
  -f ./build/libresize_filter.so -c tests/test_resize_config.toml \
  -f ./build/librotate_filter.so -c tests/rotate_filter_config.toml \
  -f ./build/libmirror_filter.so -c tests/mirror_filter_config.toml
```

#### 範例 3：提取單一色彩通道 (紅色通道)
```bash
./build/imgproc_app -i inputs/photo.jpg -o outputs/red_only.png -ch R
```

#### 範例 4：自動批次處理 (Auto Batch Mode)
如果不帶任何命令列參數，`imgproc_app` 會自動掃描 `inputs/` 目錄中的所有圖片，並對其進行處理後輸出至 `outputs/`：
```bash
./build/imgproc_app
```
