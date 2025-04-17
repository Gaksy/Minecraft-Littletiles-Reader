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


#ifndef GALIB_EXCEPTION_MINECRAFTEXCEPTION_H
#define GALIB_EXCEPTION_MINECRAFTEXCEPTION_H

#include <cstdint>

#include "GalibNamespaceDef.h"
#include "Exception/GalibExceptionBasic.h"

namespace galib::exception {
    using MinecraftErrorCodeType = GALIB_STD uint8_t;

    enum class MinecraftErrorCode: MinecraftErrorCodeType {
        mc_invalid_coord = 1,
        mc_invalid_args = 2,
        mc_chunk_exists = 3,
        mc_decode = 4,
        mc_file_read = 5,
        mc_nbt_empty = 6
    };

    class MinecraftException : public GALIB exception::GalibExceptionBasic<MinecraftErrorCodeType> {
    public:
        explicit MinecraftException(
            const MinecraftErrorCode &kErrorCode,
            const char *kPErrorMessage = nullptr,
            const char *kPErrorSender = nullptr
        );
    };
}


#endif //GALIB_EXCEPTION_MINECRAFTEXCEPTION_H
