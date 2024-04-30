cmake_minimum_required (VERSION 3.11)

project(cpp-tui-workshop LANGUAGES CXX)

find_package(ftxui REQUIRED)
find_package(cpr REQUIRED)

function(register_executable TARGET_NAME SRC_PATH LIBRARIES)
	file(GLOB_RECURSE SRC_FILES "${SRC_PATH}/*.cpp")
	add_executable("${TARGET_NAME}.out" ${SRC_FILES})
	target_link_libraries("${TARGET_NAME}.out" ${LIBRARIES})
endfunction()

register_executable(
	"weather-app"
	"weather-app"
	"ftxui::screen;ftxui::dom;ftxui::component;cpr::cpr"
)

register_executable(
	"basic-demo"
	"basic-demo"
	"ftxui::screen;ftxui::dom;ftxui::component"
)
