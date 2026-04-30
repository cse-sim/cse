# The Preprocessor

*Note: The organization and wording of this section is based on section A12 of Kernigan and Richie \[1988\]. The reader is referred to that source for a somewhat more rigorous presentation but with the caution that the CSE input language preprocessor does not **completely** comply to ANSI C specifications.*

The preprocessor performs macro definition and expansion, file inclusion, and conditional inclusion/exclusion of text. Lines whose first non-whitespace character is `#` communicate with the preprocessor and are designated *preprocessor directives*. Line boundaries are significant to the preprocessor (in contrast to the rest of the input language in which a newline is simply whitespace), although adjacent lines can be spliced with \\, as discussed below. The syntax of preprocessor directives is separate from that of the rest of the language. Preprocessor directives can appear anywhere in an input file and their effects last until the end of the input file. The directives that are supported by the input language preprocessor are the following:

        #if
        #else
        #elif
        #endif
        #ifndef

        #define
        #redefine
        #undef

        #include

## Line splicing

If the last character on a line is the backslash \\, then the next line is spliced to that line by elimination of the backslash and the following newline. Line splicing occurs *before* the line is divided into tokens.

Line splicing finds its main use in defining long macros:

        // hourly light gain values:
        #define LIGHT_GAIN       .024, .022, .021, .021, .021, .026, \
                                 .038, .059, .056, .060, .059, .046, \
                                 .045, .5  , .5  , .05 , .057, .064, \
                                 .064, .052, .050, .055, .044, .027

## Macro definition and expansion

A directive of the form

        #define _identifier_ _token-sequence_

is a macro definition and causes the preprocessor to replace subsequent instances of the identifier with the given token sequence. Note that the token string can be empty (e.g. `#define FLAG`).

A line of the form

        #define _identifier_( _identifier-list_) _token-sequence_

where there is no space between the identifier and the (, is a macro with parameters given by the identifier list. The expansion of macros with parameters is discussed below.

Macros may also be defined *on the CSE command line*, making it possible to vary a run without changing the input files at all. As described in the [command line][command-line] section, macros are defined on the CSE command line using the `-D` switch in the forms

        -D_identifier_

        -D_identifier_=_token-sequence_

The first form simply defines the name with no token-sequence; this is convenient for testing with `#ifdef`, `#ifndef`, or `defined()`, as described in the section on [conditional inclusion of tex][conditional-inclusion-of-text]. The second form allows an argument list and token sequence. The entire command line argument must be enclosed in quotes if it contains any spaces.

A macro definition is forgotten when an `#undef` directive is encountered:

        #undef _identifier_

It is not an error to `#undef` an undefined identifier.

A macro may be re-`#defined` without a prior `#undef` unless the second definition is identical to the first. A combined `#undef`/`#define` directive is available to handle this common case:

        #redefine _identifier_ _token-sequence_

        #redefine _identifier_( _identifier-list_) _token-sequence_

When a macro is `#redefined`, it need not agree in form with the prior definition (that is, one can have parameters even if the other does not). It is not an error to `#redefine` an undefined identifier.

Macros defined in the second form (with parameters) are expanded whenever the preprocessor encounters the macro identifier followed by optional whitespace and a comma-separated parameter list enclosed in parentheses. First the comma separated token sequences are collected; any commas within quotes or nested parentheses do not separate parameters. Then each unquoted instance of the each parameter identifier in the macro definition is replaced by the collected tokens. The resulting string is then repeatedly re-scanned for more defined identifiers. The macro definition and reference must have the same number of arguments.

It is often important to include parentheses within macro definitions to make sure they evaluate properly in all situations. Suppose we define a handy area macro as follows:

        #define AREA(w, h) w*h        // WRONG

Consider what happens when this macro is expanded with arguments 2+3 and 4+1. The preprocessor substitutes the arguments for the parameters, then the input language processor processes the statement containing the macro expansion without regard to the beginning and end of the arguments. The expected result is 25, but as defined, the macro will produce a result of 15. Parentheses fix it:

        #define AREA(w, h) ((w)*(h))  // RIGHT

The outer enclosing set of parentheses are not strictly needed in our example, but are good practice to avoid evaluation errors when the macro expands within a larger expression.

Note 1: The CSE preprocessor does not support the ANSI C stringizing (\#) or concatenation (\#\#) operators.

Note 2: Identifiers are case *insensitive* (unlike ANSI C). For example, the text “myHeight” will be replaced by the `#defined` value of MYHEIGHT (if there is one).

*The preprocessor examples at the end of this section illustrate macro definition and expansion.*

## File inclusion

Directives of the form

`#include "filename"` and

`#include <filename>`

cause the replacement of the directive line with the entire contents of the referenced file. If the filename does not include an extension, a default extension of .INP is assumed. The filename may include path information; if it does not, the file must be in the current directory.

`#include`s may be nested to a depth of 5.

For an example of the use `#include`s, please see the preprocessor examples at the end of this section.

## Conditional inclusion of text

Conditional text inclusion provides a facility for selectively including or excluding groups of input file lines. The lines so included or excluded may be either CSE input language text *or other preprocessor directives*. The latter capability is very powerful.

Several conditional inclusion directive involve integer constant expressions. Constant integer expressions are formed according the rules discussed in the section on [expressions][expressions] with the following changes:

1.  Only constant integer operands are allowed.

2.  All values (including intermediate values computed during expression evaluation) must remain in the 16 bit range (-32768 - 32767). The expression processor treats all integers as signed values and requires signed decimal constants -- however, it requires unsigned octal and hexadecimal constants. Thus decimal constants must be in the range -32768 - 32767, octal must be in the range 0 - 0o177777, and hexadecimal in the range 0 - 0xffff. Since all arithmetic comparisons are done assuming signed values, 0xffff &lt; 1 is true (unhappily). Care is required when using the arithmetic comparison operators (&lt;, &lt;=, &gt;=, &gt;).

3.  The logical relational operators && and || are not available. Nearly equivalent function can be obtained with & and |.

4.  A special operand defined( ) is provided; it is described below.

Macro expansion *is* performed on constant expression text, so symbolic expressions can be used (see examples below).

The basic conditional format uses the directive

        #if _constant-expression_

If the constant expression has the value 0, all lines following the `#if` are dropped from the input stream (the preprocessor discards them) until a matching `#else`, `#elif`, or `#endif` directive is encountered.

The defined( *identifier* ) operand returns 1 if the identifier is the name of a defined macro, otherwise 0. Thus

        #if defined( _identifier_ )

can be used to control text inclusion based on macro flags. Two `#if` variants that test whether a macro is defined are also available. `#ifdef` *identifier* is equivalent to `#if` defined(*identifier*) and `#ifndef` *identifier* is equivalent to `#if` !defined(*identifier*).

Defined(), `#ifdef`, and `#ifndef` consider a macro name "defined" even if the body of its definition contains no characters; thus a macro to be tested with one of these can be defined with just

        #define _identifier_

or with just "-D*identifier*" on the CSE command line.

Conditional blocks are most simply terminated with `#endif`, but `#else` and `#elif` *constant-expression* are also available for selecting one of two or more alternative text blocks.

The simplest use of `#if` is to "turn off" sections of an input file without editing them out:

        #if 0
        This text is deleted from the input stream.
        #endif

Or, portions of the input file can be conditionally selected:

        #define FLRAREA 1000   // other values used in other runs
        #if FLRAREA <= 800
            CSE input language for small zones
        #elif FLRAREA <= 1500
            CSE input language for medium zones
        #else
            CSE input language for large zones
        #endif

Note that if a set of `#if` ... `#elif` ... `#elif` conditionals does not contain an `#else`, it is possible for all lines to be excluded.

Finally, it is once again important to note that conditional directives *nest*, as shown in the following example (indentation is included for clarity only):

        #if 0
            This text is NOT included.
            #if 1
                This text is NOT included.
            #endif
        #else
            This text IS included.
        #endif

## Input echo control

By default, CSE echos all input text to the input echo report (see REPORT rpType=INP).  #echooff and #echoon allow disabling and re-enabling text echoing.  This capability is useful reducing
report file size by suppressing echo of, for example, large standard include files.

        ... some input ...   // text sent to the input echo report
        #echooff
           // This text will NOT be sent to the input echo report.
           // However, it IS read and used by CSE.
           // Error messages will be echoed even if #echooff
           ... more input ...
        #echoon         // restore echo

Nesting is supported -- each #echoon "undoes" the prior #echooff, but echoing
does not resume until the topmost (earliest) #echooff is cancelled.
* #echoon has no effect when echoing is already active.
* Unmatched #echooffs are ignored -- echoing remains disabled through the end
of the input stream.


## Preprocessor examples

This section shows a few combined examples that demonstrate the preprocessor's capabilities.

The simplest use of macros is for run parameterization. For example, a base file is constructed that derives values from a macro named FLRAREA. Then multiple runs can be performed using `#include`:

        // Base file
        ... various input language statements ...

        ZONE main
            znArea = FLRAREA
            znVol  = 8*FLRAREA
            znCAir = 2*FLRAREA ...
            ... various other input language statements ...

        RUN

        CLEAR

The actual input file would look like this:

        // Run with zone area = 500, 1000, and 2000 ft2
        #define FLRAREA 500
        #include "base."
        #redefine FLRAREA 1000
        #include "base."
        #redefine FLRAREA 2000
        #include "base."

Macros are also useful for encapsulating standard calculations. For example, most U-values must be entered *without* surface conductances, yet many tabulated U-values include the effects of the standard ASHRAE winter surface conductance of 6.00 Btuh/ft^2^-^o^F. A simple macro is very helpful:

        #define UWinter(u)  ( 1/(1/(u)-1/6.00) )

This macro can be used whenever a U-value is required (e.g. SURFACE ... sfU=UWinter(.11) ... ).
