/*
 * This Source Code Form is subject to the terms of the GNU Lesser General Public License,
 * v. 3.0. If a copy of the LGPL was not distributed with this file, You can obtain one at
 * https://www.gnu.org/licenses/lgpl-3.0.html.
 *
 * Copyright (c) 2024 Gaksy (Fuhongren)
 *
 * Author: Gaksy
 * Date Created: 12/23/2024
 *
 * Additional Terms:
 * For commercial use of this software, you must obtain separate authorization from the original author.
 * Please contact Gaksy at gaksys@outlook.com to request commercial use authorization.
 */

#ifndef GALIB_EXCEPTION_GALIBEXCEPTIONBASIC_H
#define GALIB_EXCEPTION_GALIBEXCEPTIONBASIC_H

#include <exception>
#include <string>

#include "Galib/GalibNamespaceDef.h"

namespace galib::exception {

 template<typename ArgErrorCodeType>
 class GalibExceptionBasic: public GALIB_STD exception {
 public: // STATIC
  using ErrorCodeType = ArgErrorCodeType;
  using SelfType      = GalibExceptionBasic;

  static GalibExceptionBasic last_exception_;

  static const GalibExceptionBasic& GetLastException() {
   return last_exception_;
  }

  static void SetException(
   const ErrorCodeType kErrorCode,
   const char* const kPErrorMessage = "",
   const char* const kPErrorSender = "",
   const char* const kPExceptionName = "") {

  }

 public: // Construct
  explicit GalibExceptionBasic(
   const ErrorCodeType& kErrorCode = 0,
   const char* const kPErrorMessage = "",
   const char* const kPErrorSender = "",
   const char* const kPErrorCodeMessage = "",
   const char* const kPExceptionName = ""
  ):error_code_(kErrorCode) {
   if(kPErrorMessage) {
    error_message_ = kPErrorMessage;
   }
   if(kPErrorSender) {
    error_sender_ = kPErrorSender;
   }
   if(kPErrorCodeMessage) {
    error_code_message_ = kPErrorCodeMessage;
   }
   if(kPExceptionName) {
    error_exception_name_ = kPExceptionName;
   }

   if(error_exception_name_.empty()) {
    error_exception_name_ = "Galib Base Exception";
   }

   last_exception_ = *this;
  }

  ~GalibExceptionBasic() override = default;

  // GalibExceptionBasic(const GalibExceptionBasic& kRHS):
  //  GALIB_STD exception(kRHS)
  // {
  //
  // }

    // GalibBaseException(const GalibBaseException& RSH):
    //     _STD exception(RSH.error_message_.c_str()),
    //     error_code_(RSH.error_code_),
    //     error_message_(RSH.error_message_)
    // {  }
    //
    // GalibBaseException(GalibBaseException&& RSH) noexcept
    // {
    //     this->error_message_ = _STD move(RSH.error_message_);
    //     this->error_code_ = RSH.error_code_;
    // }
    //
    // GalibBaseException& operator=(GalibBaseException&& RSH) noexcept {
    //     if (this != &RSH) {
    //         this->error_message_ = _STD move(RSH.error_message_);
    //         this->error_code_ = RSH.error_code_;
    //     }
    //     return *this;
    // }
    //
    // GalibBaseException& operator=(const GalibBaseException& RSH) = default;
    //
    // public:
    //
    // _NODISCARD const char *what() const noexcept override {
    //     return error_message_.data();
    // }
    //
  GALIB_NODISCARD ErrorCodeType GetErrorCode() const {
   return error_code_;
  }

  GALIB_NODISCARD const char *GetErrorMessage() const {
   GALIB_STD string error_message(error_exception_name_);
   error_message.append("\n");
   error_message.append(error_code_message_);

   return error_message_.c_str();
  }
    //
 private:
  ErrorCodeType error_code_;
  GALIB_STD string error_message_;
  GALIB_STD string error_sender_;
  GALIB_STD string error_code_message_;
  GALIB_STD string error_exception_name_;
 };

} // exception
// galib

#endif //GALIB_EXCEPTION_GALIBEXCEPTIONBASIC_H
