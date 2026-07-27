cmake_minimum_required(VERSION 3.20)
project(ImgProc VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)

    add_compile_options(-fsanitize=address -g)
    add_link_options(-fsanitize=address)

endif()

include(FetchContent)
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)

add_library(imgproc SHARED
    src/imgproc_config.cpp
    src/imgproc_logger.cpp
    src/imgproc_utils.cpp
    src/imgproc_filter_loader.cpp
    src/imgproc_image_io.cpp
    src/imgproc_channel.cpp)
target_include_directories(imgproc
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(imgproc
    PRIVATE
        tomlplusplus::tomlplusplus
        ${CMAKE_DL_LIBS})


add_library(dummy_filter SHARED
    src/dummy_filter.cpp)
target_include_directories(dummy_filter
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(dummy_filter
    PRIVATE
        imgproc)

add_library(resize_filter SHARED
    src/resize_filter.cpp)
target_include_directories(resize_filter
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(resize_filter
    PRIVATE
        imgproc)

add_executable(imgproc_app main.cpp)
target_link_libraries(imgproc_app PRIVATE imgproc)
add_dependencies(imgproc_app resize_filter dummy_filter grayscale_filter)

add_library(grayscale_filter SHARED
    src/grayscale_filter.cpp)
target_include_directories(grayscale_filter
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(grayscale_filter
    PRIVATE
        imgproc)


enable_testing()

add_executable(test_imgproc_config
    tests/test_imgproc_config.c
)
target_link_libraries(test_imgproc_config PRIVATE imgproc)
add_custom_command(
    TARGET test_imgproc_config POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/tests/test_config.toml
    ${CMAKE_CURRENT_BINARY_DIR}/test_config.toml)
add_test(NAME ConfigTest COMMAND test_imgproc_config)

add_executable(test_imgproc_filter_loader
    tests/test_imgproc_filter_loader.c
)
target_link_libraries(test_imgproc_filter_loader PRIVATE imgproc)
add_custom_command(
    TARGET test_imgproc_filter_loader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/tests/dummy_filter_config.toml
    ${CMAKE_CURRENT_BINARY_DIR}/dummy_filter_config.toml)
add_test(NAME FilterLoaderTest COMMAND test_imgproc_filter_loader)

# Image IO Unit Test
add_executable(test_imgproc_image_io tests/test_imgproc_image_io.c)
target_link_libraries(test_imgproc_image_io PRIVATE imgproc)
add_custom_command(
    TARGET test_imgproc_image_io POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/inputs
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/outputs)
add_test(NAME ImageIOTest COMMAND test_imgproc_image_io)

# Image Channel Filter Unit Test
add_executable(test_imgproc_channel tests/test_imgproc_channel.c)
target_link_libraries(test_imgproc_channel PRIVATE imgproc)
add_test(NAME ImageChannelTest COMMAND test_imgproc_channel)

# Grayscale Filter Unit Test
add_executable(test_grayscale_filter tests/test_grayscale_filter.c)
target_link_libraries(test_grayscale_filter PRIVATE imgproc ${CMAKE_DL_LIBS})
add_dependencies(test_grayscale_filter grayscale_filter)
add_test(NAME GrayscaleFilterTest COMMAND test_grayscale_filter)

