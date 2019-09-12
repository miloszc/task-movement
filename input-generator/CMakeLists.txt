cmake_minimum_required(VERSION 2.6)

# The project name and target name should be different to generate
# project for Eclipse CDT 
project(CartesianMeshProject)
# Generate tests for ctest
enable_testing()

# The version number
set(CartesianMesh_VERSION_MAJOR 1)
set(CartesianMesh_VERSION_MINOR 0)

# Configure a header file to pass some of the CMake settings 
# to the source code
configure_file(
    "${PROJECT_SOURCE_DIR}/CartesianMeshConfig.h.in"
    "${PROJECT_BINARY_DIR}/CartesianMeshConfig.h"
    )

### cartesian_mesh target
add_executable(cartesian_mesh 
    cartesian_mesh.c
)

### install
install(TARGETS cartesian_mesh DESTINATION bin)
install(FILES "${PROJECT_BINARY_DIR}/CartesianMeshConfig.h"
    DESTINATION include)