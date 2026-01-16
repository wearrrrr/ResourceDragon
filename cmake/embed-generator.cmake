# embed-generator.cmake
file(READ "${INPUT_FILE}" BINARY_CONTENT HEX)

# Compute length
string(LENGTH "${BINARY_CONTENT}" HEX_LEN)
math(EXPR BYTE_LEN "${HEX_LEN} / 2")

# Start writing header
file(WRITE  "${OUTPUT_FILE}" "#pragma once\n")
file(APPEND "${OUTPUT_FILE}" "#include <stddef.h>\n")
file(APPEND "${OUTPUT_FILE}" "inline const unsigned char ${VAR_NAME}[] = {\n\t")

set(LINE "")
set(COUNT 0)

foreach(INDEX RANGE 0 ${BYTE_LEN}-1)
    math(EXPR POS "${INDEX} * 2")
    string(SUBSTRING "${BINARY_CONTENT}" ${POS} 2 BYTE)
    if(BYTE)  # Only append if BYTE is not empty
        set(LINE "${LINE}0x${BYTE}, ")
        math(EXPR COUNT "${COUNT} + 1")

        if(COUNT EQUAL 16)
            file(APPEND "${OUTPUT_FILE}" "${LINE}\n\t")
            set(LINE "")
            set(COUNT 0)
        endif()
    endif()
endforeach()

# Append any remaining bytes
if(LINE)
    file(APPEND "${OUTPUT_FILE}" "${LINE}\n")
endif()

file(APPEND "${OUTPUT_FILE}" "};\n")
file(APPEND "${OUTPUT_FILE}" "inline const size_t ${VAR_NAME}_len = ${BYTE_LEN};\n")
