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
#ifndef GALIB_INCLUDE_EXCEPTION_LITTLETILESEXCEPTION_H
#define GALIB_INCLUDE_EXCEPTION_LITTLETILESEXCEPTION_H

#include <cstdint>

#include "GalibNamespaceDef.h"
#include "Exception/GalibExceptionBasic.h"

namespace galib::exception {
    using LittleTilesErrorCodeType = GALIB_STD uint8_t;

    enum class LittleTilesErrorCode: LittleTilesErrorCodeType{
        lt_unknow_angle = 0,
        lt_tage_not_exist = 1,
        lt_unknow_face = 2
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
