function(vst3_configure_target target_name)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Target ${target_name} does not exist")
    endif()

    set(default_bin_dir "${CMAKE_BINARY_DIR}/bin")
    if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${default_bin_dir} CACHE PATH "" FORCE)
    endif()

    if(APPLE)
        # macOS bundle is already handled via MACOSX_BUNDLE properties
        return()
    endif()

    if(WIN32)
        set(platform_tag "x86_64-win")
    else()
        set(platform_tag "x86_64-linux")
    endif()

    set(bundle_dir "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.vst3")
    set(contents_dir "${bundle_dir}/Contents/${platform_tag}")
    set(target_file "$<TARGET_FILE:${target_name}>")

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${contents_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy "${target_file}" "${contents_dir}/$<TARGET_FILE_NAME:${target_name}>"
        COMMENT "Packaging ${target_name} into ${bundle_dir}"
    )
endfunction()
