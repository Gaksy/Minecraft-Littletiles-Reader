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
   const char* const kPErrorMessage       = "",
   const char* const kPErrorSender        = "",
   const char* const kPErrorCodeName      = "",
   const char* const kPErrorCodeInfo      = "",
   const char* const kPErrorExceptionName = ""
   ) {
   GalibExceptionBasic(
    kErrorCode, kPErrorMessage, kPErrorSender, kPErrorCodeName, kPErrorCodeInfo, kPErrorExceptionName
   );
  }

 public: // Construct
  explicit GalibExceptionBasic(
   const ErrorCodeType kErrorCode = 0,
   const char* const kPErrorMessage  = nullptr,
   const char* const kPErrorSender   = nullptr,
   const char* const kPErrorCodeName = nullptr,
   const char* const kPErrorCodeInfo = nullptr,
   const char* const kPExceptionName = nullptr
  ): error_code_(kErrorCode) {
   if(kErrorCode == 0) {
    error_message_ = "No exception.";
    error_code_name_ = "no_exsceptions";
    error_code_info_ = "No exceptions.";
   }
   else {
    if(kPErrorMessage) {
     error_message_ = kPErrorMessage;
    }
    if(kPErrorSender) {
     error_sender_ = kPErrorSender;
    }
    if(kPErrorCodeName) {
     error_code_name_ = kPErrorCodeName;
    }
    if(kPErrorCodeInfo) {
     error_code_info_ = kPErrorCodeInfo;
    }
    if(kPExceptionName) {
     exception_name_ = kPExceptionName;
    }
    else {
     exception_name_ = "Galib Exception Basic";
    }
   }

   // Error Title
   desc_error_message_.append(exception_name_ + "\nMessage: \t");

   // Error Msg
   if(error_message_.empty()) {
    desc_error_message_.append("Unknow Error, the error message is empty.\n");
   }
   else {
    desc_error_message_.append(error_message_ + "\n");
   }

   // Other info
   if(!error_sender_.empty()) {
    desc_error_message_.append("Sender: \t");
    desc_error_message_.append(error_sender_ + "\n");
   }

   if(!error_code_name_.empty()) {
    desc_error_message_.append("Error info: \t");
    desc_error_message_.append(error_code_name_ + "(");
    desc_error_message_.append(GALIB_STD to_string(error_code_));
    desc_error_message_.append(") ");
    if(!error_code_info_.empty()) {
     desc_error_message_.append(" - ");
     desc_error_message_.append(error_code_info_);
    }
   }

   last_exception_ = *this;
  }

  GALIB_NODISCARD ErrorCodeType GetErrorCode() const {
   return error_code_;
  }

  GALIB_NODISCARD const char * GetErrorMessage() const GALIB_NOEXCEPT {
   return desc_error_message_.c_str();
  }

  GALIB_NODISCARD const char * what() const GALIB_NOEXCEPT override {
   return desc_error_message_.c_str();
  }

 private:
  ErrorCodeType error_code_;
  GALIB_STD string desc_error_message_;

  GALIB_STD string error_message_;
  GALIB_STD string error_sender_;
  GALIB_STD string error_code_name_;
  GALIB_STD string error_code_info_;
  GALIB_STD string exception_name_;
 };

 template<typename ArgErrorCodeType>
 GalibExceptionBasic<ArgErrorCodeType> GalibExceptionBasic<ArgErrorCodeType>::last_exception_ = GalibExceptionBasic();

}

#endif //GALIB_EXCEPTION_GALIBEXCEPTIONBASIC_H
