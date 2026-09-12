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

// 界面语言。CLI 启动时二选一，库内部所有面向用户的文本都跟着切换。
enum class Language {
  kZhCn,  // 简体中文
  kEnUs,  // English
};

// 全局语言开关（默认中文）。与进度开关一样是全局状态，不是线程安全的；
// 线程化之前应当改成显式传入的 context。
void SetLanguage(Language kLanguage);
[[nodiscard]] Language GetLanguage();
[[nodiscard]] bool IsEnglish();

// 就地二选一的文本：中文与英文写在同一个调用点，避免再维护一份 ID 表。
//
//     printf(Tr("找到 %d 个区块", "found %d chunks"), count);
//
// 两个版本的格式说明符必须完全一致（同一个 printf 会按当前语言取其中一份）。
[[nodiscard]] const char* Tr(const char* kZhCn, const char* kEnUs);

}  // namespace galib

#endif  // GALIB_LOG_GALIBTEXT_H
