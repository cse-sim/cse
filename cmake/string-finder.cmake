file(READ ${file} file_content)

string(FIND ${file_content} ${error_string} pos)
message(${pos})
