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

#ifndef GALIB_GALIBNAMESPACEDEF_H
#define GALIB_GALIBNAMESPACEDEF_H

#define GALIB_STD    ::std::
#define GALIB_CSTD   ::
#define GALIB_BOOST  ::boost::
#define GALIB_NBT    ::nbt::
#define GALIB        ::galib::

#if _HAS_NODISCARD
 #define GALIB_NODISCARD [[nodiscard]]
#elif __cplusplus >= 201703L
#define GALIB_NODISCARD [[nodiscard]]
#else
 #define GALIB_NODISCARD
#endif

#if __cplusplus >= 201103L
#define GALIB_NOEXCEPT noexcept
#else
 #define GALIB_NOEXCEPT
#endif

#endif //GALIB_GALIBNAMESPACEDEF_H
