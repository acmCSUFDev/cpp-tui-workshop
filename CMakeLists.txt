cmake_minimum_required (VERSION 3.11)

project(cpp-tui-workshop LANGUAGES CXX)

find_package(ftxui REQUIRED)
find_package(cpr REQUIRED)

file(GLOB_RECURSE SRC_FILES "src/*.cpp")
add_executable(cpp-tui-workshop ${SRC_FILES})
target_include_directories(cpp-tui-workshop PRIVATE src)

target_link_libraries(cpp-tui-workshop
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
  PRIVATE cpr::cpr
)
