// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// msghans.h -- message handle definitions for msgtab.cpp/messages.cpp

// STORY
//   Message handles are 16 bit integers which identify message strings
//   through association in msgTbl[] entries.  They allow retrieval of
//   message text via fcns in messages.cpp.
//   They are passed to error reporting fcns in errhan.cpp, also some strpak.cpp fcns 3-92.

//   TYPES: cnglob.h typedefs MH which is typically used to hold msgHan values.

//   NAMES: msgHan naming conventions are evolving
//      a.  Externally visible error messages: MH_I1234 should be used for
//          error message I1234

//   VALUES: msgHan values should be in the range 1 - 16384 which matches a reserved range
//           in the value conventions used by RCs (Return Codes, see cnglob.h)


#define MHOK	0	// must be 0 to match RCOK; associated with "" msg

// --- Control and usage messages ---
// usage errors, cse.cpp
#define MH_C0001  1001	// "usage ... ... ... "
#define MH_C0002  1002	// "Too many non-switch arguments on command line: %s ..."
#define MH_C0003  1003	// "No input file name given on command line"
#define MH_C0004  1004	// "Command line error(s) prevent execution"
#define MH_C0005  1005	// "Switch letter required after '%c'"
#define MH_C0006  1006	// "Unrecognized switch '%c%c'"
#define MH_C0007  1007	// "Unexpected cul() preliminary initialization error"
#define MH_C0008  1008	// "Warning: C0008: \"%s\":\n    it is clearer to always place all switches before file name"  2-95 .454
// additions 6-95 .462
#define MH_C0009  1009	// "Can't use switch -%c unless calling program\n""    gives \"hans\" argument for returning memory handles"
#define MH_C0010  1010	// "Switch -%c not available in DOS version"
#define MH_C0011  1011	// "\"%s\": \n    input file name cannot contain wild cards '?' and '*'."
//#define MH_C0012  1012	//
// timing info, cse.cpp, 6-95, .462.
#if 0
x#define MH_T0001  1081	// "\n\n%s %s %s run(s) done: %s"
x#define MH_T0002  1082	// "\n\nInput file name:  %s"
x#define MH_T0003  1083	// "\nReport file name: %s"
x//#define MH_T0004  1084	//
#endif	// 3-24-10

// cnguts.cpp
#define MH_C0100  1100	// " *** Interrupted *** "
#define MH_C0101  1101	/* "For GAIN '%s': More than 100 percent of gnPower distributed:\n"
			   "    Total of fractions gnFrZn, gnFrPl, and gnFrRtn exceeds 1.0:\n"
			   "        gnFrZn (%g) + gnFrPl (%g) + gnFrRtn (%g) = %g" */
//#define MC_C0102  1102
//1200+ used for vrpak run errors below

//cnztu, cnah, cncoil, cnhp ...   1300...  are below.

// --- Internal errors ---
// (many coded in line errCrit calls, see msgtbl.cpp)
// ratpak.cpp --  OBSOLESCENT, review
#define MH_X0040  40
#define MH_X0041  41
#define MH_X0042  42
#define MH_X0043  43
#define MH_X0044  44
#define MH_X0045  45
#define MH_X0046  46
#define MH_X0047  47
#define MH_X0048  48
//#define MH_X0048  49
// ancrec.cpp, added 5-14-92
#define MH_X0050  50	// "record::CopyFrom: unconstructed destination (b is 0)"
#define MH_X0051  51	// "record::operator=(): unconstructed destination (b is 0)"
#define MH_X0052  52	// "record::operator=(): records not same type"
#define MH_X0053  53	// "anc4n: bad or unassigned record anchor number %d"
#define MH_X0054  54	// "%s() called for NULL object pointer 'this'"
#define MH_X0055  55	// "%s() argument not a valid anchor"
#define MH_X0056  56	// "%s(): illegal use of static-storage anchor"
#define MH_X0057  57	// "basAnc constructor: non-near anc at %p\n"
//#define MH_X0058  58	//
//#define MH_X0059  59	//

// --- Input language errors 3-92 ---

		//4000-4012  used below for cuprobe.cpp msgs.
// pp.cpp 5-92
#define MH_P0001  5001	// "ppOpen(): cn ul input file already open"
#define MH_P0002  5002	// "Cannot open '%s': already at maximum \n    include nesting depth"
#define MH_P0003  5003	// "Cannot open file '%s'"
#define MH_P0004  5004	// "Close error, file '%s'"
#define MH_P0005  5005	// "ppClI: bad is->ty"
#define MH_P0006  5006	// "Overlong preprocessor command truncated"
#define MH_P0007  5007	// " Internal error: ppC: read off the end of buffer "
#define MH_P0008  5008	// "ppC intrnl err: buf space not avail for / and * "
#define MH_P0009  5009	// "'(' expected after macro name '%s'"
#define MH_P0010  5010	// "Argument list to macro '%s' far too long: \n    ')' probably missing or misplaced"
#define MH_P0011  5011	// "comma or ')' expected"
#define MH_P0012  5012	// "Too many arguments to macro '%s'"
#define MH_P0013  5013	// "Too few arguments to macro '%s'"
#define MH_P0014  5014	// "Overlong argument to macro '%s' truncated"
#define MH_P0015  5015	// "newline in quoted text"
#define MH_P0016  5016	// "Unexpected end of file in macro reference"
#define MH_P0017  5017	// "macArgCi internal error: is->i >= is->j"
#define MH_P0018  5018	// "ppmScan(): garbage .inCmt %d"
#define MH_P0019  5019	// "Comment within comment"
#define MH_P0020  5020	// "Unexpected end of file or macro in comment"
#define MH_P0021  5021	// "Too many nested macros and #includes"
#define MH_P0022  5022	// "pp.cpp:lisFind: matching buffer line is %d but requested line is %d\n"
#define MH_P0024  5024	// "Replacing illegal null character ('\\0') with space\n    in or after input file line number shown."	//10-92
//pp.cpp additions 6-95
#define MH_P0025  5025	// "'=' expected"
#define MH_P0026  5026	// "Unexpected stuff after -U<identifier>"
//#define MH_P0027  5027	//
//#define MH_P0028  5028	//
//#define MH_P0029  5029	//

//ppcmd.cpp 5-14-92
#define MH_P0030  5030	// "Invalid preprocessor command:    word required immediately after '#'"
#define MH_P0031  5031	// "Unrecognized preprocessor command '%s'"
#define MH_P0032  5032	// "#else not preceded by #if"
#define MH_P0033  5033	// "More than one #else for same #if (#if is at line %d of file %s)"
#define MH_P0034  5034	// "#endif not preceded by #if"
#define MH_P0035  5035	// "Too many nested #if's, skipping to #endif"
#define MH_P0036  5036	// "#elif not preceded by #if"
#define MH_P0037  5037	// "#elif after #else (for #if at line %d of file %s )"
#define MH_P0038  5038	// "'<' or '\"' expected"
#define MH_P0039  5039	// "Internal error in ppcDoI: bad 'ppCase'"
#define MH_P0040  5040	// "Unexpected stuff after preprocessor command"
#define MH_P0041  5041	// "Comma or ')' expected"
#define MH_P0042  5042	// "Overlong #define truncated"
#define MH_P0043  5043	// "Warning: unbalanced quotes in #define"
#define MH_P0044  5044	// "'#endif' corresponding to #if at line %d of file %s not found"
#define MH_P0045  5045	// "Closing '%c' not found"
#define MH_P0046  5046	// "Redefinition of '%s'"
//#define MH_P0047  5047	//
//#define MH_P0048  5048	//
//#define MH_P0049  5049	//

// ppcex.cpp 5-14-92
#define MH_P0050  5050	// "ppcex.cpp:cex() Internal error:\n    parse stack level %d not 1 after constant expression"
#define MH_P0051  5051	// "Syntax error: probably operator missing before '%s'"
#define MH_P0052  5052	// "'%s' expected"
#define MH_P0053  5053	// "Unexpected '%s'"
#define MH_P0054  5054	// "Unrecognized opTbl .cs %d for token='%s' ppPrec=%d, ppTokety=%d."
#define MH_P0055  5055	// "Unrecognized ppTokety %d for token='%s' ppPrec=%d."
#define MH_P0056  5056	// "Preceding constant integer value expected before %s"
#define MH_P0057  5057	// "Internal error: parse stack underflow"
#define MH_P0058  5058	// "Value expected after %s"
#define MH_P0059  5059	// "ppcex.cpp:ppUnOp() intrnl error: unrecognized 'opSi' value %d"
#define MH_P0060  5060	// "Preceding constant integer value expected before %s"
#define MH_P0061  5061	// "ppcex.cpp:ppBiOp() internal error: unrecognized 'opSi' value %d"
#define MH_P0062  5062	// "Preprocessor parse stack overflow error: \n    preprocessor constant expression probably too complex"
//             5063-5069

// pptok.cpp 5-92
#define MH_P0070  5070	// "Number too large, truncated"
#define MH_P0071  5071	// "Ignoring illegal char 0x%x"
#define MH_P0072  5072	// "Identifier expected"
#define MH_P0073  5073	// " Internal warning: possible bug detected in pp.cpp:ppCNdc(): \n"
			// "    Character gotten with decomment, ungotten, \n"
			// "    then gotten again without decomment. "
// 		5074-5079

// sytb.cpp 5-92
#define MH_P0080  5080	// "sytb.cpp: bad call to syAdd()"
#define MH_P0081  5081	// "sytb.cpp:syAdd(): tokTy 0x%x has disallowed hi bits"
#define MH_P0082  5082	// "sytb.cpp:syAdd(): Adding symbol table entry '%s' (%d) \n    that duplicates '%s' (%d)"
#define MH_P0083  5083	// "sytb.cpp:syDel(): symbol '%s' not found in table"
#define MH_P0084  5084	// "sytb.cpp:syDel(): bad call: token type / casi / nearId is 0x%x not 0x%x"
//		5085-5089

// exman.cpp input time errors.  better prefix?  5-92
#define MH_E0090  5090	// "exman.cpp:exPile: Internal error: bad table entry: \n    bad evfOk 0x%x for 16-bit quantity"
#define MH_E0091  5091	// "exman.cpp:exPile: Internal error: \n    called with TYCH wanTy 0x%x but non-choice choiDt 0x%x"
#define MH_E0092  5092	// "exman.cpp:exPile: Internal error: \n    called with TYNC wanTy 0x%x but 2-byte choiDt 0x%x"
#define MH_E0093  5093	// "exman.cpp:exPile: Internal error: \n    4-byte choice value returned by exOrk for 2-byte type"
#define MH_E0094  5094	// "exman.cpp:expile: fdTy > 255, change UCH to USI"
#define MH_E0095  5095	// "Bad %s: %s: %s"
#define MH_E0096  5096	// "exman.cpp:exWalkRecs: os[] overflow"
#define MH_E0097  5097	// "exman.cpp:exWalkRecs(): unset data for %s"
#define MH_E0098  5098	// "exman.cpp:exReg(): Unset data for %s"
//                 5099
// exman.cpp run time errors.  better prefix?  5-92
#define MH_E0100  5100	// "exman.cpp:exEvEvf(): exTab=NULL but exN=%d"
#define MH_E0101  5101	// "Unset data or unevaluated expression error(s) have occurred.\n    Terminating run."
#define MH_E0102  5102	// "More than %d errors.  Terminating run."
#define MH_E0103  5103	// "exman.cpp:exEv(): expr %d has NULL ip"
#define MH_E0104  5104	// "While determining %s: "
#define MH_E0105  5105	// "Bad value, %s, for %s: %s"
#define MH_E0106  5106	// "exman.cpp:pRecRef(): record subscript out of range"
#define MH_E0107  5107	// "exman.cpp:txVal: unexpected 'ty' 0x%x"
#define MH_E0108  5108	// "<bad rat #: %d>"		// 10-92
#define MH_E0109  5109	// "member at offset %d"	// 10-92
#define MH_E0110  5110	// "'%s' (subscript [%d])"	// 10-92
/*
#define MH_E0111  5111	//
#define MH_E0112  5112	//
#define MH_E0113  5113	//
#define MH_E0114  5114	//
#define MH_E0115  5115	//
#define MH_E0116  5116	//
#define MH_E0117  5117	//
#define MH_E0118  5118	//
#define MH_E0119  5119	//
*/

// mostly syntax errors -- cuparse.cpp 5-92
#define MH_S0001  6001	// "Syntax error.  'FUNCTION' or 'VARIABLE' expected"
#define MH_S0002  6002	// "%s '%s' cannot be used as function name"
#define MH_S0003  6003	// "Expected identifier (name for function)"
#define MH_S0004  6004	// "'(' required. \n    If function has no arguments, empty () must nevertheless be present."
#define MH_S0005  6005	// "Syntax error: '(' expected"
#define MH_S0006  6006	// "Too many arguments"
#define MH_S0007  6007	// "Syntax error: expected a type such as 'FLOAT' or 'INTEGER'"
#define MH_S0008  6008	// "%s '%s' cannot be used as argument name"
#define MH_S0009  6009	// "Expected identifier (name for argument)"
#define MH_S0010  6010	// "Syntax error: ',' or ')' expected"
#define MH_S0011  6011	// "'=' expected"
#define MH_S0012  6012	// "cuparse:expTy: bad return type 0x%x (%s) from expr"
#define MH_S0013  6013	// "%s expected%s%s"
#define MH_S0014  6014	// "Syntax error: probably operator missing before '%s'"
#define MH_S0015  6015	// "'%s' expected"
#define MH_S0016  6016	// "Internal error in cuparse.cpp:expr: TYLLI not suppored in cuparse.cpp"
#define MH_S0017  6017	// "Unexpected '%s'"
#define MH_S0018  6018	// "Internal error in cuparse.cpp:expr: Unrecognized opTbl .cs %d\n    for token='%s' prec=%d, tokTy=%d."
#define MH_S0019  6019	// "Internal error in cuparse.cpp:expr: \n    Unrecognized tokTy %d for token='%s' prec=%d."
#define MH_S0020  6020	// "%s\n    (OR mispelled word or omitted quotes)"
#define MH_S0021  6021	// ":\n    perhaps you intended '%s'."
#define MH_S0022  6022	// "Misspelled word or undeclared function or variable name '%s'%s"
#define MH_S0023  6023	// "Value expected before ','"
#define MH_S0024  6024	// "Internal error in cuparse.cpp:expr: parse stack underflow"
#define MH_S0025  6025	// "Value varies at end of %s \n    where only start-of-interval variation permitted%s.\n    You may need to use a .prior in place of a .res"
#define MH_S0026  6026	// "at most %s variation"
#define MH_S0027  6027	// "%s variation not"
#define MH_S0028  6028	// "Statement expected%s"
#define MH_S0029  6029	// "Value expected %s%s"
#define MH_S0030  6030	// "Internal error: cuparse.cpp:expr produced %d not 1 parStk frames"
#define MH_S0031  6031	// "Preceding %s expected"
#define MH_S0032  6032	// "%s expected%s"
#define MH_S0033  6033	// "Incompatible expressions before and after ':' -- \n    cannot combine '%s' and '%s'"
#define MH_S0034  6034	// "'(' missing after built-in function name '%s'"
#define MH_S0035  6035	// "Built-in function '%s' may not be assigned a value"
#define MH_S0036  6036	// "Nested assignment should be enclosed in ()'s to avoid ambiguity" /* CAUTION 3 uses */
#define MH_S0037  6037	// "Built-in function '%s' may only be used left of '=' --\n    it can be assigned a value but does not have a value"
#define MH_S0038  6038	// "'(' missing after built-in function name '%s'"
#define MH_S0039  6039	// "',' expected"  /* CAUTION multiple uses */
#define MH_S0040  6040	// "Built-in function '%s' may not be assigned a value"
#define MH_S0041  6041	// "hourval() requires values for 24 hours"
#define MH_S0042  6042	// "hourval() arguments in excess of 24 will be ignored"
#define MH_S0043  6043	// "All conditions in '%s' call false and no default given"
#define MH_S0044  6044	// "Index out of range and no default: \n    argument 1 to '%s' is %d, not in range %d to %d"
#define MH_S0045  6045	// "',' expected. '%s' arguments must be in pairs."
#define MH_S0046  6046	// "Syntax error: expected '%s'"
#define MH_S0047  6047	// "Incompatible arguments%s to '%s':\n    can't mix '%s' and '%s'"
#define MH_S0048  6048	// "Internal error in cuparse.cpp:fcnArgs:\n    bad fcn table entry for '%s': VC, LOK, ROK all on"
#define MH_S0049  6049	// "Too few arguments to built-in function '%s': %d not %d%s"
#define MH_S0050  6050	// "Too many arguments to built-in function '%s': %d not %d"
#define MH_S0051  6051	// "Internal error in cuparse.cpp:emiFcn: bad function table entry for '%s'"
#define MH_S0052  6052	// "System variable '%s' may not be assigned a value"
#define MH_S0053  6053	// "System variable '%s' may only be used left of '=' --\n    it can be assigned a value but does not have a value"
#define MH_S0054  6054	// "'(' expected: functions '%s' takes no arguments\n    but nevertheless requires empty '()'"
#define MH_S0055  6055	// "'(' expected after function name '%s'"
#define MH_S0056  6056	// "')' expected"
#define MH_S0057  6057	// "Too %s arguments to '%s': %d not %d"
#define MH_S0058  6058	// "Internal error: bug in cuparse.cpp:dumVar switch"
#define MH_S0059  6059	// "User variables not implemented"
#define MH_S0060  6060	// "Integer value required%s"
#define MH_S0061  6061	// "Numeric expression required%s"
#define MH_S0062  6062	// "Internal error in cuparse.cpp:cleanEvf: Unexpected 'EVFDUMMY' flag"
#define MH_S0063  6063	// "Expected numeric value, found choice value"
#define MH_S0064  6064	// "Numeric value required"
#define MH_S0065  6065	// "%s\n    (or perhaps invalid mixing of string and numeric values)"
#define MH_S0066  6066	// "Internal error in cuparse.cpp:cnvPrevSf: expressions not consecutive"
#define MH_S0067  6067	// "Internal error in cuparse.cpp:cnvPrevSf:\n    pspe+.. %p != psp2 %p"
#define MH_S0068  6068	// "Parse stack overflow error: expression probably too complex"
#define MH_S0069  6069	// "Internal Error in cuparse.cpp:combSf: expressions not consecutive"
#define MH_S0070  6070	// "Internal error: bad call to cuparse.cpp:dropSfs( %d, %d)"
#define MH_S0071  6071	// "confusion (1) in dropSfs"
#define MH_S0072  6072	// "confusion (2) in dropSfs"
#define MH_S0073  6073	// "Internal error in cuparse.cpp:movPsLast: expression not last in ps."
#define MH_S0074  6074	// "Internal error in cuparse.cpp:movPsLast: expressions not consecutive"
#define MH_S0075  6075	// "Internal error in cuparse.cpp:fillJmp: address place holder not found in Jump"
#define MH_S0076  6076	// "confusion in dropJmpIf"
#define MH_S0077  6077	// "Internal Error in cuparse.cpp:emiKon:\n    no recognized bit in choiDt value 0x%x"
#define MH_S0078  6078	// "Bug in cuparse.cpp:emiKon switch"
#define MH_S0079  6079	// "Bug in cuparse.cpp:emiLod switch"
#define MH_S0080  6080	// "Bug in cuparse.cpp:emiSto switch"
#define MH_S0081  6081	// "Bug in cuparse.cpp:emiDup switch"
#define MH_S0082  6082	// "Preceding code has no effect"
#define MH_S0083  6083	// "Bug in cuparse.cpp:emiPop switch"
#define MH_S0084  6084	// "Compiled code buffer full, ceasing compilation"
// 10-92..        6084-6086  reserve for short texts that could go on disk in functions: before, after, asArg.
#define MH_S0085  6085	// "no value"
#define MH_S0086  6086	// "complete statement"
#define MH_S0087  6087	// "value (any type)"
#define MH_S0088  6088	// "numeric value"
#define MH_S0089  6089	// "limited-long-integer value"
#define MH_S0090  6090	// "integer value"
#define MH_S0091  6091	// "float value"
#define MH_S0092  6092	// "name or string value"
#define MH_S0093  6093	// "string value"
#define MH_S0094  6094	// "integer or string value"
#define MH_S0095  6095	// "name or string or integer value"
#define MH_S0096  6096	// "numeric or string value"
#define MH_S0097  6097	// "choice value"
#define MH_S0098  6098	// "numeric or choice value"
#define MH_S0099  6099	// "<unexpected 0 data type in 'datyTx' call>"
#define MH_S0100  6100	// "<unrecog data type in 'datyTx' call>"
                //6101,6102 available
#define MH_S0103  6103	// "(cuparse.cpp:evfTx(): bad cleanEvf value 0x%x)"
#define MH_S0104  6104	// "each subhour"
#define MH_S0105  6105	// "each hour"
#define MH_S0106  6106	// "each day"
#define MH_S0107  6107	// "each hour on 1st day of month/run"
#define MH_S0108  6108	// "each month"
#if 0
x #define MH_S0109  6109	// "run"
#else // attempt to clarify, 11-95
 #define MH_S0109  6109	// "run (of each phase, autoSize or simulate)"
#endif
#define MH_S0110  6110	// "input time"
#define MH_S0111  6111	// "(no change)"
#define MH_S0112  6112	// "subhourly"
#define MH_S0113  6113	// "hourly"
#define MH_S0114  6114	// "daily"
#define MH_S0115  6115	// "monthly-hourly"
#define MH_S0116  6116	// "monthly"
#if 0
x #define MH_S0117  6117	// "at run start"
x #define MH_S0118  6118	// "run start time"
#else // attempt to clarify, 11-95
 #define MH_S0117  6117	// "at start of run (of each phase, autoSize or simulate)"
 #define MH_S0118  6118	// "run start time (of each phase, autoSize or simulate)"
#endif
#define MH_S0119  6119	// "at input time"
#define MH_S0120  6120	// "input time"
#define MH_S0121  6121	// "never"
#define MH_S0122  6122	// "no"
// Import() 2-94
#define MH_S0123  6123	// "%s argument %d must be constant,\n    but given value varies %s"  2 uses re Import()
#define MH_S0124  6124	// "Import field number %d not in range 1 to %d"
//added 6-95: TERMINOLOGY??
#define MH_S0125  6125	// "each phase (autosize or simulate)"
#define MH_S0126  6126	// "at start autosize and simulate phases"
#define MH_S0127  6127	// "autosize and simulate phase start time"
#define MH_S0128  6128	// "Value varies %s\n    where %s permitted%s."	// extracted 6-95
                //6129 available
/*
#define MH_S0128  6128	//
#define MH_S0129  6129	//
*/
               // 6130-6149 avail

// mostly token syntax errors -- cutok.cpp 7-92
#define MH_S0150  6150	// "Number too big, truncated"
#define MH_S0151  6151	// "Ignoring illegal char 0x%x"
#define MH_S0152  6152	// "Overlong quoted text truncated: \"%s...\""
#define MH_S0153  6153	// "Overlong token truncated: '%s...'"
#define MH_S0154  6154	// "Unexpected end of file in quoted text"
#define MH_S0155  6155	// "Newline in quoted text"
// #define MH_S0156  6156	//
// #define MH_S0157  6157	//
// #define MH_S0158  6158	//
// #define MH_S0159  6159	//
// #define MH_S0160  6160	//
// #define MH_S0161  6161	//
// #define MH_S0162  6162	//
// #define MH_S0163  6163	//
// #define MH_S0164  6164	//
// #define MH_S0165  6165	//
// #define MH_S0166  6166	//
// #define MH_S0167  6167	//
// #define MH_S0168  6168	//
// #define MH_S0169  6169	//

// input language general errors -- cul.cpp -- 6-92
#define MH_S0200  6200	// "cul.cpp:clearRat internal error: bad tables: RATE entry without CULT ptr"
#define MH_S0201  6201	// "cul.cpp:rateDuper(): move or delete request without CULT:\n    can't adjust ownTi in owned RAT entries"
#define MH_S0202  6202	// "cul.cpp:ratCultO: internal error: bad tables: \n    '%s' rat at %p is 'owned' by '%s' rat at %p and also '%s' rat at %p"
#define MH_S0203  6203	// "cul.cpp:drefRes() internal error: bad %s RAT index %d"
#define MH_S0204  6204	// "cul(): NULL RAT entry arg (e)"
#define MH_S0205  6205	// "cul(): bad RAT entry arg (e)"
#define MH_S0206  6206	// "cul.cpp:cul(): Unexpected culComp return value %d"
#define MH_S0207  6207	// "More than %d errors.  Terminating session."
#define MH_S0208  6208	// "'RUN' apparently missing"
#define MH_S0209  6209	// "No commands in file"
#define MH_S0210  6210	// "cul.cpp:cul() internal error: no statements in file since 'RUN',\n    yet cul() recalled"
#define MH_S0211  6211	// "'$EOF' not found at end file"
#define MH_S0212  6212	// "Syntax error.  Expecting a word: class name, member name, or verb."
#define MH_S0213  6213	// "culDo() Internal error: unrecognized 'cs' %d"
#define MH_S0214  6214	// "No run due to error(s) above"
#define MH_S0215  6215	// "Context stack overflow: too much nesting in command tables. \n    Increase size of xStk[] in cul.cpp."
#define MH_S0216  6216	// "Nothing to end (use $EOF at top level to terminate file)"
#define MH_S0217  6217	// "%s name mismatch: '%s' vs '%s'"
#define MH_S0218  6218	// "Cannot DELETE '%s's"
#define MH_S0219  6219	// "Can't define a '%s' within a DEFTYPE"
#define MH_S0220  6220	// "cul.cpp:culRATE Internal error: bad table: RATE entry contains fcn not cult ptr"
#define MH_S0221  6221	// "%s type name required"
#define MH_S0222  6222	// "%s name required"
#define MH_S0223  6223	// "cannot define TYPEs for '%s'"
#define MH_S0224  6224	// "cannot ALTER '%s's"
#define MH_S0225  6225	// "'%s' not permitted with '%s'"	/* USED TWICE */
#define MH_S0226  6226	// "Can't use LIKE or COPY with USETYPE in same %s"
#define MH_S0227  6227	// "duplicate %s name '%s' in "
#define MH_S0228  6228	// "duplicate %s%s name '%s'"
#define MH_S0229  6229	// "%s type '%s' not found"
#define MH_S0230  6230	// "cul.cpp:clearRat Internal error: Too much nesting in tables: \n    Increase size of context stack xStk[] in cul.cpp."
#define MH_S0231  6231	// "'=' expected"
#define MH_S0232  6232	// "Invalid duplicate '%s =' or 'AUTOSIZE %s' command"
#define MH_S0233  6233	// "'%s' cannot be specified here -- previously frozen"
#define MH_S0234  6234	// "cul.cpp:culDAT: Bad .ty %d in CULT for '%s' at %p"
#define MH_S0235  6235	// "Value '%s' for '%s' not one of %s"
#define MH_S0236  6236	// "Statement says %s '%s', but it is \n    in statement group for %s."
#define MH_S0237  6237	// "Can't give more than %d values for '%s'"
#define MH_S0238  6238	// "Internal error: cul.cpp:datPt(): bad CULT table entry: flags incompatible with ARRAY flag"
#define MH_S0239  6239	// "Internal error: cul.cpp:datPt(): bad CULT table entry: ARRAY flag, but 0 size in .p2"
#define MH_S0240  6240	// "Internal error: cul.cpp:datPt(): bad CULT table entry: ARRAY size 0x%lx in p2 is too big"
#define MH_S0241  6241	// "cul.cpp:datPt(): Unrecognized CULT.ty %d in entry '%s' at %p"
#define MH_S0242  6242	// "cul.cpp:datPt(): Bad ty (0x%x) / dtype (0x%x) combination in entry '%s' at %p"
#define MH_S0243  6243	// "Internal error: cul.cpp:datPt(): data size inconsistency: size of cul type 0x%x is %d, but size of field data type 0x%x is %d"
#define MH_S0244  6244	// "Ambiguity in tables: '%s' is used in '%s' and also '%s'"
#define MH_S0245  6245	// "cul.cpp:finalClear() called with xSp %p not top (%p)"
#define MH_S0246  6246	// "cul.cpp:xStkPt(): bad xStk->cs value %d"
#define MH_S0247  6247	// "Illegal character 0x%x in name"
#define MH_S0248  6248	// "Zero-length name not allowed"
#define MH_S0249  6249	// "Expected ';' (or class name, member name, or verb)"
#define MH_S0250  6250	// "'SUM' cannot be used here"
#define MH_S0251  6251	// "'ALL' cannot be used here"
#define MH_S0252  6252	// "'ALL_BUT' cannot be used here"
               // 6253
#define MH_S0254  6254	// "Misspelled word?  Expected class name, member name, command, or ';'."
#define MH_S0255  6255	// "Expected ';' (or class name, member name, or command)"
#define MH_S0256  6256	// "Expected name of class to %s"
#define MH_S0257  6257	// "Internal error in cul.cpp:bFind() call: bad cs %d"
#define MH_S0258  6258	// "Can't define types for '%s'"
#define MH_S0259  6259	// "'%s' is not %s-able (here)"	/* USED TWICE */
#define MH_S0260  6260	// "'%s' cannot be given here"
#define MH_S0261  6261	// "Expected name of member to %s"
#define MH_S0262  6262	// "Confused at '%s' after error(s) above.  Terminating session."
#define MH_S0263  6263	// "No %s given in %s"
#define MH_S0264  6264	// "No %s in %s"
#define MH_S0265  6265	// "cul.cpp:ratPutTy(): 'e' argument not valid RAT entry ptr"
#define MH_S0266  6266	// "cul.cpp:ratPutTy(): RAT entry to be made a TYPE must be last"
#define MH_S0267  6267	// "Out of near heap memory (cul:ratTyr)"
#define MH_S0268  6268	// "cul.cpp:ratTyR(): argument not valid RATBASE ptr"
#define MH_S0269  6269	// "out of near heap memory (cul:ratTyr)"
#define MH_S0270  6270	// "cul.cpp:nxRat: Too much nesting in tables: Increase size of context stack xStk[] in cul.cpp"
// #define MH_S0271  6271	unused
#define MH_S0272  6272	// "*** oerI(); probable non-RAT record arg ***"
#define MH_S0273  6273	// "*** objIdTx(); probable non-RAT record ptr ***"
#define MH_S0274  6274	// "*** classObjTx(); probable non-RAT record arg ***"
#define MH_S0275  6275	// "cul.cpp:culMbrId called with xSp %p not top (%p)"
#define MH_S0276  6276	// "[%s or %s: table ambiguity: recode this error message to not use cul.cpp:culMbrId]"
#define MH_S0277  6277	// "[%s not found by cul.cpp:culMbrId]"
//10-92...
#define MH_S0278  6278	// "%s '%s' not found"  2 uses in cul.cpp:ratLuDefO.
#define MH_S0279  6279	// "ambiguous name: \n    %s '%s' exists in %s\n    and also in %s"
// 6-95
#define MH_S0280  6280	// "'%s' cannot be AUTOSIZEd"
#define MH_S0281  6281	// "Invalid duplicate 'AUTOSIZE %s' or '%s =' command"
/*
#define MH_S0282  6282	//
#define MH_S0283  6283	//
#define MH_S0284  6284	//
#define MH_S0285  6285	//
#define MH_S0286  6286	//
#define MH_S0287  6287	//
#define MH_S0288  6288	//
#define MH_S0289  6289	//
#define MH_S0290  6290	//
*/

// input statement errors & setup time errors, cncult.cpp  7-92.
#define MH_S0400  6400	// "lrFrmMat given but lrFrmFrac omitted"
#define MH_S0401  6401	// "lrFrmFrac=%g given but lrFrmMat omitted"
#define MH_S0402  6402	// "Cannot give both conU and layers"
#define MH_S0403  6403	// "Neither conU nor any layers given"
#define MH_S0404  6404	// "More than %d sgdist's for same window"
#define MH_S0405  6405	// "Neither drCon nor drU given"
#define MH_S0406  6406	// "Both drCon and drU given"
#define MH_S0408  6408	// "Wall sfTilt = %g: not 60 to 180 degrees"
#define MH_S0409  6409	// "sfTilt (other than 180 degrees) may not be specified for a floor"
#define MH_S0410  6410	// "Ceiling sfTilt = %g: not 0 to 60 degrees"
#define MH_S0411  6411	// "sfAzm may not be given for floors [and intmasses]"
#define MH_S0412  6412	// "No sfAzm given for tilted surface"
#define MH_S0414  6414	// "Cannot use sfModel=Delayed without giving sfCon"
#define MH_S0417  6417	// "Neither sfCon nor sfU given"
#define MH_S0418  6418	// "Both sfCon and sfU given"
#define MH_S0420  6420	// "sfExAbs may not be given if sfExCnd is not Ambient or SpecifiedT"
#define MH_S0422  6422	// "sfExCnd is SpecifiedT but no sfExT given"
#define MH_S0423  6423	// "sfExT not needed when sfExCnd not 'SpecifiedT'"
#define MH_S0425  6425	// "sfExCnd is Zone but no sfAdjZn given"
#define MH_S0426  6426	// "sfAdjZn not needed when sfExCnd not 'Zone'"
#define MH_S0427  6427	// "More than %d terminals for zone '%s'"
#define MH_S0428  6428	// "%sFreq may not be given with %sType=%s"
#define MH_S0429  6429	// "%sDayBeg may not be given with %sType=%s"
#define MH_S0430  6430	// "%sDayEnd may not be given with %sType=%s"
#define MH_S0431  6431	// "No %sFreq given: required with %sType=%s"
#define MH_S0432  6432	// "%sDayBeg may not be given with %sFreq=%s"
#define MH_S0433  6433	// "%sDayEnd may not be given with %sFreq=%s"
#define MH_S0434  6434	// "%sFreq=%s not allowed with %sty=%s"
#define MH_S0435  6435	// "%sFreq=%s not allowed in Sum-of-%s %s"
#define MH_S0436  6436	// "No %sDayBeg given: required with %sFreq=%s"
#define MH_S0437  6437	// "Internal error in RI::ri_CkF: bad rpFreq %d"
#define MH_S0438  6438	// "Internal error in RI::ri_CkF: bad rpTy %d"
#define MH_S0439  6439	// "%sCpl cannot be given unless %sTy is UDT"
#define MH_S0440  6440	// "%sTitle cannot be given unless %sTy is UDT"
#define MH_S0441  6441	// "Duplicate ReportFile FileName '%s'"
#define MH_S0442  6442	// "Duplicate ExportFile FileName '%s'"
#define MH_S0443  6443	// "infShld = %d: not in range 1 to 5"
#define MH_S0444  6444	// "infStories = %d: not in range 1 to 3"
#define MH_S0445  6445	// "'%s' is obsolete. Please remove it from your input."  1-93 for csType etc
//#define MH_S0446  6446	//
//#define MH_S0447  6447	//
//#define MH_S0448  6448	//
//#define MH_S0449  6449	//
                // 6450-6469

// input statement setup time errors, cncult2.cpp  7-92...
#define MH_S0470  6470	// "Daylight saving time should not start and end on same day"
#define MH_S0471  6471	// "infShld = %d: not in range 1 to 5"
#define MH_S0472  6472	// "infStories = %d: not in range 1 to 3"
#define MH_S0473  6473	// "Internal error: bad izNVType 0x%x"
#define MH_S0474  6474	// "when izNVType is NONE or omitted"
#define MH_S0475  6475	// "\n    (no ventilation is modelled with no height difference)"
#define MH_S0476  6476	// "when izNVType is NONE or omitted"
#define MH_S0477  6477	// "lrMat not given"
#define MH_S0478  6478	// "Ignoring unexpected mt_rNom=%g of framing material '%s'"
#define MH_S0479  6479	// "Non-0 lrFrmFrac %g given but lrFrmMat omitted."
#define MH_S0480  6480	// "lrFrmMat given, but lrFrmFrac omitted"
#define MH_S0481  6481	// "Material thickness %g does not match lrThk %g"
#define MH_S0482  6482	// "Framing material thickness %g does not match lrThk %g"
#define MH_S0483  6483	// "Framing material thickness %g does not match \n    material thickness %g"
#define MH_S0484  6484	// "No thickness given in layer, nor in its material%s"
#define MH_S0485  6485	// "Only 1 framed layer allowed per construction"
#define MH_S0486  6486	// "More than 1 framed layer in construction"
#define MH_S0487  6487	// "Both conU and layers given"
#define MH_S0488  6488	// "Neither conU nor layers given"
#define MH_S0489  6489	// "No gnEndUse given"
#define MH_S0490  6490	// "No gnMeter given (required when gnEndUse is given)"
#define MH_S0491  6491	// "More than 100 percent of gnPower distributed:\n    Total of fractions gnFrZn, gnFrPl, and gnFrRtn exceeds 1.0:\n        gnFrZn (%g) + gnFrPl (%g) + gnFrRtn (%g) = %g%s"
#define MH_S0493  6493	// "Required member '%s' has not been set,\n    and apparently no message about it appeared above"
#define MH_S0494  6494	// "bad %s index %d in%s %s '%s' [%d]%s"
#define MH_S0495  6495	// "%s: Internal error: field # %d: unterminated arg list?"
#define MH_S0496  6496	// "Can't give '%s' %s"
#define MH_S0497  6497	// "'%s' missing: required %s"
#define MH_S0498  6498	// "'%s' missing"
#define MH_S0499  6499	// "'%s' must be > 0"
#define MH_S0500  6500	// "refTemp (%g) seems to be over boiling for elevation (%g)"  rob 5-95
#define MH_S0501  6501	// "Weather file does not include all run dates:\n""    run is %s to %s,\n""    but '%s' dates are %s to %s.\n"
#define MH_S0502  6502	// "Run over a month long cannot end in starting month\n""    when hourly binary results are requested."
#define MH_S0503  6503	// "Extension given in binary results file name \"%s\" \n""    will not be used -- extensions \".brs\" and \".bhr\" are always used."
#define MH_S0504  6504	// "You have given a binary results file name with\n""        \"BinResFileName = %s\",\n""    but no binary results file will be written as you have not given\n""    any of the following commands:\n""        BinResFile = YES;  BinResFileHourly = YES;\n""    nor either of the following command line switches:\n""        -r   -h\n"
#define MH_S0505  6505	// "You have given a binary results file name with\n""        \"BinResFileName = %s\",\n""    but no binary results file will be written as you have also specified\n""    memory-only binary results with the -m DLL command line switch."
#define MH_S0506  6506	// "gtSMSC (%g) > gtSMSO (%g):\n""    Shades-Closed Solar heat gain coeff Multiplier\n""      must be <= shades-Open value."
#define MH_S0507  6507	// "No gnEndUse given when gnDlFrPow given.\n""    gnEndUse=\"Lit\" is usual with gnDlFrPow."
#define MH_S0508  6508	// "gnEndUse other than \"Lit\" given when gnDlFrPow given.\n""    gnEndUse=\"Lit\" is usual with gnDlFrPow."
               // 6509 free

// input statement setup time errors... cncult3.cpp  7-92
#define MH_S0511  6511	// "Area of %s (%g) is greater than \n    area of %s's surface%s (%g)"
#define MH_S0512  6512	// "Area of %s (%g) is greater than \n    remaining area of %s's surface%s \n    (%g after subtraction of previous door areas)"
#define MH_S0513  6513	// "Can't use sfModel=Delayed, Delayed_Hour, or Delayed_Subhour \n    without giving sfCon" // also cncult.cpp 1-95
#define MH_S0514  6514	// "sfModel=Delayed, Delayed_Hour, or Delayed_Subhour  given \n    but surface's construction, '%s', has no layers"
//unused 1-10-95:
//#define MH_S0515  6515	// "Time constant %g too short for sfModel=Delayed, Delayed_Hour, \n    or Delayed_Subhour.  Using sfModel=Quick.\n    (Time constant = constr heat cap (%g) / sfExH (%g) \n    must be < %g to use 'sfModel=Delayed')"
#define MH_S0516  6516	// "wnSCSC (%g) > wnSCSO (%g):\n    shades-closed shading coeff must be <= shades-open shading coeff"
#define MH_S0517  6517	// "More than %d SGDISTS for same window"
#define MH_S0518  6518	// "%ssurface '%s' not in zone '%s'. \n    Can't target solar gain to mass not in window's zone."
#define MH_S0519  6519	// "Target surface '%s' is not delayed model.\n    Solar gain being directed to zone '%s' air."
#define MH_S0520  6520	// "Internal error in cncult.cpp:topSg: Delayed model target surface\n    '%s' has no .msi assigned."
#define MH_S0521  6521	// "sfTilt is %g.  Shaded window must be in vertical surface."
#define MH_S0522  6522	// "Window '%s' is already shaded by shade '%s'. \n    Only 1 SHADE per window allowed."
#define MH_S0524  6524	// "Internal error in cncult.cpp:topSf2(): Bad exterior condition %d"
#define MH_S0525  6525	// "Internal error in cncult.cpp:topSf2(): Bad surface model %d"
#define MH_S0526  6526	// "Bad sfExCnd %d (ms_Make())"
#define MH_S0527  6527	// "Mass model setup error(s) (above)"
#define MH_S0528  6528	// "cncult:seriesU(): unexpected unset h or u"
#define MH_S0529  6529	// "cncult3.cpp:cnuCompAdd: bad zone index %d"
//cncult3 additions 6-95 .462
#define MH_S0530  6530	// "Window in interior wall not allowed"
#define MH_S0531  6531	// "No U-value given: neither wnU nor glazeType '%s' gtU given"
#define MH_S0532  6532	/* "wnSMSC (%g) > wnSMSO (%g):\n"
			   "    shades-closed SHGC multiplier must be <= \n"
			   "    shades-open SHGC multiplier." */
#define MH_S0533  6533	/* "Time constant %g probably too short for delayed (massive) sfModel.\n"
			   "    Use of sfModel=Delayed, Delayed_Subhour, or Delayed_Hour \n"
			   "        is NOT recommened when time constant is < %g.\n"
			   "    Results may be strange. \n"
			   "    (Time constant = constr heat cap (%g) / sfInH (%g) )." */
#define MH_S0534  6534	/* "Time constant %g probably too short for sfModel=Delayed_Hour.\n"
			   "    Use of sfModel=Delayed_Hour \n"
			   "        is NOT recommened when time constant is < %g.\n"
			   "    Results may be strange. \n"
			   "    (Time constant = constr heat cap (%g) / sfInH (%g) )." */
//#define MH_S0535  6535	//
               // 6536-6539

// input statement setup time errors... cncult4.cpp  10-92
#define MH_S0540  6540	// "repCpl %d is not between 78 and 132"
#define MH_S0541  6541	// "repLpp %d is less than 50"
#define MH_S0542  6542	// "repTopM %d is not between 0 and 12"
#define MH_S0543  6543	// "repBotM %d is not between 0 and 12"
#define MH_S0544  6544	// "File %s exists"	/* two uses */
#define MH_S0545  6545	// "%sCol associated with %s whose %sType is not UDT"
#define MH_S0546  6546	// "End-of-interval varying value is reported more often than it is set:\n""    %sFreq of %s '%s' is '%s',\n""    but colVal varies (is given a value) only at end of %s."
#define MH_S0547  6547	// "Bad data type (colVal.dt) %d"
#define MH_S0548  6548	// "%sCond may not be given with %sType=%s"
#define MH_S0549  6549	// "No %sMeter given: required with %sType=%s"
#define MH_S0550  6550	// "No %sAh given: required with %sType=%s"
#define MH_S0551  6551	// "No %sZone given: required with %sType=%s"
#define MH_S0552  6552	// "%sZone may not be given with %sType=%s"
#define MH_S0553  6553	// "%sMeter may not be given with %sType=%s"
#define MH_S0554  6554	// "%sAh may not be given with %sType=%s"
#define MH_S0555  6555	// "cncult:topRp: Internal error: Bad %sType %d"
#define MH_S0556  6556	// "No exExportfile given"
#define MH_S0557  6557	// "No rpReportfile given"
#define MH_S0558  6558	// "rpZone=='%s' cannot be used with %sType '%s'"
#define MH_S0559  6559	// "rpMeter=='%s' cannot be used with %sType '%s'"
#define MH_S0560  6560	// "rpAh=='%s' cannot be used with %sType '%s'"
#define MH_S0561  6561	// "%sTitle may only be given when %sType=UDT"
#define MH_S0562  6562	// "%sport has %sCols but %sType is not UDT"
#define MH_S0563  6563	// "no %sCols given for user-defined %s"
#define MH_S0564  6564	// "%s width %d greater than line width %d.\n""    Override this message if desired with %sCpl = %d"
#define MH_S0565  6565	// "cncult:topRp: unexpected %sTy %d"
#define MH_S0566  6566	// "Can't intermix use of hdCase, hdDow, and hdMon\n""    with hdDateTrue, hdDateObs, and hdOnMonday for same holiday"
#define MH_S0567  6567	// "No hdDateTrue given"
#define MH_S0568  6568	// "hdDateTrue not a valid day of year"
#define MH_S0569  6569	// "hdDateObs not a valid day of year"
#define MH_S0570  6570	// "Can't give both hdDateObs and hdOnMonday"
#define MH_S0571  6571	// "If any of hdCase, hdDow, hdMon are given, all three must be given"
#define MH_S0572  6572	// "hdDow not a valid month"
#define MH_S0573  6573	// "Either hdDateTrue or hdCase+hdDow+hdMon must be given"
//imports 2-94
#define MH_S0574  6574	// "No IMPORTFILE \"\s\" found for IMPORT(%s,...)"
#define MH_S0575  6575	// "Unused IMPORTFILE: there are no IMPORT()s from it."
#define MH_S0576  6576	// "imHeader=NO but IMPORT()s by field name (not number) used"
#define MH_S0577  6577	// "Unexpected call to IMPF::operator="
#define MH_S0578  6578	// "Unexpected call to IFFNM::operator="
                //6579-6599 avail

// input statement setup time errors... cncult5.cpp  10-93
#define MH_S0600  6600	// "when terminal has no local heat\n    (when neither tuTLh nor tuQMnLh given)"
#define MH_S0601  6601	// "tuQMxLh = %g will be ineffective because tuhcCaptRat is less: %g"
#define MH_S0602  6602	/* "'tuQDsLh' is obsolete and should be removed from your input file.\n"
	                   "    (Note: if heatPlant overloads, available power will be apportioned\n"
	                   "    amoung HW coils in proportion to tuhcCaptRat/ahhcCaptRat values.)\n" */
#define MH_S0603  6603	// "when local heat is not thermostat controlled\n    (when tuTLh is not given)"
#define MH_S0604  6604	// "when terminal has thermostat-controlled local heat\n    (when tuTLh is given)"
#define MH_S0605  6605	// "when tuTH not given"
#define MH_S0606  6606	// "when tuTC not given"
#define MH_S0607  6607	// "when terminal has no air heat nor cool\n    (when none of tuTH, tuTC, or tuVfMn is given)"
#define MH_S0608  6608	// "when terminal has air heat or cool\n    (when any of tuTH, tuTC, or tuVfMn are given)"
#define MH_S0609  6609	// "when zone plenumRet is 0"
#define MH_S0610  6610	/* "when terminal has no capabilities: \n    "
                           "no setpoint (tuTLh, tuTH, tuTC) nor minimum (tuQMnLh, tuVfMn) given" */
#define MH_S0611  6611	// "when tfanType = NONE"
#define MH_S0612  6612	// "when tfanType not NONE"
#define MH_S0613  6613	/* "Ignoring terminal with no capabilities: \n"
			   "    no setpoint (tuTLh, tuTH, tuTC) nor minimum (tuQMnLh, tuVfMn) given" */
#define MH_S0614  6614	// "Air handler has no terminals!"
#define MH_S0615  6615	// "oaVfDsMn > sfanVfDs: min outside air greater (%g) than supply fan capacity (%g)"
#define MH_S0616  6616	/* "oaVfDsMn required: default (%g) of .15 cfm per sf of served area (%g) \n"
			   "    would be greater than supply fan capacity (%g)" */
#define MH_S0617  6617	// "when no economizer (oaEcoTy NONE or omitted)"
#define MH_S0618  6618	// "oaOaLeak + oaRaLeak = %g: leaking %d% of mixed air!"
#define MH_S0619  6619	/* "Control Terminal for air handler %s:\n"
			   "    Must give heating setpoint tuTH and/or cooling setpoint tuTC" */
#define MH_S0620  6620	// "when air handler has no econimizer nor coil\n    (did you forget to specify your coil?)"
#define MH_S0621  6621	// "unless air handler has no economizer nor coil"
#define MH_S0622  6622	// "when ahTsSp is ZN or ZN2 and more than one terminal served"
#define MH_S0623  6623	// "when ahTsSp is RA"
#define MH_S0624  6624	// "ahFanCycles=YES not allowed when ahTsSp is ZN2"
#define MH_S0625  6625	// "More than one terminal not allowed on air handler when fan cycles"
#define MH_S0626  6626	// "No control terminal: required when fan cycles."
#define MH_S0627  6627	/* "When fan cycles, \"constant volume\" outside air works differently than you\n"
			   "    may expect: outside air will be supplied only when fan is on per zone\n"
			   "    thermostat.  To disable outside air use the command \"oaVfDsMn = 0;\"." */
#define MH_S0628  6628	/* "Supply fan maximum flow (vfDs x vfMxF) is %g,\n"
			   "    but terminal '%s' max cooling air flow (tuVfMxC) is %g.\n"
			   "    Usually, these should be equal when fan cycles.\n"
			   "    The more limiting value will rule."*/
#define MH_S0629  6629	/* "Supply fan maximum flow (vfDs x vfMxF) is %g,\n"
			   "    but terminal '%s' max heating air flow (tuVfMxH) is %g.\n"
			   "    Usually, these should be equal when fan cycles.\n"
			   "    The more limiting value will rule." */
#define MH_S0630  6630	// "Control terminal '%s':\n    tuVfMn=%g: must be zero or omitted when fan cycles."
#define MH_S0631  6631	// "when cchCM is NONE"
#define MH_S0632  6632	/* "Crankcase heater specifed, but neither heat coil nor cool coil \n"
			   "    is a type for which crankcase heater is appropriate" */
#define MH_S0633  6633	// "cchPMx (%g) must be > cchPMn (%g)"
#define MH_S0634  6634	// "cchTMx (%g) must be <= cchTMn (%g)"
#define MH_S0635  6635	// "when cchCM is not PTC nor PTC_CLO"
#define MH_S0636  6636	// "cchTOff (%g) must be > cchTOn (%g)"
#define MH_S0637  6637	// "when cchCM is not TSTAT"
#define MH_S0638  6638	// "Internal error: Bad cchCM %d"
#define MH_S0639  6639	// "Internal error: cncult5.cpp:ckCoil: bad coil app 0x%x"
#define MH_S0640  6640	// "'%s' given, but no '%s' to charge to it given"
#define MH_S0641  6641	// "when ahccType is NONE or omitted"
#define MH_S0642  6642	// "Bad coil type '%s' for %s"
#define MH_S0643  6643	// "Sensible capacity '%s' larger than Total capacity '%s'"
#define MH_S0644  6644	// "when ahccType is CHW"
#define MH_S0645  6645	// "when ahccType is CHW"
#define MH_S0646  6646	// "when ahccType is not CHW"
#define MH_S0647  6647	// "%scoiling coil rating air flow (%g) differs \n    from supply fan design flow (%g) by more than 20%"
#define MH_S0648  6648	// "when ahccType is not DX"
#define MH_S0649  6649	// "when ahccType is not DX nor CHW"
#define MH_S0650  6650	// "Bad coil type '%s' for %s"
#define MH_S0651  6651	// "Heating coil rated capacity 'captRat' %g is not > 0"
#define MH_S0652  6652	// "Cannot give both %s and %s"
#define MH_S0653  6653	// "%s, %s must be constant 1.0 or omitted"
#define MH_S0654  6654	// "%s, %s must be 1.0 or omitted"
#define MH_S0655  6655	// "%s must be >= 1.0"
#define MH_S0656  6656	// "%s,\n    Either %s or %s must be given"
#define MH_S0657  6657	// "%s must be <= 1.0"
#define MH_S0658  6658	// "%s must be > 0."
#define MH_S0659  6659	// "when ahhcType is NONE or omitted"
#define MH_S0660  6660	// "when ahhcType is not AHP"
#define MH_S0661  6661	// "when ahhcType is AHP"
#define MH_S0662  6662	// "ahpCap17 (%g) must be < ahhcCaptRat (%g)"
#define MH_S0663  6663	// "ahpDfrFMx (%g) must be > ahpDfrFMn (%g)"
#define MH_S0664  6664	// "ahpTOff (%g) must be <= ahpTOn (%g)"
#define MH_S0665  6665	// "ahpCd (%g) must be < 1"
#define MH_S0666  6666	// "ahpTFrMn (%g) must be < ahpTFrMx (%g)"
#define MH_S0667  6667	// "ahpTFrMn (%g) must be < ahpTFrPk (%g)"
#define MH_S0668  6668	// "ahpTFrPk (%g) must be < ahpTFrMx (%g)"
#define MH_S0669  6669	// "ahpTFrMn (%g) must be < 35."
#define MH_S0670  6670	// "ahpTFrMx (%g) must be > 35."
#define MH_S0672  6672	/* "ahpCap35 (%g) cannot be > proportional value (%g) between \n"
			   "    ahpCap17 (%g) and ahhcCaptRat (%g) -- \n"
			   "    that is, frost/defrost effects cannot increase capacity." */
#define MH_S0673  6673	/* "cdm (%g) not in range 0 <= cdm < 1\n"
			   "    (cdm is ahpCd modified internally to remove crankcase heater effects)" */
#define MH_S0674  6674	/* "cdm (%g) not <= ahpCd (%g): does this mean bad input?\n"
			   "    (cdm is ahpCd modified internally to remove crankcase heater effects)" */
#define MH_S0675  6675	// "when ahhcType is AHP"
#define MH_S0676  6676	// "Internal error: cncult5.cpp:ckFan: bad fan app 0x%x"
#define MH_S0677  6677	// "[internal error: no supply fan]"
#define MH_S0678  6678	// "when rfanType is NONE or omitted"
#define MH_S0679  6679	// "when no exhaust fan (xfanVfDs 0 or omitted)"
#define MH_S0680  6680	// "when tfanType is NONE or omitted"
#define MH_S0681  6681	// "bad fan type '%s' for %s"
#define MH_S0682  6682	// "'%s' must be given"
#define MH_S0683  6683	// "'%s' cannot be less than 1.0"
#define MH_S0684  6684	// "'%s' cannot be less than 0"
#define MH_S0685  6685	// "Cannot give both '%s' and '%s'"
#define MH_S0686  6686	// "'%s' is less than air-moving output power \n      ( = '%s' times '%s' times units conversion factor)"
#define MH_S0687  6687	/* "Inconsistent '%s' coefficients: value is %g, not 1.0,\n"
			   "    for %s = %g.\n"
			   "    Coefficients will be normalized and execution will continue." USED 3 PLACES. */
#define MH_S0688  6688	// "Inconsistent '%s' coefficients: value is %g, not 1.0,\n    for %s = %g"  USED 3 PLACES.
#define MH_S0689  6689	/* "Inconsistent '%s' coefficients: value is %g, not 1.0,\n"
			   "    for %s=%g and %s=%g.\n"
			   "    Coefficients will be normalized and execution will continue." */
#define MH_S0690  6690	/* "Inconsistent '%s' coefficients: value is %g, not 1.0,\n"
			   "    for %s=%g and %s=%g\n" */
#define MH_S0691  6691	// "when terminal has thermostat-controlled air heat (when tuTH is given)"	// 4-95
#define MH_S0692  6692	// "when terminal has thermostat-controlled air cooling (when tuTC is given)"	// 4-95
// 6693 - 6699 avail
//#define MH_S0693  6693	//
//#define MH_S0694  6694	//
//#define MH_S0695  6695	//
//#define MH_S0696  6696	//
//#define MH_S0697  6697	//
//#define MH_S0698  6698	//
//#define MH_S0699  6699	//

// input statement setup time errors... cncult6.cpp  10-93
#define MH_S0700  6700	// "heatPlant has no BOILERs"
#define MH_S0701  6701	// "HeatPlant has no loads: no HW coils, no HPLOOP HX's."
#define MH_S0702  6702	// "%s: boiler '%s' is not in this heatPlant"
#define MH_S0703  6703	// "%s: BOILER '%s' cannot be used twice in same stage"
#define MH_S0704  6704	// "Internal error: %s not terminated with 0"
#define MH_S0705  6705	// "stage %s is not more powerful than hpStage%d, \n    and thus will never be used"
#define MH_S0706  6706	// "No non-empty hpStages specified"
#define MH_S0707  6707	// "Not used in any hpStage of heatPlant"
#define MH_S0708  6708	// "blrEirR must be >= 1.0"
#define MH_S0709  6709	// "Can't give both blrEirR and blrEffR"
#define MH_S0710  6710	// "'%s' given, but no '%s' to charge to it given"
#define MH_S0711  6711	// "coolPlant has no CHILLERs"
#define MH_S0712  6712	// "CoolPlant has no loads: no CHW coils."
#define MH_S0713  6713	// "%s: chiller '%s' is not in this coolPlant"
#define MH_S0714  6714	// "%s: CHILLER '%s' cannot be used twice in same stage"
#define MH_S0715  6715	// "Internal error: %s not terminated with 0"
#define MH_S0716  6716	// "Internal error: bad # chillers (%d) found in stage %s"
#define MH_S0717  6717	// "Internal error: nonNegative total capacity %g of chillers in stage %s"
#define MH_S0718  6718	/* "stage %s is not more powerful (under design conditions) than \n"
			   "    cpStage%d, and thus may never be used" */
#define MH_S0719  6719	// "No non-empty cpStages specified"
#define MH_S0720  6720	// "Not used in any cpStage of coolPlant"
#define MH_S0721  6721	/* "Total design flow of connected coils, %g lb/hr, is greater than\n"
			   "    most powerful stage primary pumping capacity (with overrun), %g lb/hr." */
#define MH_S0722  6722	// "chMinFsldPlr (%g) must be <= chMinUnldPlr (%g)"
#define MH_S0723  6723	// "Primary pump heat (%g) exceeds chCapDs (%g)"
#define MH_S0724  6724	// "Primary pump heat (%g) exceeds 1/4 of chCapDs (%g)."
#define MH_S0725  6725	// "Condenser pump heat (%g) exceeds 1/4 of chCapDs (%g)."
#define MH_S0726  6726	// "when chEirDs is given"
#define MH_S0727  6727	// "'%s' given, but no '%s' to charge to it given"
#define MH_S0728  6728	// "TowerPlant has no loads: no coolPlants, no hpLoop HX's."
#define MH_S0729  6729	// "Total pump gpm of connected loads (%g) must be > 0"
#define MH_S0730  6730	// "when ctTy is not TWOSPEED"
#define MH_S0731  6731	// "ctVfOd (%g) is same as ctVfDs.\n    These values must differ %s."
#define MH_S0732  6732	// "ctVfOd and ctVfDs are both defaulted to %g.\n    These values must differ %s."
#define MH_S0733  6733	// "ctGpmOd (%g) is same as ctGpmDs.\n    These values must differ %s."
#define MH_S0734  6734	// "ctGpmOd and ctGpmDs are both defaulted to %g.\n    These values must differ %s."
#define MH_S0735  6735	// "ctK must be less than 1.0"
#define MH_S0736  6736	// "Inconsistent low and hi speed fan curve polynomials:\n    ctFcLo(ctLoSpd)=%g, but ctFcHi(ctLoSpd)=%g."
#define MH_S0737  6737	// "when ctType is not ONESPEED"
#define MH_S0738  6738	// "when ctType is not TWOSPEED"
#define MH_S0739  6739	// "when ctType is not VARIABLE"
#define MH_S0740  6740	/* "maOd/mwOd must not equal maDs/mwDs.\n"
			   "    maOd=%g  mwOd=%g  maDs=%g  mwDs=%g\n"
			   "    maOd/mwOd = maDs/mwDs = %g" */
#define MH_S0741  6741	// "ctK (%g) did not come out between 0 and 1 exclusive"
#define MH_S0742  6742	/* "%s outdoor wetbulb temperature (ctTWbODs=%g) must be\n"
			   "    less than %s leaving water temperature (ctTwoDs=%g)" */
#define MH_S0743  6743	// "Internal error in setupNtuaADs: haiDs (%g) not < hswoDs (%g)"
#define MH_S0744  6744	/* "%s conditions produce impossible air heating:\n"
			   "    enthalpy of leaving air (%g) not less than enthalpy of\n"
			   "    saturated air (%g) at leaving water temp (ctTWbODs=%g).\n"
			   "    Try more air flow (ctVfDs=%g)." */
#define MH_S0745  6745	// "Internal error: ntuADs (%g) not > 0"
#define MH_S0746  6746	/* "%s outdoor wetbulb temperature (ctTWbOOd=%g) must be\n"
			   "    less than %s leaving water temperature (ctTwoOd=%g)" */
#define MH_S0747  6747	// "Internal error in setupNtuaAOd: haiOd (%g) not < hswoOd (%g)"
#define MH_S0748  6748	// "Internal error: ntuAOd (%g) not > 0"
#define MH_S0749  6749	// "%s must be >= 1.0"
// 6750... avail

//#define MH_S0750  6750	//
//#define MH_S0751  6751	//
//#define MH_S0752  6752	//
//#define MH_S0753  6753	//
//#define MH_S0754  6754	//
//#define MH_S0755  6755	//
//#define MH_S0756  6756	//
//#define MH_S0757  6757	//
//#define MH_S0758  6758	//
//#define MH_S0759  6759	//
//#define MH_S0760  6760	//
//#define MH_S0761  6761	//
//#define MH_S0762  6762	//
//#define MH_S0763  6763	//
//#define MH_S0764  6764	//
//#define MH_S0765  6765	//
//#define MH_S0766  6766	//
//#define MH_S0767  6767	//
//#define MH_S0768  6768	//
//#define MH_S0769  6769	//
//#define MH_S0770  6770	//
//#define MH_S0771  6771	//
//#define MH_S0772  6772	//
//#define MH_S0773  6773	//
//#define MH_S0774  6774	//
//#define MH_S0775  6775	//
//#define MH_S0776  6776	//
//#define MH_S0777  6777	//
//#define MH_S0778  6778	//
//#define MH_S0779  6779	//
//#define MH_S0780  6780	//
//#define MH_S0781  6781	//
//#define MH_S0782  6782	//
//#define MH_S0783  6783	//
//#define MH_S0784  6784	//
//#define MH_S0785  6785	//
//#define MH_S0786  6786	//
//#define MH_S0787  6787	//
//#define MH_S0788  6788	//
//#define MH_S0789  6789	//
//#define MH_S0790  6790	//
//#define MH_S0791  6791	//
//#define MH_S0792  6792	//
//#define MH_S0793  6793	//
//#define MH_S0794  6794	//
//#define MH_S0795  6795	//
//#define MH_S0796  6796	//
//#define MH_S0797  6797	//
//#define MH_S0798  6798	//
//#define MH_S0799  6799	//

//#define MH_S0800  6800	//
//#define MH_S0801  6801	//
//#define MH_S0802  6802	//
//#define MH_S0803  6803	//
//#define MH_S0804  6804	//
//#define MH_S0805  6805	//
//#define MH_S0806  6806	//
//#define MH_S0807  6807	//
//#define MH_S0808  6808	//
//#define MH_S0809  6809	//
//#define MH_S0810  6810	//
//#define MH_S0811  6811	//
//#define MH_S0812  6812	//
//#define MH_S0813  6813	//
//#define MH_S0814  6814	//
//#define MH_S0815  6815	//
//#define MH_S0816  6816	//
//#define MH_S0817  6817	//
//#define MH_S0818  6818	//
//#define MH_S0819  6819	//
//#define MH_S0820  6820	//
//#define MH_S0821  6821	//
//#define MH_S0822  6822	//
//#define MH_S0823  6823	//
//#define MH_S0824  6824	//
//#define MH_S0825  6825	//
//#define MH_S0826  6826	//
//#define MH_S0827  6827	//
//#define MH_S0828  6828	//
//#define MH_S0829  6829	//
//#define MH_S0830  6830	//
//#define MH_S0831  6831	//
//#define MH_S0832  6832	//
//#define MH_S0833  6833	//
//#define MH_S0834  6834	//
//#define MH_S0835  6835	//
//#define MH_S0836  6836	//
//#define MH_S0837  6837	//
//#define MH_S0838  6838	//
//#define MH_S0839  6839	//
//#define MH_S0840  6840	//
//#define MH_S0841  6841	//
//#define MH_S0842  6842	//
//#define MH_S0843  6843	//
//#define MH_S0844  6844	//
//#define MH_S0845  6845	//
//#define MH_S0846  6846	//
//#define MH_S0847  6847	//
//#define MH_S0848  6848	//
//#define MH_S0849  6849	//
//#define MH_S0850  6850	//
//#define MH_S0851  6851	//
//#define MH_S0852  6852	//
//#define MH_S0853  6853	//
//#define MH_S0854  6854	//
//#define MH_S0855  6855	//
//#define MH_S0856  6856	//
//#define MH_S0857  6857	//
//#define MH_S0858  6858	//
//#define MH_S0859  6859	//
//#define MH_S0860  6860	//
//#define MH_S0861  6861	//
//#define MH_S0862  6862	//
//#define MH_S0863  6863	//
//#define MH_S0864  6864	//
//#define MH_S0865  6865	//
//#define MH_S0866  6866	//
//#define MH_S0867  6867	//
//#define MH_S0868  6868	//
//#define MH_S0869  6869	//
//#define MH_S0870  6870	//
//#define MH_S0871  6871	//
//#define MH_S0872  6872	//
//#define MH_S0873  6873	//
//#define MH_S0874  6874	//
//#define MH_S0875  6875	//
//#define MH_S0876  6876	//
//#define MH_S0877  6877	//
//#define MH_S0878  6878	//
//#define MH_S0879  6879	//
//#define MH_S0880  6880	//
//#define MH_S0881  6881	//
//#define MH_S0882  6882	//
//#define MH_S0883  6883	//
//#define MH_S0884  6884	//
//#define MH_S0885  6885	//
//#define MH_S0886  6886	//
//#define MH_S0887  6887	//
//#define MH_S0888  6888	//
//#define MH_S0889  6889	//
//#define MH_S0890  6890	//
//#define MH_S0891  6891	//
//#define MH_S0892  6892	//
//#define MH_S0893  6893	//
//#define MH_S0894  6894	//
//#define MH_S0895  6895	//
//#define MH_S0896  6896	//
//#define MH_S0897  6897	//
//#define MH_S0898  6898	//
//#define MH_S0899  6899	//

// cuprobe.cpp -- probe()
#define MH_U0001   4001
#define MH_U0002   4002
#define MH_U0003   4003
#define MH_U0004   4004
#define MH_U0005   4005
#define MH_U0006   4006
#define MH_U0007   4007
#define MH_U0008   4008
// cuprobe.cpp:findMember
#define MH_U0010   4010
#define MH_U0011   4011
#define MH_U0012   4012
#define MH_U0013   4013
#define MH_U0014   4014
#define MH_U0015   4015
#define MH_U0016   4016
#define MH_U0017   4017
// cuprobe.cpp:tryImInProbe
#define MH_U0020   4020
#define MH_U0021   4021
#define MH_U0021a  4022
#define MH_U0022   4023
#define MH_U0023   4024
#define MH_U0024   4025
#define MH_U0025   4026	// "\nInternal error: Ambiguous class name '%s':\n   there are TWO %s rats with that .what. Change one of them.\n"	// 10-92

// --- Value conversion messages ---
// tdpak.cpp --
#define MH_V0001  501
#define MH_V0002  502
#define MH_V0003  503
// cvpak.cpp --
#define MH_V0010  510
#define MH_V0011  511
#define MH_V0012  512
#define MH_V0013  513
#define MH_V0014  514
#define MH_V0015  515
#define MH_V0016  516
//         17-20
#define MH_V0021  521
#define MH_V0022  522
#define MH_V0023  523
#define MH_V0024  524
#define MH_V0025  525
#define MH_V0026  526
#define MH_V0027  527
#define MH_V0028  528
//         27-30
#define MH_V0031  531
#define MH_V0032  532
#define MH_V0033  533
#define MH_V0034  534
#define MH_V0034  534
#define MH_V0035  535
//5-14-92 cvpak additions:
#define MH_V0036  536	// "cvpak:getChoiTxI(): choice %d out of range for dt 0x%x"
#define MH_V0037  537	// "cvpak:getChoiTxI(): given data type 0x%x not a choice type"
#define MH_V0038  538	// "cvpak:cvS2Choi(): given data type 0x%x not a choice type"
#define MH_V0039  539	// "'%s' is not one of"
//#define MH_V0040  540	//

// --- Runtime / calculation errors ---
// psychro1.cpp, psychro2.cpp --
#define MH_R0001  701
#define MH_R0002  702
#define MH_R0003  703	// "%s(): temperature apparently over boiling for elevation"	// psyTWSat  Rob 5-95
// wfpak.cpp
#define MH_R0100  800
#define MH_R0101  801
// #define MH_R0102  802
#define MH_R0103  803
#define MH_R0104  804
//#define MH_R0105  805	// probably unused, 10-94
//#define MH_R0106  806	// probably unused, 10-94
//wfpak additions 6-95
#define MH_R1101 1801	// "Corrupted header in ET1 weather file \"%s\"."
#define MH_R1102 1802	// "Missing or invalid number at offset %d in %s %s"
#define MH_R1103 1803	// "Bad decoding table for %s in wfpak.cpp"
#define MH_R1104 1804	// "Unrecognized characters after number at offset %d in %s %s"
//wfpak additions 6-95
#define MH_R0107  807	// "Design Days not supported in BSGS (aka TMP or PC) weather files."
#define MH_R0108  808	// "Program error in use of function wfPackedhrRead\n\n""    Invalid Month (%d) in \"jDay\" argument: not in range 1 to 12."
#define MH_R0109  809	// "Weather file \"%s\" contains no design days."
#define MH_R0110  810	// "Weather file contains no design day for month %d\n\n""    (Weather file \"%s\" contains design day data only for months %d to %d)."
#define MH_R0112  812	// "Unreadable Date."
#define MH_R0113  813	// "Incorrect date (%d Julian not %d Julian)."
#define MH_R0114  814	// "Unreadable hour."
#define MH_R0115  815	// "Incorrect hour (%d not %d)."
#define MH_R0116  816	// "Hourly data for hour %d is unreadable."
#define MH_R0117  817	// "Hourly zone data for hour %d is unreadable."
//#define MH_R0118  818	//
//#define MH_R0119  819	//
// mspak.cpp
#define MH_R0120  820
#define MH_R0121  821
#define MH_R0122  822
#define MH_R0123  823
#define MH_R0124  824
#define MH_R0125  825
// cgresult.cpp. some are runtime internal errors, classification is getting too tedious 9-92.
#define MH_R0150  850	// "cgresult:vrRxports: unexpected rpFreq %d"
#define MH_R0151  851	// "cgresult.cpp:vpRxports: unexpected rpType %d"
#define MH_R0152  852	// "%sCond for %s '%s' is unset or not yet evaluated"
//#define MH_R0153  853	//
// cgsolar.cpp 9-92
#define MH_R0160  860	// "Internal error in cgsolar.cpp:makHrSgt:\n    Surface '%s' sgdist[%d]: sgd_FSC (%g) != sgd_FSO (%g)."
#define MH_R0161  861	// "Window '%s': wnSCSC (%g) > wnSCSO (%g):\n    shades-closed shading coeff must be <= shades-open shading coeff."
#define MH_R0162  862	// "%g percent of %s solar gain for window '%s'\n    of zone '%s' distributed: more than 100 percent.  Check SGDISTs." /* 2 uses */
//cgsolar.cpp additions 6-95
#define MH_R0163  863	// "Window '%s': wnSMSC (%g) > wnSMSO (%g):\n""    SHGC Multiplier for Shades Closed must be <= same for Shades Open"
#define MH_R0164  864	// "cgsolar.cpp:SgThruWin::doit(): zone \"%s\", window \"%s\":\n""    undistributed gain fraction is %g (should be 0)."
#define MH_R0165  865	// "cgsolar.cpp:SgThruWin::toZoneCav:\n""    control zone (\"%s\") differs from target zone (\"%s\")."
#define MH_R0166  866	// "cgsolar.cpp:sgrAdd(): called for SGDTT %d"
//#define MH_R0167  867	//
/*
#define MH_R0180  880	//
#define MH_R0181  881	//
#define MH_R0182  882	//
#define MH_R0183  883	//
#define MH_R0184  884	//
#define MH_R0185  885	//
#define MH_R0186  886	//
#define MH_R0187  887	//
#define MH_R0188  888	//
#define MH_R0189  889	//
*/

// cueval.cpp: runtime errors in expression evaluator 7-92
#define MH_R0201  901	// "cuEvalTop: %d words left on eval stack"
#define MH_R0202  902	// "cuEvalR internal error: \n    value %d bytes: neither 2 nor 4"
// #define MH_R0203  903
#define MH_R0204  904	// "cuEvalI internal error: \n    eval stack overflow at ps ip=%p"
#define MH_R0205  905	// "cuEvalI internal error: \n    eval stack underflow at ps ip=%p"
#define MH_R0206  906	// "cuEvalI internal error: \n    arg access thru NULL frame ptr at ip=%p"
#define MH_R0207  907	// "Choice (%d) value used where number needed"
#define MH_R0208  908	// "Unexpected UNSET value where number neeeded"
#define MH_R0209  909	// "Unexpected reference to expression #%d"
#define MH_R0210  910	// "Unexpected mystery nan"
#define MH_R0211  911	// "Day of month %d is out of range 1 to %d"
#define MH_R0212  912	// "Division by 0"
#define MH_R0213  913	// "Division by 0."
#define MH_R0214  914	// "SQRT of a negative number"
#define MH_R0215  915	// "In choose() or similar fcn, \n    dispatch index %d is not in range %d to %d \n    and no default given."
#define MH_R0216  916	// "In select() or similar function, \n    all conditions false and no default given."
#define MH_R0217  917	// "%s subscript %d out of range %d to %d"
#define MH_R0218  918	// "%s name '%s' is ambiguous: 2 or more records found.\n    Change to unique names."
#define MH_R0219  919	// "%s record '%s' not found."
#define MH_R0220  920	// "cuEvalI internal error: bad expression number 0x%x"
#define MH_R0221  921	// "cuEvalI internal error:\n    Bad pseudo opcode 0x%x at psip=%p"
#define MH_R0222  922	// "Internal Error: mistimed probe: %s varies at end of %s\n    but accessed occurred %s"
#define MH_R0223  923	// "at BEGINNING of %s"
#define MH_R0224  924	// "neither at beginning nor end of an interval????"
#define MH_R0225  925	// "Mistimed probe: %s\n    varies at end of %s but accessed at end of %s.\n    Possibly you combined it in an expression with a faster-varying datum."
#define MH_R0226  926	// "Internal error: Unset data for %s"
#define MH_R0227  927	// "Internal error: bad expression number %d found in %s"
#define MH_R0228  928	// "%s has not been evaluated yet."  			// used twice in cueval.cpp.
#define MH_R0229  929	// "Bad IVLCH value %d"
#define MH_R0230  930	// "%s has not been evaluated yet."	10-92
#define MH_R0231  931	// "Unexpected being-autosized value where number needed"
#define MH_R0232  932	// "%s probed while being autosized"
//#define MH_R0233  933	//
//#define MH_R0234  934	//
//#define MH_R0235  935	//
//#define MH_R0236  936	//
//#define MH_R0237  937	//
//#define MH_R0238  938	//
//#define MH_R0239  939	//
//#define MH_R0240  940	//
//#define MH_R0241  941	//
//#define MH_R0242  942	//
//#define MH_R0243  943	//
//#define MH_R0244  944	//
//#define MH_R0245  945	//
//#define MH_R0246  946	//
//#define MH_R0247  947	//
//#define MH_R0248  948	//
//#define MH_R0249  949	//
//#define MH_R0250  950	//
//#define MH_R0251  951	//
//#define MH_R0252  952	//
//#define MH_R0253  953	//
//#define MH_R0254  954	//
//#define MH_R0255  955	//
//#define MH_R0256  956	//
//#define MH_R0257  957	//
//#define MH_R0258  958	//
//#define MH_R0259  959	//
//1000+,1100+ used near start of this file

// mostly run errors, virtual reports
//vrpak.cpp 9-92
#define MH_R1200 1200	// "re virtual reports, file '%s', function '%s':\n%s"
#define MH_R1201 1201	// "Ignoring redundant or late call"
#define MH_R1202 1202	// "Too little buffer space and can't write: deadlock"
#define MH_R1203 1203	// "Virtual reports not initialized"
#define MH_R1204 1204	// "Virtual reports spool file not open"
#define MH_R1205 1205	// "Out of range virtual report handle %d"
#define MH_R1206 1206	// "Virtual report %d not open to receive text"
#define MH_R1210 1210	// "Unspool called but nothing has been spooled"
#define MH_R1211 1211	// "After initial setup, 'spl.uN' is 0"
#define MH_R1212 1212	// "At end pass, uns[%d] hStat is %d not 0"
#define MH_R1213 1213	// "At end of pass, all uns[].did's are 0"
#define MH_R1214 1214	// "Unspool did not use all its arguments"
#define MH_R1215 1215	// "At end of Unspool, uns[%d] fStat is %d not -2"
#define MH_R1216 1216	// "At end of Unspool, uns[%d] hStat is %d not 2"
#define MH_R1217 1217	// "Out of range vr handle %d for output file %s in vrUnspool call"
#define MH_R1218 1218	// "Unformatted virtual report, handle %d, name '%s' \nin formatted output file '%s'.  Will continue."
#define MH_R1219 1229	// "Unexpected empty unspool buffer"
#define MH_R1220 1220	// "Invalid type %d in spool file at offset %ld"
#define MH_R1221 1221	// "text runs off end spool file"
#define MH_R1222 1222	// "text too long for spool file buffer"
#define MH_R1223 1223	// "Debugging Information: Number of simultaneous report/export output \n"
                        // "  files limited to %d by # available DOS file handles.  Expect bugs."
               //1224-1349 avail
//cse.cpp (1000+), cnguts.cpp (1100+): near start of file

// runtime errors: cnztu.cpp 10-92
#define MH_R1250 1250	// "airHandler - Terminals convergence failure: %d iterations"
#define MH_R1251 1251	// "More than %d errors.  Terminating run."
#define MH_R1252 1252	// "heatPlant %s is scheduled OFF, \n"
			// "    but terminal %s's local heat is NOT scheduled off: \n"
			// "        tuQMnLh = %g   tuQMxLh = %g"
#define MH_R1253 1253	// "Local heat that requires air flow is on without flow\n"
			// "    for terminal '%s' of zone '%s'.\n"
			// "    Probable internal error, \n"
			// "    but perhaps you can compensate by scheduling local heat off."
#define MH_R1254 1254	// "Internal error: 'b' is 0, cannot determine float temp for zone '%s'"
#define MH_R1255 1255	// "Internal error: ztuCompute is increasing tFuzz to %g"
#define MH_R1256 1256	// "Inconsistency in ztuMode: qsHvac (%g) != q2 (%g)"
#define MH_R1257 1257	// "Equal setpoints (%g) with equal priorities (%d) in zone '%s' -- \n"
			// "    %s and %s"
#define MH_R1258 1258	// "terminal '%s'"	/* two uses */
#define MH_R1259 1259	// "Cooling setpoint temp (%g) is less than heating setpoint (%g)\n"
			// "    for terminal '%s' of zone '%s'"
#define MH_R1260 1260	// "Bad Top.humMeth %d in cnztu.cpp:ZNR::ztuAbs"
              // 1261-1269 avail
// runtime errors: cnah 10-92
#define MH_R1270 1270	// "airHandler '%s': ahTsMn (%g) > ahTsMx (%g)"
#define MH_R1271 1271	// "airHandler '%s': ahFanCycles=YES not allowed when ahTsSp is ZN2"
#define MH_R1272 1272	// "airHandler '%s': more than one terminal\n"
			// "    not allowed when fan cycles"
#define MH_R1273 1273	// "airHandler '%s': fan cycles, but no control terminal:\n"
			// "    ahCtu = ... apparently missing"
#define MH_R1274 1274	// "Control terminal '%s' of airHandler '%s':\n"
			// "    terminal minumum flow (tuVfMn=%g) must be 0 when fan cycles"
#define MH_R1275 1275	// "airHandler '%s': frFanOn (%g) > 1.0"
#if 0		//not used 11-1-92, changed here 1-7-94
x #define MH_R1276 1276	// "airHandler '%s': convergence failure.\n"
x			// "   tr=%g  wr=%g  cr=%g  frFanOn=%g\n"
x			// "   po=%g  ts=%g  ws=%g  fanF=%g  ahMode=%d\n"
x			// "   trNx=%g  wrNx=%g  crNx=%g"
#else
//               1276 free
#endif
#define MH_R1277 1277	// "airHandler '%s': bad oaLimT 0x%lx"
#define MH_R1278 1278	// "airHandler '%s': bad oaLimE 0x%lx"
#define MH_R1279 1279	// "Internal error: Airhandler '%s': unrecognized ts sp control method 0x%lx"
#define MH_R1280 1280	// "airHandler '%s': ahTsSp is RA but no ahTsRaMn has been given"
#define MH_R1281 1281	// "airHandler '%s': ahTsSp is RA but no ahTsRaMx has been given"
#define MH_R1282 1282	// "airHandler '%s': ahTsSp is RA but no ahTsMn has been given"
#define MH_R1283 1283	// "airHandler '%s': ahTsSp is RA but no ahTsMx has been given"
#define MH_R1284 1284	// "ahTsSp for airHandler '%s' is ZN or ZN2 but no ahCtu has been given"
#define MH_R1285 1285	// "Unexpected use 0x%x for terminal [%d] for control zone [%d] for ah [%d]"
#define MH_R1286 1286	// "airHandler '%s': ahTsSp is '%s'\n    but control terminal '%s' has no %s"
#define MH_R1287 1287	// "airHandler '%s': ahTsSp is '%s'\n    but no control zone with terminal with %s found"
#define MH_R1288 1288	// "airHandler %s: antRatTs: internal error: not float mode as expected"
#define MH_R1289 1289	// "airHandler %s: Internal error in antRatTs: \n"
			// "    terminal %s is not terminal of zone's active zhx"
#define MH_R1290 1290	// "airHandler %s: Internal error in antRatTs: unexpected tu->useAr 0x%x"
#define MH_R1291 1291	// "Terminal '%s': tuVfMn is %g, not 0, when ahFanCyles = YES"
#define MH_R1292 1292	// "AH::setFrFanOn: airHandler '%s': unexpected 'uUseAr' 0x%x"
#define MH_R1293 1293	// "AH::setFrFanOn: airHandler '%s': \n""    cMxnx is 0 but crNx is non-0: %g"
#define MH_R1294 1294	// "airHandler '%s' not yet converged. Top.iter=%d\n"
			// "   tr=%g  wr=%g  cr=%g  frFanOn=%g\n"
			// "   po=%g  ts=%g  ws=%g  fanF=%g  ahMode=%d\n"
			// "   trNx=%g  wrNx=%g  crNx=%g\n"
			// "   DISREGARD THIS DEBUGGING MESSAGE UNLESS FOLLOWED BY\n"
			// "       \"Air handler - Terminals convergence failure\" error message."
#define MH_R1295 1295	// "airHandler '%s': ahTsSp is ZN (with ahFanCycles=YES)\n    but no ahTsMn has been given."	5-95
#define MH_R1296 1296	// "airHandler '%s': ahTsSp is ZN (with ahFanCycles=YES)\n    but no ahTsMx has been given."	5-95
              // 1297-1309 avail
// mostly runtime errors: cncoil.cpp 10-92
#define MH_R1310 1310	// "AirHandler %s's heat coil is scheduled on, \n    but heatPlant %s is scheduled off."
#define MH_R1311 1311	// "AirHandler %s's cool coil is scheduled on, \n    but coolPlant %s is scheduled off."
//(1312-1317 are cncoil.cpp setup time errors)
#define MH_R1312 1312	// "DX coil rated entering conditions are supersaturated."
#define MH_R1313 1313	// "DX coil capacity specifications yield supersaturated air at coil exit:\n"
			// "    full-load exit enthalpy (%g) greater than saturated air enthalpy\n"
			// "    (%g) at full-load exit temp (%g) at rated conditions."
#define MH_R1314 1314	// "Program Error: DX coil effective temp (te) at rated conditions\n"
			// "    seems to be greater than entering air temp"
#define MH_R1315 1315	// "DX coil effective temperature (intersection of coil process line with\n"
			// "    saturation line on psychro chart) at rated conditions is less than 0F.\n"
			// "    Possibly your ahccCaptRat is too large relative to ahccCapsRat."
#define MH_R1316 1316	// "airHandler '%s': \n"
			// "    DX coil setup effective point search convergence failure\n"
			// "        errTe=%g"
#define MH_R1317 1317	// "Program Error: DX coil effectiveness %g not in range 0 to 1"
//               1318,1319 avail
//cncoil runtime errors...
#define MH_R1320 1320	// "airHandler '%s': Air entering DX cooling coil is supersaturated:\n"
			// "    ten = %g  wen = %g.  Coil model won't work."
#define MH_R1321 1321	// "airHandler '%s': DX coil effectiveness %g not in range 0 to 1"
#define MH_R1322 1322	// "airHandler '%s' DX coil inconsistency:\n"
			// "    he is %g but h(te=%g,we=%g) is %g.  wena=%g. cs1=%d"
#define MH_R1323 1323	// "airHandler '%s':\n"
			// "    DX coil full-load exit state convergence failure.\n"
			// "      entry conditions: ten=%g  wen=%g  plrVf=%g\n"
			// "      unfinished results: te=%g  we=%g  last we=%g\n"
			// "          wena=%g  last wena=%g  next wena=%g"
#define MH_R1324 1324	// "airHandler '%s' DX cool coil inconsistency: \n"
			// "    slopef = %g but slopee = %g.\n"
			// "           wen=%g wena=%g we=%g wexf=%g;\n"
			// "           ten=%g texf=%g;  cs=%d,%d"
#define MH_R1325 1325	// "airHandler '%s':\n"
			// "    DX coil exit humidity ratio convergence failure.\n"
			// "      inputs:  ten=%g  wen=%g  tex=%g\n"
			// "      unfinished result: wex=%g  last wex=%g\n"
			// "          teCOn=%g    weSatCOn=%g\n"
			// "          plr=%g  frCOn=%g  last frCOn=%g"
#define MH_R1326 1326	// "airHandler %s: Inconsistency #1: total capacity (%g) not negative"
#define MH_R1327 1327	// "airHandler %s: Inconsistency #1a: sensible capacity (%g) not negative"
#define MH_R1328 1328	// "airHandler %s: Inconsistency #2: sensible capacity (%g)\n"
			// "    larger than total capacity (%g)"
#define MH_R1329 1329	// "airHandler %s: Inconsistency #3: wex (%g) > wen (%g)"
#define MH_R1330 1330	// "airHandler %s: Inconsistency #4: tex (%g) > ten (%g)"
#define MH_R1331 1331	// "airHandler %s: Inconsistency #5: teCOn (%g) < te (%g)\n"
			// "           wen=%g wena=%g we=%g wexf=%g;  ten=%g texf=%g\n"
			// "           ql=%g qs=%g q=%g;   plr=%g plrSens=%g;   case=%d,%d"
#define MH_R1332 1332	// "airHandler %s: Inconsistency #6: weSatCOn (%g) < we (%g)\n"
			// "           wen=%g wena=%g we=%g wexf=%g;  ten=%g texf=%g\n"
			// "           ql=%g qs=%g q=%g;   plr=%g plrSens=%g;   case=%d,%d"
#define MH_R1333 1333	// "airHandler %s: Inconsistency #7: wex (%g) < weSatCOn (%g)"
#define MH_R1334 1334	// "airHandler %s: Inconsistency #8: wex (%g) < we (%g)"
#define MH_R1335 1335	// "airHandler %s: Inconsistency #9: wex (%g) < wexf (%g)"
#define MH_R1336 1336	// "airHandler '%s':\n"
			// "    heat coil capacity 'ahhcCaptRat' (%g) not > 0"
#define MH_R1337 1337	// "airHandler '%s':\n"
			// "    heat coil full-load energy input ratio 'ahhcEirR' (%g) not > 1.0"
#define MH_R1338 1338	// "cncoil.cpp:AH::coilsEndSubhr: Bad heat coil type 0x%x"
#define MH_R1339 1339	// "cncoil.cpp:AH::coilsEndSubhr: Bad cool coil type 0x%x"
#define MH_R1340 1340	// "airHandler %s: frozen CHW cooling coil:\n    leaving water temperature computed as %g F or less"  12-3-92
//cncoil additions 6-95
#define MH_R1341 1341	/* "airHandler %s: Internal error in doAhpCoil: \n"
                           "    arg to sqrt (%g) for quad formula hlf not >= 0.\n"
                           "        cdm %g   pDfrhCon %g   capCon %g   qWant %g\n"
                           "        A %g   B %g   C %g\n" */
#define MH_R1342 1342	/* "airHandler %s: Internal error in doAhpCoil: \n"
                           "    hlf (%g) not in range 0 < hlf <= qWant/capCon (%g)" */
#define MH_R1343 1343	/* "airHandler %s: Internal error in doAhpCoil: \n"
                           "    plf (%g) not > 0" */
#define MH_R1344 1344	/* "airHandler %s: Internal error in doAhpCoil: \n"
                           "    frCprOn (%g) > frFanOn (%g)" */
#define MH_R1345 1345	/* "airHandler %s: Internal error in doAhpCoil: \n"
                           "    q = %g but qWant is %g -- should be the same" */
               //1346-1349 avail
// cnhp.cpp runtime errors 10-92
#define MH_R1350 1350	// "heatPlant %s overload failure: q (%g) > stgCap[stgMxQ] (%g)"
#define MH_R1351 1351	// "heatPlant %s: bad stage number %d: not in range 0..%d"
	       //1352-1359 avail
// cncp.cpp runtime errors 6-95
#define MH_R1360 1360	// "COOLPLANT '%s': cpTsSp (%g F) is below freezing"
#define MH_R1361 1361	// "COOLPLANT '%s': \n    return water temperature (%g F) from coils is below freezing"
#define MH_R1362 1362	// "COOLPLANT '%s' stage %d chiller capacity (%g)\n    is less than pump heat (%g) (ts=%g, tCnd=%g)"
#define MH_R1363 1363	// "CHILLER '%s' of COOLPLANT '%s': \n    delivered water temp (%g F) is below freezing"
#define MH_R1364 1364	// "COOLPLANT %s: bad stage number %d: not in range 1..%d"
               //1365-1369 avail
// cntp.cpp runtime errors 6-95
#define MH_R1370 1370	// "Frozen towerPlant '%s': supply temp tpTs = %gF"
#define MH_R1371 1371	// "Boiling towerPlant '%s': supply temp tpTs = %gF"
#define MH_R1372 1372	// "TowerPlant '%s': varSpeedF() convergence failure, %d iterations \n    qWant=%g  q=%g  f=%g"
               //1373-1379 avail
// 1380-1799 avail.
/*
#define MH_R1370 1370	//
#define MH_R1371 1371	//
#define MH_R1372 1372	//
#define MH_R1373 1373	//
#define MH_R1374 1374	//
#define MH_R1375 1375	//
#define MH_R1376 1376	//
#define MH_R1377 1377	//
#define MH_R1378 1378	//
#define MH_R1379 1379	//
#define MH_R1380 1380	//
#define MH_R1381 1381	//
#define MH_R1382 1382	//
#define MH_R1383 1383	//
#define MH_R1384 1384	//
#define MH_R1385 1385	//
#define MH_R1386 1386	//
#define MH_R1387 1387	//
#define MH_R1388 1388	//
#define MH_R1389 1389	//
#define MH_R1390 1390	//
#define MH_R1391 1391	//
#define MH_R1392 1392	//
#define MH_R1393 1393	//
#define MH_R1394 1394	//
#define MH_R1395 1395	//
#define MH_R1396 1396	//
#define MH_R1397 1397	//
#define MH_R1398 1398	//
#define MH_R1399 1399	//
#define MH_R1400 1400	//
#define MH_R1401 1401	//
#define MH_R1402 1402	//
#define MH_R1403 1403	//
#define MH_R1404 1404	//
#define MH_R1405 1405	//
#define MH_R1406 1406	//
#define MH_R1407 1407	//
#define MH_R1408 1408	//
#define MH_R1409 1409	//
*/
// 1800-1899 being used above when more numbers in 0800-0899 area needed, 6-95.

// impf.cpp: import files (runtime) errors, 6-95
#define MH_R1901 1901	// "Cannot open import file %s. No run."
#define MH_R1902 1902	// "Tell error (%ld) on import file %s. No run."
#define MH_R1903 1903	// "Seek error (%ld) on import file %s, handle %d"
#define MH_R1904 1904	// "Warning: import File %s has too many lines. \n    Text at at/after line %d not used."
#define MH_R1905 1905	// "%s(%d): Internal error:\n    Import file names table record subscript %d out of range 1 to %d."
#define MH_R1906 1906	// "%s(%d): Internal error:\n    Import file subscript %d out of range 1 to %d."
#define MH_R1907 1907	// "%s(%d): Internal error:\n    Import file %s was not opened successfully."
#define MH_R1908 1908	// "%s(%d): End of import file %s: data previously used up.%s"
#define MH_R1909 1909	// "\n    File must contain enough data for CSE warmup days (default 7)."
#define MH_R1910 1910	// "Internal error: ImpfldDcdr::axscanFnm():\n    called w/o preceding successful file access"
#define MH_R1911 1911	// "%s(%d): Internal error:\n    no IFFNM.fnmt pointer for Import file %s"
#define MH_R1912 1912	// "%s(%d): Internal error: in IMPORT() from file %s\n    field name index %d out of range 1 to %d."
#define MH_R1913 1913	// "%s(%d): Internal error:\n    in IMPORT() from file %s field %s (name index %d),\n    field number %d out of range 1 to %d."
#define MH_R1914 1914	// "%s(%d): Too few fields in line %d of import file %s:\n      looking for field %s (field # %d), found only %d fields."
#define MH_R1915 1915	// "Internal error: ImpfldDcdr::axscanFnr():\n    called w/o preceding successful file access"
#define MH_R1916 1916	// "%s(%d): Internal error: in IMPORT() from file %s, \n    field number %d out of range 1 to %d."
#define MH_R1917 1917	// "%s(%d): Too few fields in line %d of import file %s:\n      looking for field %d, found only %d fields."
#define MH_R1918 1918	// "Internal error: ImpfldDcdr::decNum():\n    called w/o preceding successful field scan"
#define MH_R1919 1919	// "non-numeric value"
#define MH_R1920 1920	// "garbage after numeric value"
#define MH_R1921 1921	// "number out of range"
#define MH_R1922 1922	// "%s(%d): \n    Import file %s, line %d, field %s:\n      %s:\n          \"%s\""
#define MH_R1923 1923	// "Warning: Import file %s: title is %s not %s."
#define MH_R1924 1924	// "Import file %s: bad header format. No Run."
#define MH_R1925 1925	// "Warning: Import file %s: \n    File header says frequency is %s, not %s."
#define MH_R1926 1926	// "Warning: Import file %s: Incorrect header format."
#define MH_R1927 1927	// "Import file %s:\n    The following field name(s) were used in IMPORT()s\n    but not found in import file %s:\n        %s  %s  %s  %s  %s"
#define MH_R1928 1928	// "Import file %s: \n    Premature end-of-file or error while reading header. No Run."
#define MH_R1929 1929	// "Import file %s, line %d: \n    record longer than %d characters"
#define MH_R1930 1930	// "Import file %s, line %d: second quote missing"
#define MH_R1931 1931	// "Read error on import file %s"
#define MH_R1932 1932	// "Close error on import file %s"
//1933-1959 believed available

// brfw.cpp: binary results file writing (runtime) errors, 11-94.
//brfw.cpp:ResfWriter messages
#define MH_R1960 1960	// "ResfWriter: months not being generated sequentially"
#define MH_R1961 1961	// "ResfWriter::endMonth not called before next startMonth"
#define MH_R1962 1962	// "ResfWriter::startMonth not called before endMonth"
#define MH_R1963 1963	// "startMonth/endMonth out of sync with setHourInfo" 2 uses
//brfw 6-95 additions
#define MH_R1964 1964	// "ResfWriter::create: 'pNam' is NULL, \n    but file must be specified under DOS."
#define MH_R1965 1965	/* "ResfWriter::create: 'keePak' is non-0, \n"
                           "    but memory handle return NOT IMPLEMENTED under DOS." */
#define MH_R1966 1966	// "ResfWriter::close: 'hans' argument given\n    but 'keePak' was FALSE at open"
#define MH_R1967 1967	/* "ResfWriter::startMonth: repeating month %d.\n"
                           "    Run cannot end in same month it started in \n"
                           "    when creating hourly binary results file." */
//#define MH_R1968 1968	//
//brfw.cpp:BinBlock messages
#define MH_R1969 1969	// "BinBlock::init: 'discardable' and 'keePak' arguments should be 0 under DOS" // 6-95
#define MH_R1970 1970	// "Out of memory for binary results file buffer"
#define MH_R1971 1971	// "Lock failure in BinBlock::alLock re binary results file"
#define MH_R1972 1972	// "Out of memory for binary results file packing buffer"
#define MH_R1973 1973	// "Lock failure in BinBlock::write re binary results file"
#define MH_R1974 1974	// "GlobalFree failure in BinBlock::write"
#define MH_R1975 1975	// "GlobalFree failure in BinBlock::free"
#define MH_R1976 1976	// "GlobalFree failure (packed block) in BinBlock::free"
#define MH_R1977 1977	// "BinBlock: Request to write block but file not open"
#define MH_R1978 1978	// "BinBlock: handle return for unpacked blocks not implemented" // 6-95
//#define MH_R1979 1979	//
//brfw.cpp:BinFile messages
#define MH_R1980 1980	// "Create failure, file %s"
#define MH_R1981 1981	// "Close failure, file %s"
#define MH_R1982 1982	// "Seek failure, file %s"
#define MH_R1983 1983	// "Write error on file %s -- disk full?"
//1984-1987 free
//brfw.cpp non-mbr fcn msgs
#define MH_R1988 1988	// "In brfw.cpp::%s: NULL source pointer "
#define MH_R1989 1989	// "In brfw.cpp::%s: NULL destination pointer"

// --- IO errors ---
// xiopak.cpp --
#define MH_I0001  2001
#define MH_I0002  2002
#define MH_I0003  2003
#define MH_I0004  2004
#define MH_I0005  2005
#define MH_I0006  2006
#define MH_I0007  2007
#define MH_I0008  2008
#define MH_I0009  2009
#define MH_I0010  2010
#define MH_I0011  2011
//#define MH_I0012  2012	// "I0012: IO error, file '%s', %s". In ram cuz used re opening msg file.

#define MH_I0021  2021
#define MH_I0022  2022
#define MH_I0023  2023
#define MH_I0024  2024
#define MH_I0025  2025
#define MH_I0026  2026
#define MH_I0027  2027

#define MH_I0031  2031

#define MH_I0101  2101	// "Cannot open"
#define MH_I0102  2102	// "Cannot create"
#define MH_I0103  2103	// "Close error on"
#define MH_I0104  2104	// "Seek error on"  3 uses
#define MH_I0105  2105	// "Write error on" 2 uses
#define MH_I0106  2106	// "Read error on"
#define MH_I0107  2107	// "Unexpected end of file in"
#define MH_I0108  2108	// "Writing to file not opened for writing:" 2 uses
//used with errFlLn
#define MH_I0109  2109	// "Unexpected end of %s."
#define MH_I0110  2110	// "Token too long."
#define MH_I0111  2111	// "Internal error: invalid character '%c' in control string in call to YACAM::get()."
#define MH_I0112  2112	// "Overlong string will be truncated to %d characters (from %d):\n    \"%s\"."
#define MH_I0113  2113	// "Invalid date \"%s\"."
#define MH_I0114  2114	// "Missing or invalid number \"%s\"."
#define MH_I0115  2115	// "Integer too large: \"%s\"."
#define MH_I0116  2116	// "Unrecognized character(s) after %snumber \"%s\"."
#define MH_I0117  2117	// "Internal error: Invalid call to YACAM::get(): \n    NULL not found at end arg list.\n"
//errFlLn meta-message, no msg number:
#define MH_I0118  2118	// "Error in %s %s near line %d:\n  %s"
//#define MH_I0119  2119	//
//#define MH_I0120  2120	//

// end of msghans.h
