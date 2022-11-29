#include <regex>
#include <iostream>
#include <fstream>

int main( int argc, char **argv)
{
    std::ifstream ifs(argv[0]);
    std::regex genex{"\\bP0003\\b"};
    std::string line;

    while( std::getline(ifs, line ))
    {
        std::cout << line;
        if ( std::regex_search( line, genex) )
            return 0;
    }
    return -1;
}