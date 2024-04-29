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

class APIError : public std::runtime_error {
public:
  int status_code;

  APIError(int status_code)
      : std::runtime_error(format_message(status_code)),
        status_code(status_code) {}

private:
  static auto format_message(int status_code) -> const std::string {
    std::stringstream ss;
    ss << "API returned unexpected status code " << status_code;
    return ss.str();
  };
};

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

  Quantity<double> temperature;
  Quantity<double> humidity;
  Quantity<double> precipitation;
  bool is_day;

  std::vector<HourForecast> hourly_forecast;
  std::vector<DayForecast> daily_forecast;
};

struct Location {
  std::string name;
  std::string timezone;
  std::pair<double, double> coordinates;
};

class API {
public:
  static void fetch_weather(const Location location,
                            AsyncCallback<Conditions> callback) {
    cpr::GetCallback(
        [callback](cpr::Response resp) {
          try {
            if (resp.status_code != 200) {
              throw Weather::APIError(resp.status_code);
            }

            const auto object = nlohmann::json::parse(resp.text);

            const auto current = object["current"];
            Weather::Conditions condition{
                .temperature =
                    Quantity(current["temperature_2m"].get<double>(), "°F"),
                .humidity = Quantity(
                    current["relative_humidity_2m"].get<double>(), "%"),
                .precipitation =
                    Quantity(current["precipitation"].get<double>(), "in"),
                .is_day = current["is_day"].get<int>() != 0,
            };

            const auto now = std::chrono::system_clock::now();

            const auto hourly = object["hourly"];
            const auto hourly_count = hourly.size();
            condition.hourly_forecast.reserve(hourly_count);
            for (size_t i = 0; i < hourly_count; i++) {
              auto future =
                  std::chrono::seconds(hourly["time"][i].get<int64_t>());
              auto d = future - now.time_since_epoch();
              condition.hourly_forecast.push_back({
                  .since_now =
                      std::chrono::duration_cast<std::chrono::hours>(d),
                  .temperature =
                      Quantity(hourly["temperature_2m"][i].get<double>(), "°F"),
                  .humidity = Quantity(
                      hourly["relative_humidity_2m"][i].get<double>(), "%"),
                  .precipitation =
                      Quantity(hourly["precipitation"][i].get<double>(), "in"),
              });
            }

            const auto daily = object["daily"];
            const auto daily_count = daily.size();
            condition.daily_forecast.reserve(daily_count);
            for (size_t i = 0; i < daily_count; i++) {
              auto d = std::chrono::seconds(daily["time"][i].get<int64_t>());
              auto t = std::chrono::time_point<std::chrono::local_t,
                                               std::chrono::seconds>(d);
              condition.daily_forecast.push_back({
                  .date = std::chrono::year_month_day(
                      std::chrono::time_point_cast<std::chrono::days>(t)),
                  .temperature_max = Quantity(
                      daily["temperature_2m_max"][i].get<double>(), "°F"),
                  .temperature_min = Quantity(
                      daily["temperature_2m_min"][i].get<double>(), "°F"),
                  .precipitation = Quantity(
                      daily["precipitation_sum"][i].get<double>(), "in"),
              });
            }

            callback(condition);
          } catch (...) {
            callback(std::current_exception());
          }
        },
        cpr::Url("https://api.open-meteo.com/v1/forecast"),
        cpr::Parameters{
            {"latitude", std::to_string(location.coordinates.first)},
            {"longitude", std::to_string(location.coordinates.second)},
            {"timezone", location.timezone},
            {"current",
             "temperature_2m,relative_humidity_2m,is_day,precipitation,rain"},
            {"hourly", "temperature_2m,relative_humidity_2m,precipitation"},
            {"daily",
             "temperature_2m_max,temperature_2m_min,precipitation_sum"},
            {"temperature_unit", "fahrenheit"},
            {"wind_speed_unit", "mph"},
            {"precipitation_unit", "inch"},
            {"timeformat", "unixtime"}});
  };

  static void search_locations(std::string query,
                               AsyncCallback<std::vector<Location>> callback) {
    cpr::GetCallback(
        [callback](cpr::Response resp) {
          try {
            if (resp.status_code != 200) {
              throw Weather::APIError(resp.status_code);
            }

            const auto object = nlohmann::json::parse(resp.text);
            const auto results = object["results"];

            std::vector<Location> locations;
            locations.reserve(results.size());

            for (const auto &result : results) {
              std::stringstream location_name;
              location_name << result["name"];
              if (result.contains("admin1")) {
                location_name << ", " << result["admin1"].get<std::string>();
              }
              if (result.contains("admin2")) {
                location_name << ", " << result["admin2"].get<std::string>();
              }
              location_name << ", "
                            << result["country_code"].get<std::string>();

              locations.emplace_back(
                  location_name.str(), result["timezone"].get<std::string>(),
                  std::make_pair(result["latitude"].get<double>(),
                                 result["longitude"].get<double>()));
            }

            callback(locations);
          } catch (...) {
            callback(std::current_exception());
          }
        },
        cpr::Url("https://geocoding-api.open-meteo.com/v1/search"),
        cpr::Parameters{{"name", query}});
  };

  static void
  search_location(std::string query,
                  std::function<void(std::optional<Location>)> callback) {
    search_locations(query,
                     [&callback](AsyncResult<std::vector<Location>> result) {
                       result.maybe_throw();

                       auto locations = result.value();
                       if (locations.empty()) {
                         callback(std::nullopt);
                       } else {
                         callback(locations[0]);
                       }
                     });
  }
};

// This class fetches weather conditions from the API and caches them.
// It does so every time invalidate() is called.
// This is useful to avoid fetching the same data multiple times.
struct ConditionsFetcher : std::optional<Conditions> {
public:
  Location location;
  const std::function<void()> on_update;

  ConditionsFetcher(Location location, const std::function<void()> on_update)
      : location(location), on_update(on_update) {}

  // Fetches the weather conditions from the API in the background.
  // The function immediately returns to not block the UI thread, and on_update
  // is called when the data is fetched.
  auto update() -> void {
    if (is_fetching) {
      return;
    }

    is_fetching = true;
    this->reset();

    API::fetch_weather(location,
                       [&](AsyncResult<Weather::Conditions> condition) {
                         if (condition.has_value()) {
                           this->emplace(condition.value());
                           error = nullptr;
                         } else {
                           error = condition.error();
                         }
                         is_fetching = false;
                         on_update();
                       });
  };

  bool has_error() const { return error != nullptr; }
  const std::exception &get_error() const { return get_exception(error); }

private:
  bool is_fetching = false;
  std::exception_ptr error;
};

} // namespace Weather
