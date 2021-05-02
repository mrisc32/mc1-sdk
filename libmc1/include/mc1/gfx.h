// -*- mode: c; tab-width: 2; indent-tabs-mode: nil; -*-
//--------------------------------------------------------------------------------------------------
// Copyright (c) 2021 Marcus Geelnard
//
// This software is provided 'as-is', without any express or implied warranty. In no event will the
// authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose, including commercial
// applications, and to alter it and redistribute it freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not claim that you wrote
//     the original software. If you use this software in a product, an acknowledgment in the
//     product documentation would be appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
//     being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//--------------------------------------------------------------------------------------------------

#ifndef MC1_GFX_H_
#define MC1_GFX_H_

#include <mc1/framebuffer.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Clear framebuffer.
/// @param color The fill color.
void gfx_clear(fb_t* fb, uint32_t color);

/// @brief Fill a rectangle.
/// @param x0 Rectangle origin x coordinate.
/// @param y0 Rectangle origin y coordinate.
/// @param w Rectangle width.
/// @param h Rectangle height.
/// @param color The fill color.
void gfx_fill_rect(fb_t* fb, int x0, int y0, int w, int h, uint32_t color);

/// @brief Set the color of a single pixel.
/// @param x Pixel x coordinate.
/// @param y Pixel y coordinate.
/// @param color The fill color.
void gfx_draw_point(fb_t* fb, int x, int y, uint32_t color);

/// @brief Draw a line.
/// @param x0 Line start x coordinate.
/// @param y0 Line start y coordinate.
/// @param x1 Line stop x coordinate.
/// @param y1 Line stop y coordinate.
/// @param color The line color.
void gfx_draw_line(fb_t* fb, int x0, int y0, int x1, int y1, uint32_t color);

#ifdef __cplusplus
}
#endif

#endif  // MC1_GFX_H_

