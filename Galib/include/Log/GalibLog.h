/*
 * Copyright (c) 2024 Gaksy (Fuhongren)
 *
 * This work is licensed under the GNU Lesser General Public License v3.0.
 * You may obtain a copy of the license at https://www.gnu.org/licenses/lgpl-3.0.html.
 *
 * This source code form is subject to the terms of the LGPL v3.0 license.
 * If a copy of the LGPL was not distributed with this file, you can obtain one
 * at the above license URL.
 */

/*
 * Author: Gaksy
 * Date Created: 09/12/2026
 */

#ifndef GALIB_LOG_GALIBLOG_H
#define GALIB_LOG_GALIBLOG_H

#include "GalibNamespaceDef.h"

#if defined(__clang__) || defined(__GNUC__)
#define GALIB_PRINTF_LIKE(kFormatIndex, kFirstArgumentIndex) \
  __attribute__((format(printf, kFormatIndex, kFirstArgumentIndex)))
#else
#define GALIB_PRINTF_LIKE(kFormatIndex, kFirstArgumentIndex)
#endif

namespace galib {

// 进度输出的运行时开关。
//
// 这个库会逐区块、逐方块地打印进度（解析到哪个区块、构建到第几个方块……），
// 本地 CLI 看着还行，但作为服务器端核心库时必须能整体关掉——既污染日志，
// 也会在高并发下把 stdout 拖成瓶颈。CLI 的"显示进度提示"选项即控制它。
//
// 注意：这是全局状态，和库里其它全局状态一样不是线程安全的；
// 线程化之前应当改成显式传递的 logger 参数。
void SetProgressEnabled(bool kEnabled);

[[nodiscard]] bool IsProgressEnabled();

// 进度输出：开关关闭时什么都不打印。
// 只用于进度/调试信息；错误与警告仍直接走 std::printf / std::cerr。
void ProgressPrintf(const char* kFormat, ...) GALIB_PRINTF_LIKE(1, 2);

}  // namespace galib

#endif  // GALIB_LOG_GALIBLOG_H
