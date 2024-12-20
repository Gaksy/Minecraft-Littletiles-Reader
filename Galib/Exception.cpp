#include "Exception.h"

using galib::Exception::GalibErrorCode;
using galib::Exception::GalibException;

GalibException GalibException::last_exception_;

std::string GetStdErrorMessage(const GalibErrorCode& kErrCode) {
    static const std::string STD_ERROR_MESSAGE[] = {
        "No errors.\n",
        "The function argument is incorrect or invalid.\n",
        "Failed to open file.\n",
        "Failed to retrieve file.\n",
        "Failed to open folder.\n",
        "Memory allocation failed.\n",
        "Failed to retrieve MCA file map chunk coordinates.\n",
        "Object is uninitialized.\n",
        "Can't decode the desc data.\n",
        "The Chunk exists.\n"
    };

    return std::string("Galib error\t: ") + STD_ERROR_MESSAGE[kErrCode];
}

GalibException::GalibException(const GalibErrorCode& kErrorCode, const char* const kPErrorMessage, const char* const kPErrorSender):
    error_code_(kErrorCode),
    error_message_(GetStdErrorMessage(error_code_))
{
    if (kPErrorMessage) {
        error_message_ += "Error message\t: ";
        error_message_ += kPErrorMessage;
        error_message_ += ".\n";
    }

    if (kPErrorSender) {
        error_message_ += "Error function\t: ";
        error_message_ += kPErrorSender;
        error_message_ += "\n";
    }

    GalibException::last_exception_ = *this;
}

const char* GalibException::what()const noexcept {
    return error_message_.c_str();
}

const GalibErrorCode& GalibException::GetErrorCode()const {
    return error_code_;
}

const std::string& GalibException::GetErrorMessage()const {
    return error_message_;
}

const GalibException& galib::Exception::GalibException::GetLastException()
{
    return GalibException::last_exception_;
}

void GalibException::SetException(const GalibErrorCode& kErrorCode, const char* const kPErrorMessage, const char* const kPErrorSender) {
    GalibException temp(kErrorCode, kPErrorMessage, kPErrorSender);
}