file(READ ${file} file_content)

string(FIND ${file_content} ${error_string} pos)

if (${pos} LESS 0)
    message(FATAL_ERROR "${pos}")
else()
    message(${pos})
endif()
