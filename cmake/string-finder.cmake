file(READ ${file} file_content)

string(FIND ${file_content} ${error_string} content_location)

if (${content_location} LESS 0)
    message(FATAL_ERROR "Content not found in the file, ${content_location}")
else()
    message(${content_location})
endif()
