#include "weather.hpp"
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <optional>

// Normally, `using namespace` is pretty bad, but we're giving ftxui an
// exception because the API is just so nice!
using namespace ftxui;

int main() {
  // Allocate us a new screen.
  auto screen = ScreenInteractive::Fullscreen();

  // The default current location is Fullerton, CA.
  const Weather::Location currentLocation("Fullerton, CA", 33.8703, -117.9253);

  // Create a wrapper that keeps track of the current weather condition.
  // The wrapper starts off not having a value, and will be updated every time
  // we call update().
  auto weather = Weather::ConditionsFetcher(currentLocation, [&] {
    // When the weather condition is updated,
    // emit an event to force the screen to refresh.
    screen.PostEvent(Event::Custom);
  });

  // Update the weather condition in the background.
  weather.update();

  /*
   * Components
   */

  auto loading_component = [&] {
    return hbox({
        text("Loading weather data...") | center,
    });
  };

  auto weather_component = [&] {
    return vbox({
        text("Weather in " + currentLocation.name) | bold,
        separatorEmpty(),
        text("Temperature: " + weather->temperature_string()),
        text("Humidity: " + weather->humidity_string()),
    });
  };

  /*
   * Interactive Buttons
   */

  auto refresh_button = Button(
      "Refresh", [&] { weather.update(); }, ButtonOption::Border());

  auto exit_button = Button(
      "Exit",
      [&] {
        const auto exit = screen.ExitLoopClosure();
        exit();
      },
      ButtonOption::Border());

  /*
   * Main Window
   */

  auto window_body = [&] {
    return vbox({
        separatorEmpty(),
        weather.has_value() ? weather_component() | center
                            : loading_component() | center,
        separatorEmpty(),
        separator(),
        hbox({
            separatorEmpty(),
            refresh_button->Render() | flex,
            separatorEmpty(),
            exit_button->Render() | flex,
            separatorEmpty(),
        }),
    });
  };

  auto window_component = [&] {
    auto title = text(" Weather App ") | bold | center;
    return window(title, window_body()) | size(WIDTH, GREATER_THAN, 50) |
           center;
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

      // Exit the application when 'q' or 'Escape' is pressed.
      const auto exit = screen.ExitLoopClosure();
      exit();

      return true;
    }

    return false;
  };

  // Start the main loop.
  screen.Loop(Renderer(window_controls, window_component) |
              CatchEvent(on_event));

  return 0;
}
