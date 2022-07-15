#include "gtest/gtest.h"

//#include "cnglob.h"
//#include "messages.h"	// MAX_MSGLEN msgI

inline int _stricmp_test(const char* char1, const char* char2);
inline int _strnicmp_test(const char* char1, const char* char2, size_t count);
char* _strupr_test(char* stringMod);
char* _strlwr_test(char* stringMod);

TEST(strpak, compare_case_insensitive_string) 
{
    // Set up strings to compare
    const char* helloUpr = "HELLO";
    const char* helloLwr = "hello";
    const char* helloWrg = "H3llo";
	const char* str1 = "This IS a LONG string.";
	const char* str2 = "this is a long string ";
	const char* str3 = "this is a wrong string.";

    // Test _stricmp
    EXPECT_EQ( _stricmp_test(helloUpr, helloLwr),_stricmp(helloUpr, helloLwr) );
    EXPECT_EQ( _stricmp_test(helloUpr, helloWrg),_stricmp(helloUpr, helloWrg) );
    EXPECT_EQ(_stricmp_test(str1 , str2), _stricmp(str1,str2) );
	EXPECT_EQ(_stricmp_test(str1, str3), _stricmp(str1, str3));

    // Test _strnicmp
    EXPECT_EQ( _strnicmp_test(helloUpr,helloLwr,5), _strnicmp(helloUpr,helloLwr,5) );
	EXPECT_EQ( _strnicmp_test(helloUpr, helloWrg, 4), _strnicmp(helloUpr, helloWrg, 4));
	EXPECT_EQ( _strnicmp_test(helloUpr, helloWrg, 1), _strnicmp(helloUpr, helloWrg, 1));
	EXPECT_EQ( _strnicmp_test(str1, str2, 22), _strnicmp(str1, str2, 22));
	EXPECT_EQ( _strnicmp_test(str1, str3, 22), _strnicmp(str1, str3, 22));
	EXPECT_EQ( _strnicmp_test(str1, str3, 10), _strnicmp(str1, str3, 10));
}

TEST(strpak, convert_case_functions)
{
	// Set up strings
	char uppercaseString1[] = "this should be uppercase.";
	char uppercaseString2[] = "this should be uppercase.";
	char lowercaseString1[] = "THIS SHOULD BE LOWERCASE";
	char lowercaseString2[] = "THIS SHOULD BE LOWERCASE";
	char randomCaseString1[] = "Hello There ARE many case HERE!";
	char randomCaseString2[] = "Hello There ARE many case HERE!";

	// Test convert uppercase strings
	EXPECT_STREQ( _strupr_test(uppercaseString1), _strupr(uppercaseString2));
	EXPECT_STREQ( _strupr_test(randomCaseString1), _strupr(randomCaseString2));
	
	// Test convert lowercase strings
	EXPECT_STREQ( _strlwr_test(lowercaseString1), _strlwr(lowercaseString2));
	EXPECT_STREQ( _strlwr_test(randomCaseString1), _strlwr(randomCaseString2));
}

inline int _stricmp_test(	// Substitude windows _stricmp functions
	const char* char1,	// First string to be compare
	const char* char2)	// Second string to be compare
// Compares two string ignoring case sensitivity
// Eventually replace this function with POSIX standard
{
	int sum{ 0 };
	for (;; char1++, char2++) {
		sum += tolower((unsigned char)*char1) - tolower((unsigned char)*char2);
		if (sum != 0 || *char1 == '\0' || *char2 == '\0') {
			return sum;
		}
	}
} // _stricmp
//-----------------------------------------------------------------------------
inline int _strnicmp_test(			// Substitude windows _strnicmp
	const char* char1,	// First string to be compare
	const char* char2,	// Second string to be compare
	size_t count)		// Number of characters to compare
// Compares two string ignoring case sensitivity upto the count.
// Eventually replace this function with POSIX standard
{
	int sum{ 0 };
	for (size_t i = 0; i < count; i++, char1++, char2++) {
		sum += (tolower((unsigned char)*char1) - tolower((unsigned char)*char2));
		if (sum != 0 || *char1 == '\0' || *char2 == '\0') {
			return sum > 0? 1:-1;
		}
	}
	return 0;
} // _stricmp
//-----------------------------------------------------------------------------
char* _strupr_test(char* stringMod) // Substitude strupr function
// Converts a string to uppercase
{		
	for (int i = 0; stringMod[i] != '\0'; ++i) {
		stringMod[i] = toupper(stringMod[i]);
	}
	return stringMod;
}	// _strupr
//-----------------------------------------------------------------------------
char* _strlwr_test(char* stringMod) // Substitude strlwr function
// Converts a string to lowercase
{
	for (int i = 0; stringMod[i] != '\0'; ++i) {
		stringMod[i] = tolower(stringMod[i]);
	}
	return stringMod;
}	// strlwr
