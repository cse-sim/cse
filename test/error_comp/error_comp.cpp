#include <iostream>
#include <fstream>
#include <string>

int main( int argc, char **argv)
{
    std::string file_name{ argv[1] }, error_code{ argv[2] };
    std::ifstream error_file (file_name);

    if (error_file.is_open() && !error_file.eof()) {
        std::string line;
        size_t loc;
        while (std::getline(error_file, line)) {
            if (line.find(error_code) != -1) {
                return 0; // Error code found
            }
        }
    }
    // Could not open the file or error code not found
    return -1;
}