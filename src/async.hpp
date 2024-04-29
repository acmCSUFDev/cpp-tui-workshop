#pragma once

#include <exception>
#include <functional>
#include <stdexcept>
#include <variant>

template <typename T> class AsyncResult : std::variant<T, std::exception_ptr> {
public:
  AsyncResult(const T &value) : std::variant<T, std::exception_ptr>(value) {}
  AsyncResult(const std::exception_ptr &error)
      : std::variant<T, std::exception_ptr>(error) {}

  bool has_value() const { return this->index() == 0; }
  bool has_error() const { return this->index() == 1; }

  const T &value() const { return std::get<0>(*this); }
  const std::exception_ptr &error() const { return std::get<1>(*this); }

  void maybe_throw() const {
    if (has_error()) {
      std::rethrow_exception(error());
    }
  }
};

template <typename T> using AsyncCallback = std::function<void(AsyncResult<T>)>;

const std::exception &get_exception(const std::exception_ptr &e) {
  try {
    std::rethrow_exception(e);
  } catch (const std::exception &e) {
    return e;
  }
  throw std::runtime_error("undefined error");
}
