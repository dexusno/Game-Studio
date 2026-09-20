cmake_minimum_required(VERSION 3.20)
project(OverkillUpgradeReview LANGUAGES CXX)
set(OVERKILL_QA_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." CACHE PATH "Exact reviewed game source")
set(core "${OVERKILL_QA_SOURCE_ROOT}/core")
add_executable(upgrade_probes probes.cpp
  "${core}/src/core.cpp" "${core}/src/serialization.cpp"
  "${core}/src/catalogue.cpp" "${core}/src/robots.cpp"
  "${core}/src/upgrades.cpp")
target_include_directories(upgrade_probes PRIVATE "${core}/include")
target_compile_features(upgrade_probes PRIVATE cxx_std_17)
if(MSVC)
  target_compile_options(upgrade_probes PRIVATE /W4 /WX /permissive-)
endif()
