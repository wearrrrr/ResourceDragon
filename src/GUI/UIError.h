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

inline UIError::Error ui_error = {
  .message = "",
  .title = "",
  .show = false
};
