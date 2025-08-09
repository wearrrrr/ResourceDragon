#pragma once
#include <string>

namespace UIError {
    struct Error {
      std::string message;
      std::string title;
      bool show = false;
    };

    Error CreateError(const std::string& message, const std::string& title);
};

extern UIError::Error ui_error;
