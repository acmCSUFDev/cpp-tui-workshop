#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

int main() {
  auto interface = [&] {
    return window(text("Title") | center, vbox({
                                              text("Hello!!") | center,
                                          }));
  };

  auto screen = ScreenInteractive::Fullscreen();
  screen.Loop(Renderer(interface));
}
