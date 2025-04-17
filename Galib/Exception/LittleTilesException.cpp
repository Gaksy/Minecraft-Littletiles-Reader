/*
 * This Source Code Form is subject to the terms of the GNU Lesser General Public License,
 * v. 3.0. If a copy of the LGPL was not distributed with this file, You can obtain one at
 * https://www.gnu.org/licenses/lgpl-3.0.html.
 *
 * Copyright (c) 2024 Gaksy (Fuhongren)
 *
 * Author: Gaksy
 * Date Created: 4/17/2025
 *
 * Additional Terms:
 * For commercial use of this software, you must obtain separate authorization from the original author.
 * Please contact Gaksy at gaksys@outlook.com to request commercial use authorization.
 */

#include "Exception/GalibExceptionBasic.h"
#include "Exception/LittleTilesException.h"

constexpr static const char *const STD_ERROR_MESSAGE[] = {
    "LittleTiles Box angle enum arg is invalid.",
    "The NBT tag \"TileEntities\" does not exist."
};

constexpr static const char *const STD_ERROR_CODE[] = {
    "lt_unknow_angle",
    "lt_tile_enities_not_exist"
};

constexpr static const char *const EXCEPTION_NAME = "LittleTiles Exception";

GALIB exception::LittleTilesException::LittleTilesException(
    const LittleTilesErrorCode &kErrorCode,
    const char *kPErrorMessage,
    const char *kPErrorSender
): GalibExceptionBasic<LittleTilesErrorCodeType>(
    static_cast<LittleTilesErrorCodeType>(kErrorCode),
    kPErrorMessage,
    kPErrorSender,
    STD_ERROR_CODE[static_cast<LittleTilesErrorCodeType>(kErrorCode) - 1],
    STD_ERROR_MESSAGE[static_cast<LittleTilesErrorCodeType>(kErrorCode) - 1],
    EXCEPTION_NAME
) { }