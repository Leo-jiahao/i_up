cmake_minimum_required(VERSION 3.1)



#add user head files to the end of compiler's include path list
include_directories(
    .
)

set(src
    r_boot_master.c
)


add_library(r_bootm ${src})

# target_link_libraries(r_bootm m)
