#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

int main() {
  auto interface = [&] {
    return window(text(" Title ") | center | bold,
                  vbox({
                      filler(),
                      text("Hello!!") | center,
                      filler(),
                  }));
  };

  auto screen = ScreenInteractive::Fullscreen();
  screen.Loop(Renderer(interface));
}
