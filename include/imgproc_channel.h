#pragma once

#include "imgproc_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Represents the color channels in an RGB image.
 */
typedef enum {
    IMGPROC_CHANNEL_R = 0,
    IMGPROC_CHANNEL_G = 1,
    IMGPROC_CHANNEL_B = 2
} ImgProcChannel;

/**
 * @brief Keep only the specified channel in the image and zero out the others.
 *
 * @param [in,out] image Pointer to the ImgProcImage to modify.
 * @param [in] keep_ch The channel to keep (R, G, or B).
 */
void imgproc_image_keep_channel(ImgProcImage* image, ImgProcChannel keep_ch);

#ifdef __cplusplus
}
#endif
