# CMake generated Testfile for 
# Source directory: /home/runner/work/parangonar/parangonar
# Build directory: /home/runner/work/parangonar/parangonar/build_test_emscripten
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(parangonar_tests "/usr/bin/node" "--experimental-wasm-threads" "/home/runner/work/parangonar/parangonar/build_test_emscripten/test_parangonar_cpp")
set_tests_properties(parangonar_tests PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/parangonar/parangonar/CMakeLists.txt;83;add_test;/home/runner/work/parangonar/parangonar/CMakeLists.txt;0;")
add_test(wasm_api_tests "/usr/bin/node" "--experimental-wasm-threads" "/home/runner/work/parangonar/parangonar/build_test_emscripten/test_wasm_api")
set_tests_properties(wasm_api_tests PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/parangonar/parangonar/CMakeLists.txt;84;add_test;/home/runner/work/parangonar/parangonar/CMakeLists.txt;0;")
