#pragma once

#include "async.hpp"
#include <chrono>
#include <cpr/api.h>
#include <cpr/cpr.h>
#include <cpr/cprtypes.h>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>

namespace Weather {

struct Location {
  std::string name;
  std::string timezone;
  std::pair<double, double> coordinates;
};

// A quantity with a value and a unit.
template <typename T = double> class Quantity {
public:
  Quantity(T value, std::string unit) : value(value), unit(unit) {}
  auto as_value() const -> double { return value; }
  auto as_string() const -> const std::string {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << value << unit;
    return ss.str();
  }

private:
  T value;
  std::string unit;
};

struct Conditions {
  struct HourForecast {
    std::chrono::hours since_now;
    Quantity<double> temperature;
    Quantity<double> humidity;
    Quantity<double> precipitation;
  };

  struct DayForecast {
    std::chrono::year_month_day date;
    Quantity<double> temperature_max;
    Quantity<double> temperature_min;
    Quantity<double> precipitation;
  };

  const Location &location;
  Quantity<double> temperature;
  Quantity<double> humidity;
  Quantity<double> precipitation;
  bool is_day;

  std::vector<HourForecast> hourly_forecast;
  std::vector<DayForecast> daily_forecast;
};

namespace API {
// Fetches the weather conditions for a given location.
auto fetch_weather(const Location &location) -> std::future<Conditions>;

// Searches for locations that match the query.
auto search_locations(std::string query) -> std::future<std::vector<Location>>;

// Searches for a single location that matches the query.
auto search_location(std::string query) -> std::future<std::optional<Location>>;

// Exception thrown when the API returns an unexpected status code.
class Error : public std::runtime_error {
public:
  int status_code;

  Error(int status_code)
      : std::runtime_error(format_message(status_code)),
        status_code(status_code) {}

private:
  static auto format_message(int status_code) -> const std::string {
    std::stringstream ss;
    ss << "API returned unexpected status code " << status_code;
    return ss.str();
  };
};
}; // namespace API

} // namespace Weather
