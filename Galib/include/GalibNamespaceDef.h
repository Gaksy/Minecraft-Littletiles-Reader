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

#ifndef GALIB_GALIBNAMESPACEDEF_H
#define GALIB_GALIBNAMESPACEDEF_H

#define GALIB_STD ::std::
#define GALIB_CSTD ::
#define GALIB_BOOST ::boost::
#define GALIB_NBT ::nbt::
#define GALIB_CGAL ::CGAL::
#define GALIB ::galib::

#define GALIB_DEBUG

#if defined(__GNUC__) || defined(__clang__)
#if __cplusplus >= 201703L
#define GALIB_NODISCARD [[nodiscard]]
#elif __GNUC__ >= 7  // GCC 7.0+ 支持 [[nodiscard]]
#define GALIB_NODISCARD [[nodiscard]]
#else
#define GALIB_NODISCARD
#endif
#endif

#if defined(_MSC_VER)
#if _MSC_VER >= 1928  // Visual Studio 2019, version 16.3+ 支持 [[nodiscard]]
#define GALIB_NODISCARD [[nodiscard]]
#else
#define GALIB_NODISCARD
#endif
#endif

#if __cplusplus >= 201103L
#define GALIB_NOEXCEPT noexcept
#else
#define GALIB_NOEXCEPT
#endif

#endif  //GALIB_GALIBNAMESPACEDEF_H
