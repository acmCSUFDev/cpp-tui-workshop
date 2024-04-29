#pragma once

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

struct Conditions {
  double temperature_fahrenheit;
  double humidity_percentage;
  double precipitation_inch;
  bool is_day;

  auto temperature_string() const -> const std::string {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << temperature_fahrenheit << "°F";
    return ss.str();
  }

  auto humidity_string() const -> const std::string {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << humidity_percentage << "%";
    return ss.str();
  }
};

struct Location {
  std::string name;
  double latitude;
  double longitude;

  Location(std::string name, double lat, double lon)
      : name(name), latitude(lat), longitude(lon) {}
};

class API {
public:
  static void fetch_weather_async(const Location location,
                                  std::function<void(Conditions)> callback) {

    std::stringstream url;
    url << "https://api.open-meteo.com/v1/forecast"
        << "?latitude=" << location.latitude
        << "&longitude=" << location.longitude
        << "&current=temperature_2m,relative_humidity_2m,is_day,precipitation,"
           "rain"
        << "&temperature_unit=fahrenheit"
        << "&wind_speed_unit=mph"
        << "&precipitation_unit=inch"
        << "&timeformat=unixtime";

    cpr::GetCallback(
        [callback](cpr::Response resp) {
          if (resp.status_code != 200) {
            throw Weather::APIError(resp.status_code);
          }

          const auto object = nlohmann::json::parse(resp.text);
          const auto current = object["current"];

          Weather::Conditions condition{
              .temperature_fahrenheit = current["temperature_2m"].get<double>(),
              .humidity_percentage =
                  current["relative_humidity_2m"].get<double>(),
              .precipitation_inch = current["precipitation"].get<double>(),
              .is_day = current["is_day"].get<int>() != 0,
          };
          callback(condition);
        },
        cpr::Url(url.str()));
  };

  static void
  search_locations(std::string query,
                   std::function<void(std::vector<Location>)> callback) {
    std::stringstream url;
    url << "https://geocoding-api.open-meteo.com/v1/search"
        << "?name=" << query;

    cpr::GetCallback(
        [callback](cpr::Response resp) {
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
            location_name << ", " << result["country_code"].get<std::string>();

            locations.emplace_back(location_name.str(),
                                   result["latitude"].get<double>(),
                                   result["longitude"].get<double>());
          }

          callback(locations);
        },
        cpr::Url(url.str()));
  };

  static void
  search_location(std::string query,
                  std::function<void(std::optional<Location>)> callback) {
    search_locations(query, [&callback](std::vector<Location> locations) {
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

    API::fetch_weather_async(location, [&](Weather::Conditions condition) {
      is_fetching = false;
      this->emplace(condition);
      on_update();
    });
  };

private:
  bool is_fetching = false;
};

} // namespace Weather
