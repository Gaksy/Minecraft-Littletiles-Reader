/*
 * This Source Code Form is subject to the terms of the GNU Lesser General Public License,
 * v. 3.0. If a copy of the LGPL was not distributed with this file, You can obtain one at
 * https://www.gnu.org/licenses/lgpl-3.0.html.
 *
 * Copyright (c) 2024 Gaksy (Fuhongren)
 *
 * Author: Gaksy
 * Date Created: 12/22/2024
 *
 * Additional Terms:
 * For commercial use of this software, you must obtain separate authorization from the original author.
 * Please contact Gaksy at gaksys@outlook.com to request commercial use authorization.
 */

#include "Galib/Exception/GalibExceptionBasic.h"
#include "Galib/Exception/MinecraftException.h"

constexpr static const char* const STD_ERROR_MESSAGE[] = {
 "Minecraft coordinates is invalid.",
 "The args is invalid.",
 "The chunk exists.",
 "Decoding process is failed."
};

constexpr static const char* const STD_ERROR_CODE[] = {
 "mc_invalid_coord",
 "mc_invalid_args",
 "mc_chunk_exites",
 "mc_decode"
};

constexpr static const char* const EXCEPTION_NAME = "Minecraft Exception";

GALIB exception::MinecraftException::MinecraftException(
 const MinecraftErrorCode& kErrorCode,
 const char* const kPErrorMessage,
 const char* const kPErrorSender
): GalibExceptionBasic<MinecraftErrorCodeType>(
  static_cast<MinecraftErrorCodeType>(kErrorCode),
  kPErrorMessage,
  kPErrorSender,
  STD_ERROR_CODE[static_cast<MinecraftErrorCodeType>(kErrorCode) - 1],
  STD_ERROR_MESSAGE[static_cast<MinecraftErrorCodeType>(kErrorCode) - 1],
  EXCEPTION_NAME
 ) { }