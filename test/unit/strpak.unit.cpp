#include "gtest/gtest.h"

#include "cnglob.h"
#include "strpak.h"

TEST(strpak, compare_case_insensitive_string) 
{
	// Set up strings to compare
	const char* helloUpperCase = "HELLO";
	const char* helloLowerCase = "hello";
	const char* helloWrong = "H3llo";
	const char* longStringUpperCase = "This IS a LONG string.";
	const char* longStringLowerCase = "this is a long string ";
	const char* longStringWrongCase = "this is a wrong string.";

	// Test _stricmp
	EXPECT_EQ( _stricmp(helloUpperCase, helloLowerCase), 0);
	EXPECT_EQ( _stricmp(helloUpperCase, helloWrong), 50);
	EXPECT_EQ(_stricmp(longStringUpperCase,longStringLowerCase), 14);
	EXPECT_EQ( _stricmp(longStringUpperCase, longStringWrongCase), -11);

	// Test _strnicmp
	EXPECT_EQ( _strnicmp(helloUpperCase, helloLowerCase, 5), 0);
	EXPECT_EQ( _strnicmp(helloUpperCase, helloWrong, 4), 1);
	EXPECT_EQ( _strnicmp(helloUpperCase, helloWrong, 1), 0);
	EXPECT_EQ( _strnicmp(longStringUpperCase, longStringLowerCase, 22), 1);
	EXPECT_EQ( _strnicmp(longStringUpperCase, longStringLowerCase, 20), 0);
	EXPECT_EQ( _strnicmp(longStringUpperCase, longStringWrongCase, 22), -1);
	EXPECT_EQ( _strnicmp(longStringUpperCase, longStringWrongCase, 10), 0);
}
