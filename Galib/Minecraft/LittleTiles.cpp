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
 * Date Created: 04/29/2025
 */

#include "Minecraft/LittleTiles.h"

#include <cinttypes>

#include "GalibNamespaceDef.h"

using GALIB minecraft::littletiles::AngleID;
using GALIB_STD uint8_t;

uint8_t(GALIB minecraft::littletiles::ConvertAngleIdToInt)(
    const AngleID kAngleId) {
  return static_cast<uint8_t>(kAngleId);
}

AngleID(galib::minecraft::littletiles::ConvertIntToAngleId)(
    const uint8_t kNumId) {
  return static_cast<AngleID>(kNumId);
}
