include(CMakeParseArguments)

# Adds 'lib' to the include dir and libraries used by target 'name'.
function(require_lib name lib)
    # Find Package scripts use inconsistent naming :(
    if (${lib}_INCLUDE_DIR)
        target_include_directories(${name} PUBLIC ${${lib}_INCLUDE_DIR})
    elseif (${lib}_INCLUDE_DIRS)
        target_include_directories(${name} PUBLIC ${${lib}_INCLUDE_DIRS})
    elseif (${lib}_HAS_NO_INCLUDE_DIRS)
    else ()
        message("${name} - Warning: No include dirs for ${lib}")
    endif ()
    if (${lib}_LIBRARY)
        target_link_libraries(${name} ${${lib}_LIBRARY})
    elseif (${lib}_LIBRARIES)
        target_link_libraries(${name} ${${lib}_LIBRARIES})
    elseif (${lib}_HAS_NO_LIBRARIES)
    else ()
        message("${name} - Warning: No libraries for ${lib}")
    endif ()
    set_property(TARGET ${name} APPEND_STRING PROPERTY COMPILE_FLAGS " -DFOUND_${lib}=1 ")
endfunction()


# Function to generate executable targets, based on their dependencies.
# Specify LIBRARY to create a library instead of an executable.
# SOURCE lists the source code files of the target
# REQUIRED lists the mandatory libraries
# OPTIONAL lists the libraries that are used for additional functionality.
# (use #ifdef FOUND_LIB to test whether lib is available).
function(add_target name) 
    cmake_parse_arguments(TARGET "LIBRARY" "HEADERS" "SOURCE;REQUIRED;OPTIONAL" ${ARGN})
    string(TOUPPER "${TARGET_REQUIRED}" TARGET_REQUIRED)
    string(TOUPPER "${TARGET_OPTIONAL}" TARGET_OPTIONAL)
    set(REQUIRED_SATISFIED TRUE)
    set(MISSING "")
    # Check if required libraries are available
    foreach(LIB ${TARGET_REQUIRED})
        if (NOT ${LIB}_FOUND)
            set(REQUIRED_SATISFIED FALSE)
            set(MISSING "${MISSING} ${LIB}")
        endif ()
    endforeach()
    if (REQUIRED_SATISFIED)
        set(BUILDABLE_TARGETS "${BUILDABLE_TARGETS} ${name}" PARENT_SCOPE)
        # Create the target
        if (TARGET_LIBRARY)
            add_library(${name} ${TARGET_SOURCE})
            string(TOUPPER "${name}" UPPER_NAME)
            set(${UPPER_NAME}_FOUND TRUE PARENT_SCOPE)
            set(${UPPER_NAME}_LIBRARIES "${name}" PARENT_SCOPE)
            if (TARGET_HEADERS)
                set(${UPPER_NAME}_INCLUDE_DIR ${TARGET_HEADERS} PARENT_SCOPE)
            else()
                set(${UPPER_NAME}_HAS_NO_INCLUDE_DIRS TRUE PARENT_SCOPE)
            endif()
        else ()
            add_executable(${name} ${TARGET_SOURCE})
        endif ()
        # Attach the libraries.
        foreach(LIB ${TARGET_REQUIRED})
            # We already know these libraries exist
            require_lib(${name} ${LIB})
        endforeach()
        foreach(LIB ${TARGET_OPTIONAL})
            if (${LIB}_FOUND)
                require_lib(${name} ${LIB})
            endif ()
        endforeach ()
    else ()
        message("Cannot build ${name}. Libraries missing:${MISSING}")
    endif ()
endfunction ()