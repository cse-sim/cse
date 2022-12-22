# string-finder: Returns an error if string is not found in the given file. Otherwise exits gracefully.
file(READ ${file} file_content)

string(FIND ${file_content} ${string} content_location)

if (${content_location} LESS 0)
    message(FATAL_ERROR "STRING-FINDER ERROR: String, \"${string}\", not found in the file, \"${file}\".")
endif()
