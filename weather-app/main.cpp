#include <exception>
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <future>
#include <variant>

#include "async.hpp"
#include "weather.hpp"

// Normally, `using namespace` is pretty bad, but we're giving ftxui an
// exception because the API is just so nice!
using namespace ftxui;

// The default current location is Fullerton, CA.
const Weather::Location currentLocation{
    .name = "Fullerton, CA",
    .timezone = "America/Los_Angeles",
    .coordinates = std::make_pair(33.8703, -117.9253),
};

int main() {
  // Allocate us a new screen.
  auto screen = ScreenInteractive::Fullscreen();
  auto exit = [&screen] { screen.ExitLoopClosure()(); };

  std::variant<
      // No weather data yet.
      std::monostate,
      // Weather data is loaded into a pointer.
      std::unique_ptr<Weather::Conditions>,
      // An error occurred.
      std::exception_ptr>
      weather_maybe;
  std::future<Weather::Conditions> weather_future;

  auto update_weather = [&] {
    weather_maybe = std::monostate{};
    weather_future = std::async(std::launch::async, [&] {
      // Wait for the weather to be done fetching.
      auto promise = Weather::API::fetch_weather(currentLocation);
      promise.wait();

      // Emit an event to force the screen to refresh.
      // PostEvent is thread-safe.
      screen.PostEvent(Event::Custom);

      return promise.get();
    });
  };

  // Update the weather condition asynchronously in the background.
  update_weather();

  auto weather_component = [&]() {
    switch (weather_maybe.index()) {
      case 0: {
        return vbox({
            text("Loading weather data...") | center,
        });
      }
      case 1: {
        auto weather =
            *std::get<std::unique_ptr<Weather::Conditions>>(weather_maybe);
        return vbox({
            text("Weather in " + weather.location.name) | bold,
            separatorEmpty(),
            text("Temperature: " + weather.temperature.as_string()),
            text("Humidity: " + weather.humidity.as_string()),
        });
      }
      case 2: {
        return vbox({
            text("Error loading data:") | bold,
            text(get_exception_message(
                std::get<std::exception_ptr>(weather_maybe))),
        });
      }
      default:
        throw std::runtime_error("Invalid index");
    }
  };

  auto refresh_button = Button(
      "Refresh", [&] { update_weather(); }, ButtonOption::Border());

  auto exit_button = Button(
      "Exit", [&] { exit(); }, ButtonOption::Border());

  auto window_component = [&] {
    return window(text(" Weather App ") | bold | center,
                  vbox({
                      separatorEmpty(),
                      hbox({
                          separatorEmpty(),
                          weather_component(),
                          separatorEmpty(),
                      }),
                      separatorEmpty(),
                      separator(),
                      hbox({
                          separatorEmpty(),
                          refresh_button->Render() | flex,
                          separatorEmpty(),
                          exit_button->Render() | flex,
                          separatorEmpty(),
                      }),
                  })) |
           size(WIDTH, GREATER_THAN, 50) | center;
  };

  // This defines how to navigate using the keyboard.
  // List all interactive elements here.
  auto window_controls = Container::Horizontal({
      refresh_button,
      exit_button,
  });

  /*
   * Event Handler
   */

  auto on_event = [&](Event event) {
    if (event == Event::Character('q') || event == Event::Escape) {
      exit();
      return true;
    }

    if (event == Event::Custom) {
      // Check if the weather data is ready.
      if (weather_future.valid()) {
        try {
          weather_maybe =
              std::make_unique<Weather::Conditions>(weather_future.get());
        } catch (...) {
          weather_maybe = std::current_exception();
        }
        return true;
      } else if (weather_maybe.index() == 0) {
        // Data is still loading. Try again later.
        screen.PostEvent(Event::Custom);
      }
    }

    return false;
  };

  // Start the main loop.
  screen.Loop(Renderer(window_controls, window_component) |
              CatchEvent(on_event));

  return 0;
}
