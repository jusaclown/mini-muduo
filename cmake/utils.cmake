# Turn on warnings on the given target
function(muduo_enable_warnings target_name)
    message(STATUS "${target_name} warnings enabled")
    target_compile_options(
        ${target_name}
        PRIVATE 
            -Wall
            -Wextra
            -Wconversion
            -pedantic
            -Werror
            -Wfatal-errors
            -Wno-unused-parameter
            -Wold-style-cast
            -Woverloaded-virtual
            -Wpointer-arith
            -Wshadow
            -Wwrite-strings
    ) 
endfunction()


# Enable address sanitizer (gcc/clang only)
function(muduo_enable_sanitizer target_name)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "Sanitizer supported only for gcc/clang")
    endif()
    message(STATUS "${target_name} Address sanitizer enabled")
    target_compile_options(${target_name} PRIVATE -fsanitize=address,undefined)
    target_compile_options(${target_name} PRIVATE -fno-sanitize=signed-integer-overflow)
    target_compile_options(${target_name} PRIVATE -fno-sanitize-recover=all)
    target_compile_options(${target_name} PRIVATE -fno-omit-frame-pointer)
    target_link_libraries(${target_name} PRIVATE -fsanitize=address,undefined -fuse-ld=gold)
endfunction()
