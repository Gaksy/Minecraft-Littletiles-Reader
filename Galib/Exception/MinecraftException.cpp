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
 * Date Created: 12/22/2024
 */

#include "Exception/GalibExceptionBasic.h"
#include "Exception/MinecraftException.h"

constexpr static const char *const STD_ERROR_MESSAGE[] = {
    "Minecraft coordinates is invalid.",
    "The args is invalid.",
    "The chunk exists.",
    "Decoding process is failed.",
    "Unable to access file.",
    "Attempting to access an empty NBT tag."
};

constexpr static const char *const STD_ERROR_CODE[] = {
    "mc_invalid_coord",
    "mc_invalid_args",
    "mc_chunk_exites",
    "mc_decode",
    "mc_file_read",
    "mc_nbt_empty"
};

constexpr static const char *const EXCEPTION_NAME = "Minecraft Exception";

GALIB exception::MinecraftException::MinecraftException(
    const MinecraftErrorCode &kErrorCode,
    const char *const kPErrorMessage,
    const char *const kPErrorSender
): GalibExceptionBasic<MinecraftErrorCodeType>(
    static_cast<MinecraftErrorCodeType>(kErrorCode),
    kPErrorMessage,
    kPErrorSender,
    STD_ERROR_CODE[static_cast<MinecraftErrorCodeType>(kErrorCode) - 1],
    STD_ERROR_MESSAGE[static_cast<MinecraftErrorCodeType>(kErrorCode) - 1],
    EXCEPTION_NAME
) { }
