# User-defined Functions

User defined functions have the format:

        type FUNCTION name ( arg decls ) = expr ;

*Type* indicates the type of value the function returns, and can be:

        INTEGER
        FLOAT
        STRING
        DOY       (day of year date using month name and day; actually same as integer).

*Arg decls* indicates zero or more comma-separated argument declarations, each consisting of a *type* (as above) and the name used for the argument in *expr*.

*Expr* is an expression of (or convertible to) *type*.

The tradeoffs between using a user-defined function and a preprocessor macro (`#define`) include:

1.  Function may be slightly slower, because its code is always kept separate and called, while the macro expansion is inserted directly in the input text, resulting in inline code.

2.  Function may use less memory, because only one copy of it is stored no matter how many times it is called.

3.  Type checking: the declared types of the function and its arguments allow CSE to perform additional checks.

Note that while macros require line-splicing ("\\") to extend over one line, functions do not require it:

        // Function returning number of days in ith month of year:
        DOY FUNCTION MonthLU (integer i) = choose1 ( i , Jan 31, Feb 28, Mar 31,
                                                         Apr 30, May 31, Jun 30,
                                                         Jul 31, Aug 31, Sep 30,
                                                         Oct 31, Nov 30, Dec 31 ) ;
        // Equivalent preprocessor macro:
        #define MonthLU (i) = choose1 ( i , Jan 31, Feb 28, Mar 31,  \
                                            Apr 30, May 31, Jun 30,  \
                                            Jul 31, Aug 31, Sep 30,  \
                                            Oct 31, Nov 30, Dec 31 ) ;
