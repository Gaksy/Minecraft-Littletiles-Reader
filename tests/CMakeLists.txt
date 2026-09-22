# Unit tests for the structure reader / writer (the two SNBT dialects and the
# conversion between them). Turn them off with -DGALIB_BUILD_TESTS=OFF, e.g. when
# only the CLI is being packaged.
add_executable(lt_structure_test lt_structure_test.cpp)
target_link_libraries(lt_structure_test PRIVATE galib)
target_compile_definitions(
  lt_structure_test
  PRIVATE TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

add_test(NAME lt_structure_test COMMAND lt_structure_test)

# The test also accepts a real structure file (1.12.2 house or 1.20 blueprint) and
# then round trips every box of it; the test data is not part of this repository,
# so that part stays opt-in:
#   <build>/tests/lt_structure_test <path to a .txt/.struct structure>
