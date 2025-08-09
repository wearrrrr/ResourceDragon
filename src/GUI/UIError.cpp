#include <UIError.h>

UIError::Error ui_error = {
  .message = "",
  .title = "",
  .show = false
};

UIError::Error UIError::CreateError(const std::string &message, const std::string &title) {
    UIError::Error error;
    error.message = message;
    error.title = title;
    error.show = true;
    return error;
}
