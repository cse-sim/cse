// wcmp.cpp -- simple file compare utility
//  rev 7-31-03 chip

// usage:  wcmp file1 file2 [commentchar]
//    if commentchar, compares by line and ignores chars commentchar -> \n
//    else compares binary
// returned errorlevel 0=same, 1=diff, 2=error

#include <stdio.h>		// NULL EOF
#include <string.h>		// memcmp
#include <io.h>			// open, read, close TODO:be-removed
#include <ctype.h>		// isspace
#include <fcntl.h>		// O_RDONLY
#include <share.h>		// _SH_DENYRW

static int filesAreDiffBinary( int hFile1, int hFile2);
static int filesAreDiffLine( FILE* file1, FILE* file2, int comChar);
static int getLine( char* line, int szLine, FILE* pF, int comChar);
//---------------------------------------------------------------------------
int main( int argc, char **argv)
{
	int ret = 0;
    if (argc <= 2)				// if don't have two filename args
    {  printf( "file compare utility\n"
               "usage:  wcmp file1 file2 [commentchar]\n"
			   "        errorlevel 0=same, 1=diff, 2=error");
       return 2;			// return error
    }

	if (argc == 3)		// no comment char
	{	int hFile1;
		_sopen_s(&hFile1, argv[ 1], O_RDONLY|O_BINARY, _SH_DENYRW, 0);	// open files
		int hFile2;
		_sopen_s(&hFile2, argv[ 2], O_RDONLY|O_BINARY, _SH_DENYRW, 0);
		ret = filesAreDiffBinary( hFile1, hFile2);
	}
	else
	{	char comChar = *argv[ 3];
		FILE* file1;
		fopen_s(&file1, argv[ 1], "r");
		FILE* file2;
		fopen_s(&file2, argv[ 2], "r");
		ret = filesAreDiffLine( file1, file2, comChar);
	}

	return ret;
}		// main
//---------------------------------------------------------------------------
static int filesAreDiffBinary( 	// binary compare file length and contents
	int hFile1, int hFile2)		// file handles
// returns 0 if files are same
//         1 if files are not same
//         2 if error (could not open one/both files, )
{
const int BUFSZ=16384;
char buf1[ BUFSZ], buf2[ BUFSZ];

	if (hFile1 == -1 || hFile2 == -1)
		return 2;
    int areDiff = 1;
    int len1 = _filelength( hFile1);		// get file lengths
    int len2 = _filelength( hFile2);
    if (len1 >= 0 && len2 >= 0 && len1==len2)	// if not error and lengths are same
    {	int szBuf = BUFSZ;
		long cumRead = 0L;
		for ( ; ; )					// compare buffers til difference or done
		{	int nRead1 = _read( hFile1, buf1, szBuf);	// read from each file
			int nRead2 = _read( hFile2, buf2, szBuf);
			if ( nRead1==-1				// if read error, consider different (or issue error message?)
			 ||  nRead1 != nRead2 )			// if read different # bytes from the 2 files, different
				break;
			cumRead += nRead1;			// accumulate total read
			if (!nRead1)  				// if eof, done
			{	if (cumRead==len1)  	// if read the correct number of bytes (else bug; message?)
				   areDiff = 0;			// files are the same!
				break;
			}
			if (memcmp( buf1, buf2, nRead1))		// if buffer loads not identical,
				break;					// files are different
		}
    }
    if (_close( hFile2)==-1)
	   areDiff = 2;	// on close error, consider different (or issue msg?)
    if (_close( hFile1)==-1)
	   areDiff = 2;

    return areDiff;
}			// filesAreDiffBinary
//---------------------------------------------------------------------------
static int filesAreDiffLine(	// compare files line-by-line, hiding commented parts
	FILE* file1, FILE* file2,
	int comChar)		// comment char
						//   -1 = none
						//   else ignore all chars from comChar - \n
// returns 0: files same
//         1: files different
//         2: error (file1 or file2 NULL, )
{
	if (!file1 || !file2)
		return 2;

	char line1[ 1000];
	char line2[ 1000];

	int areDiff = 0;

	while (!areDiff)
	{	int ret1 = getLine( line1, sizeof( line1), file1, comChar);
		int ret2 = getLine( line2, sizeof( line2), file2, comChar);
		if (!ret1 && !ret2)
			break;		// end of both: same
		if (ret1 && ret2)
			areDiff = strcmp( line1, line2) != 0;
		else
			areDiff = 1;
	}

	return areDiff;
}		// filesAreDiffLine
//------------------------------------------------------------------------------
static int getLine(
	char* line,
	int szLine,
	FILE* pF,
	int comChar)
{
	char* p = fgets( line, szLine-1, pF);
	if (!p)
		return 0;

	// truncate at comment char or nl
	for (int i=0; line[ i]; i++)
	{	if (line[ i] == '\n' || line[ i] == comChar)
			line[ i] = '\0';
	}
	return 1;
}		// getLine
//------------------------------------------------------------------------------


// end of wcmp.cpp
