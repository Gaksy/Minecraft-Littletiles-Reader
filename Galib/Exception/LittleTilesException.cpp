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
 * Date Created: 4/17/2025
 */

#include "Exception/GalibExceptionBasic.h"
#include "Exception/LittleTilesException.h"

constexpr static const char *const STD_ERROR_MESSAGE[] = {
    "LittleTiles Box angle enum arg is invalid.",
    "The NBT tag does not exist."
};

constexpr static const char *const STD_ERROR_CODE[] = {
    "lt_unknow_angle",
    "lt_tage_not_exist"
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