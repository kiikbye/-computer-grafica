add_executable(week9 main.cpp)
target_link_libraries(week9 PRIVATE common)

option(WEEK9_BUILD_CHECKS "Build camera and cube geometry checks" OFF)
if(WEEK9_BUILD_CHECKS)
    add_executable(week9_checks tests/camera_checks.cpp)
    target_link_libraries(week9_checks PRIVATE common)
endif()
