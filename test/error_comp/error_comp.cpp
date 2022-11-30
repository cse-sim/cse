#include <iostream>
#include <fstream>

int main( int argc, char **argv)
{
    std::fstream error_file;
    std::string file_content, file_name{ argv[1] }, error_code{argv[2]};
    error_file.open("D:\code\cse\test\error_handling\no_error.err", std::fstream::in);
    if(!error_file && error_file.is_open()) {
        error_file >> file_content;
        size_t loc = file_content.find(error_code);
        if (loc != file_content.length()) {
            return 0;
        }
    }
    return -1;
}