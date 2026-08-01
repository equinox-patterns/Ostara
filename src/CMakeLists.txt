add_subdirectory(libs)
add_subdirectory(app)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
