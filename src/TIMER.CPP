// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// timer.cpp:  timing routines for program measurement

#include <CNGLOB.H>
#include <sys/types.h>
#include <sys/timeb.h>
#include <VRPAK.H>		// vrPrintf
#include <TIMER.H>

#pragma optimize( "", off)

// local data
struct TMRDATA
{
	int tcalls;		// # of calls
	LLI tbeg;		// time at beg of current timing, ticks
	LLI ttot;		// total time for timer, ticks
	WStr tname;	// name of timer
	TMRDATA(const char* _tname = "") : tcalls(0), tbeg(0), ttot(0), tname(_tname)
	{}
};
static WVect< TMRDATA> tmrtab;
//------------------------------------------------------------------------------
static double tmrSecsPerTick = -1.;
//-----------------------------------------------------------------------------
static double tmrTicksToSecs(
	LLI ticks)
{	return ticks * tmrSecsPerTick;
}		// tmrTicksToSecs
//==============================================================================
#if defined( TIMING_HIRES)
#error TIMING HIRES no longer supported

// hi-res timing (Windows dependent)

static inline LLI tmrTicks()		// current clock time
{	LARGE_INTEGER t;
	QueryPerformanceCounter( &t);
	return t.QuadPart;
}		// tmrTicks
//-----------------------------------------------------------------------------
static void tmrSetupIf()			// one-time setup
{	if (tmrSecsPerTick < 0.)
	{	LARGE_INTEGER freq;
		if (QueryPerformanceFrequency( &freq))
		{	LLI t = freq.QuadPart;
			if (t > 0)
				tmrSecsPerTick = 1./t;
		}
		if (tmrSecsPerTick < 0.)
		{	err( PWRN,
				"Clock frequency determination failed."
				"\nTiming results are unreliable");
			tmrSecsPerTick = 1./3000000.;
		}
#if 0
x		// determine call overhead as recommended by on-line sources
x       //   result = 0 4-22-2013, did not implement
x		LLI t1 = tmrTicks();
x		LLI t2 = tmrTicks();
x		LLI overhead = t2 - t1;
#endif
	}
}		// tmrSetupIf
//-----------------------------------------------------------------------------
#else
// not TIMING_HIRES
static inline LLI tmrTicks()
{	struct timeb t;
	ftime(&t);
	return t.time*1000 + t.millitm;
}		// tmrTicks
//-----------------------------------------------------------------------------
static void tmrSetupIf()
{	if (tmrSecsPerTick < 0.)
		tmrSecsPerTick = 0.001;
}		// tmrSetupIf
//=============================================================================
#endif		// TIMING_HIRES

/*=============================== TEST CODE ================================*/
/* #define TEST */
#ifdef TEST
t main ()
t{
t SI tb1, tb2, i;
t
t     printf("Start\n");
t     tb1 = 0;
t     tb2 = 1;
t     tmrInit( "First test", tb1);
t     tmrInit( "Second test", tb2);
t     tmrStart(tb1);
t     for (i = 0; i < 1000; i++)
t        sqrt((double)i);
t     tmrStart(tb2);
t     printf("Done first loop\n");
t     for (i = 0; i < 1000; i++)
t        sqrt((double)i);
t     tmrstop(tb1);
t     tmrstop(tb2);
t     printf("Print\n");
t     tmrdisp( stdout, tb1, 1.78);
t     tmrdisp( stdout, tb2, 1.78);
t}
#endif
//========================================================================
void tmrInit( 	// Initialize (but do *NOT* start) a timer

	const char* tmrName,	// Timer name -- 20 char max string to identify timer values on printouts etc.
	int tmr)				// Timer number.  Must be 0 < tmr < TMRMAX
{
	if (size_t( tmr) >= tmrtab.size())
		tmrtab.resize(tmr+5);
	new (&tmrtab[ tmr]) TMRDATA(tmrName);
	tmrSetupIf();		// constants etc if needed
}		// tmrInit
//========================================================================
void tmrStart(			// Start a timer
	int tmr)	// Timer number
{
	tmrtab[tmr].tbeg = tmrTicks();
}		// tmrStart
//========================================================================
void tmrStop(			// Stop a timer
	int tmr ) 	// Timer number + bits
				//  0x1000: do not count this call

// Returns cumulative time (in seconds) on timer
{
	int tn = tmr & 0xfff;
	tmrtab[ tn].ttot += tmrTicks() - tmrtab[tn].tbeg;
	tmrtab[ tn].tbeg = 0LL;
	if (!(tmr & 0x1000))
		tmrtab[tn].tcalls++;
}			// tmrStop
//----------------------------------------------------------------------
double tmrCurTot(			// current total time on timer
	int tmr ) 	// Timer number
// Returns cumulative time (in seconds) on timer
{
	LLI ttot = tmrtab[ tmr].ttot;
	if (tmrtab[ tmr].tbeg > 0LL)
		ttot += tmrTicks() - tmrtab[ tmr].tbeg;
	return tmrTicksToSecs( ttot);
}			// tmrCurTot
#ifdef WANTED	// restore when needed
//========================================================================
void tmrdisp( 		// Display current data for a timer
	FILE *f,	 	// Stream on which to display results.  Must be open
	int tmr,		// Timer number
	double delta )	// Correction time seconds per call subtracted from
				 	// total time.
{
	double nc = (double)tmrtab[tmr].tcalls;
	if (nc==0.)
		nc = 1.;
	double t  = tmrTicksToSecs( tmrtab[tmr].ttot) - nc * delta;
	fprintf( f, "%20s:  Time = %-7.2f  Calls = %-5ld  T/C = %-9.4f\n",
		tmrtab[tmr].tname, t, tmrtab[tmr].tcalls, t/nc );
}			// tmrdisp
#endif
//========================================================================
RC vrTmrDisp( 		// Display current data for a timer

	int vrh,					// open virtual report to which to display results
	int tmr,					// Timer number
	const char* pfx /*=""*/,	// optional line pfx (for e.g. hiding data from test text compare)
	double delta /*=0.*/)		// Correction time in seconds per call subtracted from
								//   total time
{
	if (!pfx)
		pfx = "";		// insurance

	double nc = max( (double)tmrtab[tmr].tcalls, 1.);
	double t  = tmrTicksToSecs( tmrtab[tmr].ttot) - nc * delta;
	return vrPrintf( vrh, "%s%20s:  Time = %-7.2f  Calls = %-5ld  T/C = %-0.4f\n",
					 pfx, tmrtab[tmr].tname.c_str(), t, tmrtab[tmr].tcalls, t/nc );

}			// vrTmrDisp

// end of timer.cpp
