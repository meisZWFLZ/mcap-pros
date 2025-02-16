/*
 * MIT License
 *
 * Copyright (c) 2024 Jerry Lum
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// Modified from:
// https://github.com/Jerrylum/hot-cold-asset/blob/9638cca75a8d6ea93b6ddf5c767d2d023e1803cf/hot-cold-asset-repo/include/asset.h

#pragma once

#include "api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __attribute__((__packed__)) _asset {
    uint8_t* buf;
    size_t size;
} asset;

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#define ASSET(x)                                                               \
  extern "C" {                                                                 \
  extern uint8_t _binary_static_##x##_start[], _binary_static_##x##_size[];    \
  static asset x = {_binary_static_##x##_start,                                \
                    (size_t)_binary_static_##x##_size};                        \
  }

#define ASSET_LIB(x)                                                           \
  extern "C" {                                                                 \
  extern uint8_t _binary_static_lib_##x##_start[],                             \
      _binary_static_lib_##x##_size[];                                         \
  static asset x = {_binary_static_lib_##x##_start,                            \
                    (size_t)_binary_static_lib_##x##_size};                    \
  }

#else

#define ASSET(x)                                                               \
  extern uint8_t _binary_static_##x##_start[], _binary_static_##x##_size[];    \
  static asset x = {_binary_static_##x##_start,                                \
                    (size_t)_binary_static_##x##_size};

#define ASSET_LIB(x)                                                           \
  extern uint8_t _binary_static_lib_##x##_start[],                             \
      _binary_static_lib_##x##_size[];                                         \
  static asset x = {_binary_static_lib_##x##_start,                            \
                    (size_t)_binary_static_lib_##x##_size};

#endif
