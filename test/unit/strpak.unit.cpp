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
	char buf[200];
	{	// Replace a single character
		char* originalStr{ strsave("Find end of line\n relace with tab.") };
		int replaceCount = strReplace(originalStr, '\n', '\t');
		EXPECT_EQ(replaceCount, 1);
		EXPECT_STREQ(originalStr, "Find end of line\t relace with tab.");
	}

	{	// Replace multiple character string
		const char* originalStr = "Replace a lot of characters.";
		int replaceCount = strReplace(buf, sizeof( buf), originalStr, "a lot", "lots");
		EXPECT_EQ(replaceCount, 1);
		EXPECT_STREQ(buf, "Replace lots of characters.");
	}

	{	// Replace multiple instances
		const char* originalStr = "Replace all the Es even in eels and words that end in e";
		int replaceCount = strReplace( buf, sizeof( buf), originalStr, "e", "??");
		EXPECT_EQ(replaceCount, 10);
		EXPECT_STREQ(buf, "R??plac?? all th?? ??s ??v??n in ????ls and words that ??nd in ??");
	}

	{	// Instance not found
		const char* originalStr = "Not here";
		int replaceCount = strReplace(buf, sizeof( buf), originalStr, "find line", "and \t");
		EXPECT_EQ(replaceCount, 0);
		EXPECT_STREQ(buf, "Not here");
	}

	{	// Remove substring
		const char* originalStr = "Not here";
		int replaceCount = strReplace(buf, sizeof( buf), originalStr, "Not ", "");
		EXPECT_EQ(replaceCount, 1);
		EXPECT_STREQ(buf, "here");
	}


	{	// buffer space exceeded and case-sensitivity
		const char* originalStr = "Medium sized original Ztring without interesting characteristics.";
		int replaceCount = strReplace(buf, 27, originalStr, "z", "XX", true);
		EXPECT_EQ(replaceCount, -1);
		EXPECT_STREQ(buf, "Medium siXXed original Ztr");
	}
}

#if 1
TEST(strpak, CULSTR_funtions) {
	
	CULSTR us1;

	// "null" string
	EXPECT_EQ(us1.IsNull(), true);
	const char* s1 = us1.c_str();
	EXPECT_STREQ(s1, "");

	const char* sTest = "Testing";
	us1.Set( sTest);
	EXPECT_EQ(us1.IsNull(), false);
	s1 = us1.c_str();
	EXPECT_STREQ(s1, sTest);

	CULSTR us2("A rather longer string");
	CULSTR us3(us1);
	EXPECT_STREQ(us3.c_str(), sTest);

	us2.Set(nullptr);
	CULSTR us4("Here is another string that should go in slot 2");
	EXPECT_EQ(us4.us_hStr, 2);

	EXPECT_STREQ((const char*)us1, sTest);


}
#endif

