#pragma once

#include <exception>
#include <string>

const std::exception &get_exception(const std::exception_ptr &e);
const std::string get_exception_message(const std::exception_ptr &e);
