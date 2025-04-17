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
#ifndef GALIB_INCLUDE_EXCEPTION_LITTLETILESEXCEPTION_H
#define GALIB_INCLUDE_EXCEPTION_LITTLETILESEXCEPTION_H

#include <cstdint>

#include "GalibNamespaceDef.h"
#include "Exception/GalibExceptionBasic.h"

namespace galib::exception {
    using LittleTilesErrorCodeType = GALIB_STD uint8_t;

    enum class LittleTilesErrorCode: LittleTilesErrorCodeType{
        lt_unknow_angle = 0,
        lt_tile_enities_not_exist = 1
    };

    class LittleTilesException : public GALIB exception::GalibExceptionBasic<LittleTilesErrorCodeType>{
    public:
        explicit LittleTilesException(
            const LittleTilesErrorCode &kErrorCode,
            const char *kPErrorMessage = nullptr,
            const char *kPErrorSender = nullptr
        );
    };

}

#endif //GALIB_INCLUDE_EXCEPTION_LITTLETILESEXCEPTION_H
