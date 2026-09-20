cmake_minimum_required(VERSION 3.20)
project(FoundryMechanicsIntegration LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(OVERKILL_BUILD_CORE_TOOLS ON CACHE BOOL "Include original core and runner regressions" FORCE)
enable_testing()
add_subdirectory(../.. core-runtime)

# Every contract below links the same authoritative core library. Historical
# per-suite captures remain separate; this target checks their integration.
foreach(suite part_lifecycle upgrade_contract shared_recipe_contract mara_recipe_contract upgrade_copy_contract encounter_contract campaign_lifecycle_contract)
  add_executable(${suite}_tests ../${suite}_tests.cpp)
  target_link_libraries(${suite}_tests PRIVATE overkill_core)
  add_test(NAME integrated_${suite} COMMAND ${suite}_tests)
endforeach()
add_executable(terminal_transition_tests ../terminal/terminal_tests.cpp)
target_link_libraries(terminal_transition_tests PRIVATE overkill_core)
add_test(NAME integrated_terminal_transitions COMMAND terminal_transition_tests)

set(contract_targets part_lifecycle_tests upgrade_contract_tests shared_recipe_contract_tests mara_recipe_contract_tests upgrade_copy_contract_tests encounter_contract_tests campaign_lifecycle_contract_tests terminal_transition_tests
    overkill_core_tests overkill_route_tests overkill_robot_tests overkill_recipe_tests overkill_upgrade_tests overkill_campaign_tests overkill_campaign_upgrade_tests overkill_runner_tests)
foreach(target IN LISTS contract_targets)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX /permissive- /utf-8)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
endforeach()
