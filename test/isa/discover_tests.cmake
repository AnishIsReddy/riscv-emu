# Invoked at post-build with:
#   -D TEST_EXECUTABLE=<path to test binary>
#   -D OUT_FILE=<path of generated ctest include file>
# Asks the binary for its test list and writes one add_test() per name.
# CTest sources OUT_FILE at test time via the TEST_INCLUDE_FILES property.

execute_process(
        COMMAND "${TEST_EXECUTABLE}" --list
        OUTPUT_VARIABLE listing
        ERROR_VARIABLE errout
        RESULT_VARIABLE rc
)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR
            "test discovery failed: '${TEST_EXECUTABLE} --list' exited ${rc}\n${errout}")
endif()

string(STRIP "${listing}" listing)

if(listing STREQUAL "")
    # No tests registered (yet). Write an empty file so ctest still works.
    file(WRITE "${OUT_FILE}" "")
    return()
endif()

string(REPLACE "\n" ";" test_names "${listing}")

set(script "")
foreach(name IN LISTS test_names)
    string(STRIP "${name}" name)
    if(NOT name STREQUAL "")
        string(APPEND script
                "add_test(\"${name}\" \"${TEST_EXECUTABLE}\" \"${name}\")\n")
    endif()
endforeach()

file(WRITE "${OUT_FILE}" "${script}")