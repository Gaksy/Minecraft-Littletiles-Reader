#ifndef _GALIB_EXCEPTION_H_
#define _GALIB_EXCEPTION_H_

#include <exception>
#include <string>

namespace galib {
    namespace Exception {
        enum GalibErrorCode {
            galib_no_error,
            galib_invalid_argument,
            galib_failed_open_file,
            galib_failed_retrieve_file,
            galib_failed_open_folder,
            galib_bad_alloc,
            minecraft_invalid_coord,
            minecraft_object_uninitialized,
            minecraft_decode_error,
            minecraft_chunk_exists
        };

        class GalibException : std::exception {
        public:
            GalibException(const GalibErrorCode& kErrorCode = GalibErrorCode::galib_no_error, const char* const kPErrorMessage = "", const char* const kPErrorSender = "");
            ~GalibException()=default;
            GalibException& operator=(const GalibException& RHS) = default;

            const char* what() const noexcept override;

            const GalibErrorCode& GetErrorCode()const;
            const std::string& GetErrorMessage()const;

        public:
            static const GalibException& GetLastException();
            static void SetException(const GalibErrorCode& kErrorCode = GalibErrorCode::galib_no_error, const char* const kPErrorMessage = "", const char* const kPErrorSender = "");

        private:
            GalibErrorCode  error_code_;
            std::string     error_message_;

            // STATIC
            static GalibException last_exception_;
        };
    }
}

#endif // !_GALIB_EXCEPTION_H_
