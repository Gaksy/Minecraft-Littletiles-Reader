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

#ifndef GALIB_LOG_GALIBTEXT_H
#define GALIB_LOG_GALIBTEXT_H

#include "GalibNamespaceDef.h"

namespace galib {

// 输出统一为英文，避免终端编码导致中文乱码。
// 历史上 Tr(zh, en) 支持中英双语；现在固定返回英文，中文参数被忽略。
// 保留同样签名，调用点无需改动。
[[nodiscard]] const char* Tr(const char* kZhCn, const char* kEnUs);

}  // namespace galib

#endif  // GALIB_LOG_GALIBTEXT_H