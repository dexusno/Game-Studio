cmake_minimum_required(VERSION 3.20)
project(OverkillUpgradeCampaignReview LANGUAGES CXX)
set(OVERKILL_BUILD_CORE_TOOLS OFF CACHE BOOL "Independent probes only" FORCE)
set(OVERKILL_QA_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." CACHE PATH "Exact captured game source")
add_subdirectory("${OVERKILL_QA_SOURCE_ROOT}/core" core-runtime EXCLUDE_FROM_ALL)
add_executable(upgrade_campaign_probes probes.cpp
  "${OVERKILL_QA_SOURCE_ROOT}/platform/src/campaign_session.cpp"
  "${OVERKILL_QA_SOURCE_ROOT}/platform/src/windows_save_store.cpp")
target_include_directories(upgrade_campaign_probes PRIVATE "${OVERKILL_QA_SOURCE_ROOT}/platform/include")
target_link_libraries(upgrade_campaign_probes PRIVATE overkill_core bcrypt)
target_compile_features(upgrade_campaign_probes PRIVATE cxx_std_17)
target_compile_definitions(upgrade_campaign_probes PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_options(upgrade_campaign_probes PRIVATE /W4 /WX /permissive-)
