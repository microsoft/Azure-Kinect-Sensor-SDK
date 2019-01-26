add_library(k4a_depth STATIC 
            depth.c
            )

# Consumers should #include <k4ainternal/depth.h>
target_include_directories(k4a_depth PUBLIC 
    ${K4A_PRIV_INCLUDE_DIR})

# Dependencies of this library
target_link_libraries(k4a_depth PUBLIC 
    k4ainternal::depth_mcu
    k4ainternal::logging)


# Define alias for other targets to link against
add_library(k4ainternal::depth ALIAS k4a_depth)