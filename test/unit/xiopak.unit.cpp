#include "gtest/gtest.h"

#include "cnglob.h"
#include "xiopak.h"
#include "srd.h"

// Stubs defined separately for RCDEF
LI Dttab[691];
UNIT Untab[80*sizeof(UNIT)];
int Unsysext = 0;


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