cmake_minimum_required(VERSION 2.8)

cpu_setup("haripo")

add_library(haripo_mc_lib
            playout_evaluator.cc)

function(haripo_add_executable exe)
  cpu_add_executable(${exe} ${ARGN})
  cpu_target_link_libraries(${exe} haripo_mc_lib)
  cpu_target_link_libraries(${exe} mayah_lib)
  cpu_target_link_libraries(${exe} mayah_thinker_lib)
  cpu_target_link_libraries(${exe} mayah_evaluator_lib)
  cpu_target_link_common_libraries(${exe})
endfunction()

# cpu_add_runner(run.sh)

haripo_add_executable(haripo_mc montecarlo.cc)
