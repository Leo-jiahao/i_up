cmake_minimum_required(VERSION 3.1)



#add user head files to the end of compiler's include path list
include_directories(
    .
)

set(src
    can_protocol.c
    utilities.c
)


add_library(kp_can ${src})

# target_link_libraries(r_bootm m)
