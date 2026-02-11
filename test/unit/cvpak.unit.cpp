#include <limits>

#include "gtest/gtest.h"

#include "cnglob.h"
#include "cvpak.h"

TEST(cvpak, output_convert) 
{

// float tests -- special cases (NANDLES)
	NANDAT unset{ UNSET };
	const char* str = cvin2s(&unset, DTFLOAT, UNNONE, 10, 0, 0);
	EXPECT_STREQ(str, "<unset>");

	NANDAT expr13{ NANDLE(13) };
	str = cvin2s(&expr13, DTFLOAT, UNNONE, 10, 0, 0);
	EXPECT_STREQ(str, "<expr 13>");

	auto x{ NCHOICE(3 | NCNAN) };
	NANDAT chn3{ AsNANDAT(x) };
	// NANDAT chn3{ AsNANDAT(NCHOICE(3 | NCNAN) };
	str = cvin2s(&chn3, DTFLOAT, UNNONE, 6, 0, 0);
	EXPECT_STREQ(str, "?     ");

// float tests -- numeric values
	struct FVTOS
	{
		float fV;
		SI units;
		int mfw;
		int fmt;
		int xfw;
		const char* exp;
	};

        float nanf = std::numeric_limits<float>::infinity()/std::numeric_limits<float>::infinity();
        
	FVTOS fvt[] = 
	{ { 5.f,       UNNONE, 10, FMTLJ, 0, "5         " },
#if 0
	  // units don't work due to imcomplete test environment
	  { 22.3f,    UNENERGYDEN, 10, FMTLJ+2, 0, "22.30 Btu/ft2" },
#endif
	  { 5.f,       UNNONE, 10, FMTRJ, 0, "         5" },
	  { 5000000.f, UNNONE, 6, FMTLJ, 0, "5000 k" },
	  { -11.3700f, UNNONE, 10, (FMTSQ | FMTRTZ) + 4, 0, "-11.37" },
          { nanf, UNNONE, 10, (FMTSQ | FMTRTZ) + 4, 0, "nan" },
          { nanf, UNLENGTH, 10, (FMTSQ | FMTRTZ) + 4, 0, "nan" },
          { 1.5f, UNLENGTH, 10, (FMTSQ | FMTRTZ) + 6, 0, "1'6\"" },
          { 1.55f, UNLENGTH, 10, (FMTSQ | FMTRTZ) + 6, 0, "1'6.6\"" },
          { 1.0f, UNLENGTH, 10, (FMTSQ | FMTRTZ) + 6, 0, "1'0\"" },
          { 1.5f, UNLENGTH, 10, (FMTRTZ) + 6, 0, "1' 6\"     " },
	  };

	for (FVTOS& fv : fvt)
	{
		const char* str = cvin2s(&fv.fV, DTFLOAT, fv.units, fv.mfw, fv.fmt, fv.xfw);
		EXPECT_STREQ(str, fv.exp);
	}

        // double tests -- numeric values
        struct DVTOS
        {
          double dV;
          SI units;
          int mfw;
          int fmt;
          int xfw;
          const char* exp;
        };
        
        double nand = std::numeric_limits<double>::infinity()/std::numeric_limits<double>::infinity();
        
        DVTOS dvt[] =
        {
            { nand,  UNENERGY1, 10, FMTSQ+FMTUNITS+4, 0, "nan kBtu" }
        };
        
        for (DVTOS& dv : dvt)
        {
          const char* str = cvin2s(&dv.dV, DTDBL, dv.units, dv.mfw, dv.fmt, dv.xfw);
          EXPECT_STREQ(str, dv.exp);
        }
        
#if 0

		const void* data,	// Pointer to data in internal form, or NULL to do nothing and return NULL
							//  (for DTCHP, is ptr to ptr to string to print, 11-91) */
		USI dt, 		// Data type of internal data, or DTNA for "--" or DTUNDEF for "?" from cvfddisp()
		SI units,		// Units of internal data (made signed 5-89)
		USI _mfw,		// Maximum field width (not including '\0').  If requested format results in string longer
						// than mfw, format will be altered if possible to give max significance within mfw;
						// when not possible (never possible for inteters or if field too narrow for e or k format),
						// field of **** is returned (but see _xfw).
		USI _fmt,		// Format.  See cvpak.h for definition of fields.
		USI _xfw /*=0*/)	/* 0 or extra field width available: if value cannot be formatted in _mfw columns does fit
					   in _mfw + _xfw or fewer cols, return the overlong text rather than ******.  added by rob, 4-92. */

					   // Returns pointer to result in Tmpstr.
					   // Also sets global Cvnchars to the number of characters placed in str (not incl. '\0').
#endif

}
