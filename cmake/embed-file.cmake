function(embed_file INPUT_FILE OUTPUT_HEADER VAR_NAME)
    get_filename_component(INPUT_ABS ${INPUT_FILE} ABSOLUTE)
    get_filename_component(OUTPUT_ABS ${OUTPUT_HEADER} ABSOLUTE)

    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/embedded)

    add_custom_command(
        OUTPUT ${OUTPUT_ABS}
        COMMAND ${CMAKE_COMMAND}
                -DINPUT_FILE=${INPUT_ABS}
                -DOUTPUT_FILE=${OUTPUT_ABS}
                -DVAR_NAME=${VAR_NAME}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed-generator.cmake
        DEPENDS ${INPUT_ABS} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed-generator.cmake
        COMMENT "Embedding ${INPUT_FILE} -> ${OUTPUT_ABS}"
        VERBATIM
    )

    set(${VAR_NAME}_HEADER ${OUTPUT_ABS} PARENT_SCOPE)
endfunction()
