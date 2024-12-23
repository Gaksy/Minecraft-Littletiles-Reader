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



#ifndef GALIB_EXCEPTION_MINECRAFTEXCEPTION_H
#define GALIB_EXCEPTION_MINECRAFTEXCEPTION_H

#include <cstdint>

#include "Galib/GalibNamespaceDef.h"
#include "Galib/Exception/GalibExceptionBasic.h"

namespace galib::exception {
 using MinecraftErrorCodeType = GALIB_STD uint8_t;

 enum class MinecraftErrorCode: MinecraftErrorCodeType {
  mc_invalid_coord = 1,
  mc_invalid_args  = 2,
  mc_chunk_exists  = 3,
  mc_decode        = 4
 };

 class MinecraftException: public GALIB exception::GalibExceptionBasic<MinecraftErrorCodeType> {
 public:
  explicit MinecraftException(
   const MinecraftErrorCode& kErrorCode,
   const char* kPErrorMessage = nullptr,
   const char* kPErrorSender = nullptr
  );
 };
}


#endif //GALIB_EXCEPTION_MINECRAFTEXCEPTION_H
