#include "async.hpp"
#include <stdexcept>

const std::exception &get_exception(const std::exception_ptr &e) {
  try {
    std::rethrow_exception(e);
  } catch (const std::exception &e) {
    return e;
  }
  throw std::runtime_error("undefined error");
}

const std::string get_exception_message(const std::exception_ptr &e) {
  try {
    std::rethrow_exception(e);
  } catch (const std::exception &e) {
    return e.what();
  }
  return "undefined error";
}
