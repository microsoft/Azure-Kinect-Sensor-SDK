add_executable(deversion main.c)

target_link_libraries(deversion PRIVATE
    k4ainternal::dynlib
    k4ainternal::logging)
