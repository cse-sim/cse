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

TEST(strpak, convert_case_functions)
{
	// Set up strings
	char upper_case_string[] = "this should be uppercase.";
	char mixed_case_string[] = "Hello There ARE maNy cases HERE!";
	char number_string[] = "Testing numbers: ASHRAE205, 1252345 (#).";

	// Test convert uppercase strings
	EXPECT_STREQ( _strupr(upper_case_string), "THIS SHOULD BE UPPERCASE.");
	EXPECT_STREQ( _strupr(mixed_case_string), "HELLO THERE ARE MANY CASES HERE!");
	EXPECT_STREQ( _strupr(number_string), "TESTING NUMBERS: ASHRAE205, 1252345 (#).");

	
	// Test convert lowercase strings
	char lower_case_string[] = "THIS SHOULD BE LOWERCASE.";
	char mixed_case_string_lower_test[] = "Hello There ARE maNy cases HERE!";
	char number_string_lower_test[] = "Testing numbers: ASHRAE205, 1252345 (#).";
	EXPECT_STREQ( _strlwr(lower_case_string), "this should be lowercase.");
	EXPECT_STREQ( _strlwr(mixed_case_string_lower_test), "hello there are many cases here!");
	EXPECT_STREQ( _strlwr(number_string_lower_test), "testing numbers: ashrae205, 1252345 (#).");
}
TEST(strpak, find_and_replace_functions) {
	{	// Replace a single character
		char* originalStr{ strsave("Find end of line\n relace with tab.") };
		int replaceCount = strReplace(originalStr, "\n", "\t");
		EXPECT_EQ(replaceCount, 1);
		EXPECT_STREQ(originalStr, "Find end of line\t relace with tab.");
	}

	{	// Replace multiple character string
		char* originalStr{ strsave("Replace a lot of characters.")};
		int replaceCount = strReplace(originalStr, "a lot", "lots");
		EXPECT_EQ(replaceCount, 1);
		EXPECT_STREQ(originalStr, "Replace lots of characters.");
	}

	{	// Replace multiple instances
		char* originalStr{ strsave("Replace all the es") };
		int replaceCount = strReplace(originalStr, "e", "a");
		EXPECT_EQ(replaceCount, 4);
		EXPECT_STREQ(originalStr, "Raplaca all tha as");
	}

	{	// Instance not found
		char* originalStr{ strsave("Not here") };
		int replaceCount = strReplace(originalStr, "find line", "and \t");
		EXPECT_EQ(replaceCount, 0);
		EXPECT_STREQ(originalStr, "Not here");
	}

	{	// Remove substring
		char* originalStr{ strsave("Not here") };
		int replaceCount = strReplace(originalStr, "Not ", "");
		EXPECT_EQ(replaceCount, 1);
		EXPECT_STREQ(originalStr, "here");
	}
}
