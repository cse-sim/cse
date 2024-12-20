#include "gtest/gtest.h"

#include "cnglob.h"
#include "xiopak.h"

// Include filesystem
#if __has_include(<filesystem>)
#include <filesystem>
namespace filesys = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;
#else
#error "no filesystem support"
#endif

RC check_sec(SEC sec)
{
    // If sec == SECOK, return RC of zero
    return sec != SECOK;
}

TEST(xiopak, file_operations)
{
    // Open file for writing
    SEC sec;
    RC rc{RCOK};
    const char* file1{"file_1.txt"};
    XFILE *xf1 = xfopen(file1, O_WT, WRN, TRUE, &sec);
    rc |= check_sec(sec);
    EXPECT_FALSE(rc) << "Open file for write";

    // Write to file
    const char* write_content{"XIOPAK Works!"};
    rc |= check_sec(xfwrite(xf1, write_content, strlen(write_content)));
    EXPECT_FALSE(rc) << "Write file";
    rc |= check_sec(xfclose(&xf1, &sec));
    EXPECT_FALSE(rc) << "Close file after write";

    // Open file for reading
    xf1 = xfopen(file1, O_RT, WRN, FALSE, &sec);
    rc |= check_sec(sec);
    EXPECT_FALSE(rc) << "Open file for read";

    // Read from file
    char read_content[100] = {0};
    rc |= check_sec(xfread(xf1, read_content, static_cast<USI>(strlen(write_content))));
    EXPECT_FALSE(rc) << "Read file";

    // Confirm contents is the same
    EXPECT_FALSE(_stricmp(read_content, write_content));

    // Get directory
    const char* cwd = xgetdir();
    const char* path = file1;

    // Get path
    xfGetPath(&path, WRN);

    // Check if cwd + file1 = path
    char joined_path[400] = {0};
    xfjoinpath(cwd, file1, joined_path);
    EXPECT_FALSE(_stricmp(joined_path, path));

    // Close file
    rc |= check_sec(xfclose(&xf1, &sec));
    EXPECT_FALSE(rc) << "Close file after read";

    // Delete file
    rc |= check_sec(xfdelete(file1, WRN));
    EXPECT_FALSE(rc) << "Delete file";

    // Ensure the file is gone!
    EXPECT_FALSE(xfExist(file1));

}

TEST(xiopak, path_functions) {

    filesys::path true_path{ MAKE_LITERAL(SOURCE_DIR) };
    true_path /= "test/unit/xiopak.unit.cpp";

    // Check real path
    {
        char pbuf[CSE_MAX_PATH * 4];
#define part(p) (pbuf+((p)*CSE_MAX_PATH))

        // Filesystem
        xfpathroot(true_path.string().c_str(), part(0));
        xfpathdir(true_path.string().c_str(), part(1));
        xfpathstem(true_path.string().c_str(), part(2));
        xfpathext(true_path.string().c_str(), part(3));

        // Compare
        char true_dir[2046];
        strcpy(true_dir, true_path.root_directory().string().c_str());
        strcat(true_dir, true_path.relative_path().parent_path().string().c_str());
        strcat(true_dir, true_path.root_directory().string().c_str());

        EXPECT_STREQ(part(0), true_path.root_name().string().c_str());
        EXPECT_STREQ(part(1), true_dir);
        EXPECT_STREQ(part(2), true_path.stem().string().c_str());
        EXPECT_STREQ(part(3), true_path.extension().string().c_str());
    }

    {   // False path
#if CSE_OS == CSE_OS_WINDOWS
        filesys::path false_path{ "F:/something/does/not/exist.dat" };
#else
        filesys::path false_path{ "/something/does/not/exist.dat" };
#endif
        char pbuf[CSE_MAX_PATH * 4];

        // Filesystem
        xfpathroot(false_path.string().c_str(), part(0));
        xfpathdir(false_path.string().c_str(), part(1));
        xfpathstem(false_path.string().c_str(), part(2));
        xfpathext(false_path.string().c_str(), part(3));

#if CSE_OS == CSE_OS_WINDOWS
        EXPECT_STREQ(part(0), "F:");
#else
        EXPECT_STREQ(part(0), "");
#endif
        EXPECT_STREQ(part(1), "/something/does/not/");
        EXPECT_STREQ(part(2), "exist");
        EXPECT_STREQ(part(3), ".dat");
    }

}
