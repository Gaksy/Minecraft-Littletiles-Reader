# Unit tests for the structure reader / writer (the two SNBT dialects and the
# conversion between them). Turn them off with -DGALIB_BUILD_TESTS=OFF, e.g. when
# only the CLI is being packaged.
add_executable(lt_structure_test lt_structure_test.cpp)
target_link_libraries(lt_structure_test PRIVATE galib)
target_compile_definitions(
  lt_structure_test
  PRIVATE TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

add_test(NAME lt_structure_test COMMAND lt_structure_test)

add_executable(block_tile_entities_test block_tile_entities_test.cpp)
target_link_libraries(block_tile_entities_test PRIVATE galib)
add_test(NAME block_tile_entities_test COMMAND block_tile_entities_test)
add_executable(bbmodel_export_test bbmodel_export_test.cpp)
target_link_libraries(bbmodel_export_test PRIVATE galib)
target_compile_definitions(bbmodel_export_test PRIVATE TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
add_test(NAME bbmodel_export_test COMMAND bbmodel_export_test)
add_executable(world_profile_test world_profile_test.cpp)
target_link_libraries(world_profile_test PRIVATE galib)
add_test(NAME world_profile_test COMMAND world_profile_test)
add_executable(texture_animation_test texture_animation_test.cpp)
target_link_libraries(texture_animation_test PRIVATE galib)
add_test(NAME texture_animation_test COMMAND texture_animation_test)
if(WIN32)
    # Tests must run directly from their own directory, not depend on PATH.
    foreach(test_target lt_structure_test block_tile_entities_test bbmodel_export_test world_profile_test texture_animation_test)
        add_custom_command(TARGET ${test_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:nbt++> $<TARGET_FILE_DIR:${test_target}>)
    endforeach()
endif()

# The test also accepts a real structure file (1.12.2 house or 1.20 blueprint) and
# then round trips every box of it; the test data is not part of this repository,
# so that part stays opt-in:
#   <build>/tests/lt_structure_test <path to a .txt/.struct structure>
