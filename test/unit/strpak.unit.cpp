#include "gtest/gtest.h"

#include "cnglob.h"
#include "strpak.h"

TEST(strpak, compare_case_insensitive_string) 
{
	// Set up strings to compare
	const char* hello_upper_case = "HELLO";
	const char* hello_lower_case = "hello";
	const char* hello_wrong = "H3llo";
	const char* long_string_upper_case = "This IS a LONG string.";
	const char* long_string_lower_case = "this is a long string ";
	const char* long_string_wrong_case = "this is a wrong string.";

	// Test _stricmp
	EXPECT_EQ( _stricmp(hello_upper_case, hello_lower_case), 0);
	EXPECT_GT( _stricmp(hello_upper_case, hello_wrong), 0);
	EXPECT_GT( _stricmp(long_string_upper_case, long_string_lower_case), 0);
	EXPECT_LT( _stricmp(long_string_upper_case, long_string_wrong_case), 0);

	// Test _strnicmp
	EXPECT_EQ( _strnicmp(hello_upper_case, hello_lower_case, 5), 0);
	EXPECT_GT( _strnicmp(hello_upper_case, hello_wrong, 4), 0);
	EXPECT_EQ( _strnicmp(hello_upper_case, hello_wrong, 1), 0);
	EXPECT_GT( _strnicmp(long_string_upper_case, long_string_lower_case, 22), 0);
	EXPECT_EQ( _strnicmp(long_string_upper_case, long_string_lower_case, 20), 0);
	EXPECT_LT( _strnicmp(long_string_upper_case, long_string_wrong_case, 22), 0);
	EXPECT_EQ( _strnicmp(long_string_upper_case, long_string_wrong_case, 10), 0);
}