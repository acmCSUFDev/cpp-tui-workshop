#include "weather.hpp"
#include <future>
#include <optional>

namespace Weather {

auto API::fetch_weather(const Location &location) -> std::future<Conditions> {
  return std::async(std::launch::async, [&] {
    const auto resp = cpr::Get(
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
    if (resp.status_code != 200) {
      throw Weather::API::Error(resp.status_code);
    }

    const auto object = nlohmann::json::parse(resp.text);

    const auto current = object["current"];
    Weather::Conditions condition{
        .location = location,
        .temperature = Quantity(current["temperature_2m"].get<double>(), "°F"),
        .humidity =
            Quantity(current["relative_humidity_2m"].get<double>(), "%"),
        .precipitation = Quantity(current["precipitation"].get<double>(), "in"),
        .is_day = current["is_day"].get<int>() != 0,
    };

    const auto now = std::chrono::system_clock::now();

    const auto hourly = object["hourly"];
    const auto hourly_count = hourly.size();
    condition.hourly_forecast.reserve(hourly_count);
    for (size_t i = 0; i < hourly_count; i++) {
      auto future = std::chrono::seconds(hourly["time"][i].get<int64_t>());
      auto d = future - now.time_since_epoch();
      condition.hourly_forecast.push_back({
          .since_now = std::chrono::duration_cast<std::chrono::hours>(d),
          .temperature =
              Quantity(hourly["temperature_2m"][i].get<double>(), "°F"),
          .humidity =
              Quantity(hourly["relative_humidity_2m"][i].get<double>(), "%"),
          .precipitation =
              Quantity(hourly["precipitation"][i].get<double>(), "in"),
      });
    }

    const auto daily = object["daily"];
    const auto daily_count = daily.size();
    condition.daily_forecast.reserve(daily_count);
    for (size_t i = 0; i < daily_count; i++) {
      auto d = std::chrono::seconds(daily["time"][i].get<int64_t>());
      auto t =
          std::chrono::time_point<std::chrono::local_t, std::chrono::seconds>(
              d);
      condition.daily_forecast.push_back({
          .date = std::chrono::year_month_day(
              std::chrono::time_point_cast<std::chrono::days>(t)),
          .temperature_max =
              Quantity(daily["temperature_2m_max"][i].get<double>(), "°F"),
          .temperature_min =
              Quantity(daily["temperature_2m_min"][i].get<double>(), "°F"),
          .precipitation =
              Quantity(daily["precipitation_sum"][i].get<double>(), "in"),
      });
    }

    return condition;
  });
};

auto API::search_locations(std::string query)
    -> std::future<std::vector<Location>> {
  return std::async(std::launch::async, [&] {
    const auto resp =
        cpr::Get(cpr::Url("https://geocoding-api.open-meteo.com/v1/search"),
                 cpr::Parameters{{"name", query}});
    if (resp.status_code != 200) {
      throw Weather::API::Error(resp.status_code);
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
                             result["timezone"].get<std::string>(),
                             std::make_pair(result["latitude"].get<double>(),
                                            result["longitude"].get<double>()));
    }

    return locations;
  });
};

auto API::search_location(std::string query)
    -> std::future<std::optional<Location>> {
  return std::async(std::launch::async, [&] {
    const auto locations = API::search_locations(query).get();
    return locations.empty() ? std::optional<Location>()
                             : std::make_optional(locations[0]);
  });
}

}; // namespace Weather
