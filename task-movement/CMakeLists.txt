cmake_minimum_required(VERSION 2.6)

# project for Eclipse CDT or Visual Studio
project(SchedulerProject)

# The version number
set(Scheduler_VERSION_MAJOR 1)
set(Scheduler_VERSION_MINOR 0)

# Configure a header file to pass some of the CMake settings 
# to the source code
configure_file(
    "${PROJECT_SOURCE_DIR}/SchedulerConfig.h.in"
    "${PROJECT_BINARY_DIR}/SchedulerConfig.h"
    )

### scheduler target
add_executable(scheduler 
    main.cpp 
    cost.cpp
    Koala/tinyxml/tinyxml.cpp 
    Koala/tinyxml/tinyxmlparser.cpp  
    Koala/tinyxml/tinystr.cpp 
    Koala/tinyxml/tinyxmlerror.cpp
	basic_types.h
	cost.h
	main.h
	multigraph.h
	processorgraph.h
	read.h
	schedparams.h
	scheduling.h
	taskgraph.h
)
# add the binary tree to the search path for include files
# so that we will find SchedulerConfig.h
target_include_directories(scheduler PRIVATE "${PROJECT_BINARY_DIR}")
#target_compile_options(scheduler)
# GNUC
#target_compile_options(scheduler PRIVATE -fpermissive -g -O2 -std=c++11 -Wall)
target_link_libraries(scheduler)

### install
install(TARGETS scheduler DESTINATION bin)
install(FILES "${PROJECT_BINARY_DIR}/SchedulerConfig.h"
    DESTINATION include)
