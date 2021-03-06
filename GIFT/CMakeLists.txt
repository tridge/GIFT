# Set up output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")

include_directories("${PROJECT_BINARY_DIR}")

# Include libraries from outside project
find_package(OpenCV 3 REQUIRED)
find_package(Eigen3 3.3 REQUIRED)

set(ADDITIONAL_INCLUDE_DIRS
${EIGEN_INCLUDE_DIRS}
# ${EIGEN_INCLUDE_DIR}/unsupported
${OpenCV_INCLUDE_DIRS})

# Find project files
####################

set(GIFT_SOURCE_FILES
    src/FeatureTracker.cpp
    src/StereoFeatureTracker.cpp
    src/EgoMotion.cpp
    src/Landmark.cpp
)

set(GIFT_HEADER_FILES
    include/CameraParameters.h
    include/FeatureTracker.h
    include/StereoFeatureTracker.h
    include/Landmark.h
    include/EgoMotion.h
)

# optional dependency: yaml-cpp
find_package(yaml-cpp REQUIRED)
if(yaml-cpp_FOUND)
    message("Including optional dependency yaml-cpp")
    list(APPEND GIFT_SOURCE_FILES
    src/Configure.cpp)
    list(APPEND GIFT_HEADER_FILES
    include/Configure.h)
else()
    message("yaml-cpp was not found.")
endif()

# Add files from within the project
add_library(GIFT
    ${GIFT_SOURCE_FILES}
    ${GIFT_HEADER_FILES}
)

# Build local library
#####################
target_include_directories(GIFT PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>  
    $<INSTALL_INTERFACE:include>
    ${ADDITIONAL_INCLUDE_DIRS}
)

target_link_libraries(GIFT
    ${OpenCV_LIBS}
    yaml-cpp
    # Eigen3::Eigen
)

# INSTALLATION
##############

set(config_install_dir "lib/cmake/${PROJECT_NAME}")
set(include_install_dir "include/${PROJECT_NAME}")
set(project_config "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake")
set(targets_export_name "${PROJECT_NAME}Targets")

include(CMakePackageConfigHelpers)

configure_package_config_file(
    "modules/Config.cmake.in"
    "${project_config}"
    INSTALL_DESTINATION "${config_install_dir}")


# Export '<PROJECT-NAME>Targets.cmake' to build dir (to find package in build dir without install)
export(TARGETS GIFT FILE "${CMAKE_CURRENT_BINARY_DIR}/${targets_export_name}.cmake")

# # Targets:
install(TARGETS GIFT
        EXPORT "${targets_export_name}"
        LIBRARY DESTINATION "lib"
        ARCHIVE DESTINATION "lib"
        RUNTIME DESTINATION "bin"
        INCLUDES DESTINATION "${include_install_dir}")

# Config
#   * <prefix>/lib/cmake/opengv/opengvConfig.cmake
#   * <prefix>/lib/cmake/opengv/opengvConfigVersion.cmake
install(FILES "${project_config}"
        DESTINATION "${config_install_dir}")

# Config
#   * <prefix>/lib/cmake/opengv/opengvTargets.cmake
install(EXPORT "${targets_export_name}"
        # NAMESPACE "${namespace}"
        DESTINATION "${config_install_dir}")
# message("The namespace is ${namespace}")

# Headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION ${include_install_dir} 
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")