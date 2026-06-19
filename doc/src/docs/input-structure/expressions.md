# Expressions

Probably the CSE input language's most powerful characteristic is its ability to accept expressions anywhere a single number, string, object name, or other value would be accepted. Preceding examples have shown the inputting zone areas and volumes as numbers (some defined via preprocessor macros) with \*'s between them to signify multiplication, to facilitate changes and avoid errors that might occur in manual arithmetic. Such expressions, where all operands are constants, are acceptable *anywhere* a constant of the same type would be allowed.

But for many object members, CSE accepts *live expressions* that *vary* according to time of day, weather, zone temperatures, etc. (etc., etc., etc.!). Live expressions permit simulation of many relationships without special-purpose features in the language. Live expressions support controlling setpoints, scheduling HVAC system operation, resetting air handler supply temperature according to outdoor temperature, and other necessary and foreseen functions without dedicated language features; they will also support many unforeseen user-generated functionalities that would otherwise be unavailable.

Additional expression flexibility is provided by the ability to access all of the input data and much of the internal data as operands in expressions (*probes*, see [this section][probes]).

As in a programming language, CSE expressions are constructed from operators and operands; unlike most programming languages, CSE determines how often an expression's operands change and automatically compute and store the value as often as necessary.

Expressions in which all operands are known when the statement is being decoded (for example, if all values are constants) are *always* allowed, because the input language processor immediately evaluates them and presents the value to the rest of the program in the same manner as if a single number had been entered. *Most* members also accept expressions that can be evaluated as soon as the run's input is complete, for example expressions involving a reference to another member that has not been given yet. Expressions that vary during the run, say at hourly or daily intervals, are accepted by *many* members. The *variability* or maximum acceptable variation for each member is given in the descriptions in the [input data][input-data] section, and the *variation* of each non-constant expression component is given in its description in this section.

<span class="underline">Interaction of expressions and the preprocessor</span>: Generally, they don't interact. The preprocessor is a text processor which completes its work by including specified files, deleting sections under false \#if's, remembering define definitions, replacing macro calls with the text of the definition, removing preprocessor directives from the text after interpreting them, etc., *then* the resulting character stream is analyzed by the input language statement compiler. However, the if statement takes an integer numeric expression argument. This expression is similar to those described here except that it can only use constant operands, since the preprocessor must evaluate it before deciding what text to feed to the input statement statement compiler.

## Expression Types

The type of value to which an expression must evaluate is specified in each member description (see the [input data][input-data] section) or other context in which an expression can be used. Each expression may be a single constant or may be made up of operators and operands described in the rest of this section, so long as the result is the required type or can be converted to that type by CSE, and its variation is not too great for the context. The possible types are:



{{
    csv_table("  *float*,            A real number (3.0&comma; 5.34&comma; -2.&comma; etc.). Approximately 7 digits are carried internally. If an int is given where a real is required &comma; it is automatically converted.
  *int*,              An integer or whole number (-1&comma; 0&comma; 1&comma; 2 etc.). If a real is given&comma; an error may result&comma; but we should change it to convert it (discarding any fractional part).
  *Boolean*,          Same as int; indicates that a 0 value will be interpreted as \"false\" and any non-0 value will be interpreted as \"true\".
  *string*,          A string of characters; for example&comma; some text enclosed in quotes.
  *object name*,      Name of an object of a specified class. Differs from *string* in that the name need not be enclosed in quotes if it consists only of letters&comma; digits&comma; _&comma; and $&comma; begins with a non-digit&comma; and is different from all reserved words now in or later added to the language (see [Object Names][object-names]).<br>The object may be defined after it is referred to. An expression using conditional operators&comma; functions&comma; etc. may be used provided its value is known when the RUN action command is reached.; no members requiring object names accept values that vary during the simulation.
  *choice*,           One of several choices; a list of the acceptable values is given wherever a *choice* is required. The choices are usually listed in CAPITALS but may be entered in upper or lower case as desired. As with object names&comma; quotes are allowed but not required.<br>Expressions may be used for choices&comma; subject to the variability of the context.
  *date*,             May be entered as a 3-letter month abbreviation followed by an *int* for the day of the month&comma; or an *int* for the Julian day of the year (February is assumed to have 28 days). Expressions may be used subject to variability limitations. Examples:<br>&nbsp;&nbsp;&nbsp;&nbsp;`Jan 23   // January 23`<br>&nbsp;&nbsp;&nbsp;&nbsp;`23       // January 23`<br>&nbsp;&nbsp;&nbsp;&nbsp;`32       // February 1`")
}}

[](){ #numeric-any-type }
These words are used in following descriptions of contexts that can accept more than one basic type:

{{
    csv_table("*numeric*, *float* or *int*; When floats and ints are intermixed with the same operator or function&comma; the result is float.
*anyType*, Any type; the result is the same type as the argument. If floats and ints are intermixed&comma; the result is float. If strings and valid choice names are intermixed&comma; the result is *choice*. Other mixtures of types are generally illegal&comma; except in expressions for a few members that will accept either one of several choices or a numeric value.")
}}

The next section describes the syntax of constants of the various data types; then, we will describe the available operators, then other operand types such as system variables and built-in functions.

## Constants

This section reviews how to enter ordinary non-varying numbers and other values.

{{
    csv_table("*int*, optional - sign followed by digits. Don't use a decimal point if your intent is to give an *int* quantity -- the decimal point indicates a *float* to CSE. Hexadecimal and Octal values may be given by prefixing the value with `0x` and `0O` respectively (yes&comma; that really is a zero followed by an 'O').
  *float*, optional - sign&comma; digits&comma; and decimal point. Very large or small values can be entered by following the number with an \"e\" and a power of ten. Examples:<br>&nbsp;&nbsp;&nbsp;&nbsp;`1.0  1.  .1  -5534.6  123.e25  4.56e-23`<br> The decimal point indicates a float as opposed to an int.<br>Generally it doesn't matter as CSE converts ints to floats as required&comma; but be careful when dividing: CSE interprets \"2/3\" as integer two divided by integer 3&comma; which will produce an integer 0 before CSE notices any need to convert to *float*. If you mean .6666667&comma; say 2./3&comma; 2/3.&comma; or .6666667.
  *feet and inches*,  Feet and inches may be entered where a *float* number of feet is required by typing the feet (or a 0 if none)&comma; a single quote '&comma; then the inches. (Actually this is an operator meaning \"divide the following value by 12 and add it to the preceding value\"&comma; so expressions can work with it.) Examples:<br>&nbsp;&nbsp;&nbsp;&nbsp;`3'6  0'.5 (10+20)'(2+3)`
  *string*,          \"Text\" -- desired characters enclosed in double quotes.<br>Maximum length 80 characters (make 132??). To put a \" within the \"'s&comma; precede it with a backslash. <br>Certain control codes can be represented with letters preceded with a backslash as follows:<br>&nbsp;&nbsp;&nbsp;&nbsp;`\\e    escape`<br>&nbsp;&nbsp;&nbsp;&nbsp;`\\t    tab`<br>&nbsp;&nbsp;&nbsp;&nbsp;`\\f    form feed`<br>&nbsp;&nbsp;&nbsp;&nbsp;`\\r    carriage return`<br>&nbsp;&nbsp;&nbsp;&nbsp;`\\n    newline or line feed`
  *object name*, Same as *string*&comma; or without quotes if name consists only of letters&comma; digits&comma; _&comma; and $&comma; begins with a non-digit&comma; and is different from all reserved words now in or later added to the language (see [Object Names][object-names]). Control character codes (ASCII 0-31) are not allowed.
  *choice*, Same as string; quotes optional on choice words valid for the member. Capitalization does not matter.
  *date*,Julian day of year (as *int* constant)&comma; or month abbreviation<br>&nbsp;&nbsp;&nbsp;&nbsp;`Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec`<br> followed by the *int* day of month. (Actually&comma; the month names are operators implemented to add the starting day of the month to the following *int* quantity).")
}}

## Operators

For *floats* and *ints*, the CSE input language recognizes a set of operators based closely on those found in the C programming language. The following table describes the available numeric operators. The operators are shown in the order of execution (precedence) when no ()'s are used to control the order of evaluation; thin lines separate operators of equal precedence.

--- 
**Feet-Inches Separator** `'`
:   `a ' b` yields `a + b/12`

    Example: `4'6 = 4.5`

**Unary plus** `+`
:   The familiar "positive", as in `+3`. Does nothing; rarely used.

**Unary minus** `-`
:   The familiar "minus", as in `-3`.
    
    Example: `-(-3) = +3`

**Logical NOT** `!`
:   Changes `0` to `1` and any non-`0` value to `0`.

    Examples: `!0 = 1`, `!17 = 0`

**One's Complement** `~`
:   Complements each bit in an *int* value.

**Multiplication** `*`  
:   Multiplication
    
    Examples: `3*4 = 12`, `3.24*18.54 = 60.07`


**Division** `/`  
:   Division
    
    Examples: `4/2 = 2`, `3.24/1.42 = 2.28`

    !!! danger "CAUTION!"
        Integer division truncates toward 0: `3/2 = 1`, `3/-2 = -1`, `-3/2 = -1`, `2/3 = 0`
    
**Modulus** `%`
: Yields the remainder after division, e.g. `7%2 = 1`. The result has the same sign as the left operand (e.g.`(-7)%2 = -1`). `%` is defined for both integer and floating point operands (unlike ANSI C).

**Addition** `+`
:   Yields the sum of the operands

    Example: `5+3=8`

**Subtraction** `-`
:   Yields the difference of the operands

    Example: `5-3=2`

**Right Shift** `>>`  
:   `a >> b` yields `a` shifted right `b` bit positions

    Example: `8 >> 2 = 2`

**Left Shift** `<<`  
:   `a << b` yields `a` shifted left `b` bit positions
    
    Example: `8 << 2 = 32`

**Less than** `<`  
:   `a < b` yields 1 if `a` is less than `b`, otherwise 0

**Less than or equal** `<=`  
:   `a <= b` yields 1 if `a` is less than or equal to `b`, otherwise 0

**Greater than or equal** `>=`  
:   `a >= b` yields 1 if `a` is greater than or equal to `b`, otherwise 0

**Greater than** `>`  
:   `a > b` yields 1 if `a` is greater than `b`, otherwise 0

**Equal** `==`  
:   `a == b` yields 1 if `a` is *exactly* (bit wise) equal to `b`, otherwise 0

**Not equal** `==`  
:   `a != b` yields 1 if `a` is not equal to `b`, otherwise 0

**Bitwise AND** `&`  
:   `a & b` yields the bitwise AND of the operands

    Example: `6 & 2 = 2`

**Bitwise EXCLUSIVE OR** `^`  
:   `a ^ b` yields the bitwise XOR of the operands

    Example: `6 ^ 2 = 4`

**Bitwise INCLUSIVE OR** `|`  
:   `a | b` yields the bitwise IOR of the operands

    Example: `6 | 2 = 6`

**Logical AND** `&&`  
:   `a && b` yields 1 if both `a` and `b` are non-zero, otherwise 0.
    `&&` guarantees left to right evaluation: if the first operand evaluates to 0, the second operand is not evaluated and the result is 0.

**Logical OR** `||`  
:   `a || b` yields 1 if either `a` or `b` is true (non-0), otherwise 0.
    `||` guarantees left to right evaluation: if the first operand evaluates to non-zero, the second operand in not evaluated and the result is 1.

**Conditional** `?:`  
:   `a ? b : c` yields `b` if `a` is true (non-0), otherwise `c`.
 
---

*Dates* are stored as *ints* (the value being the Julian day of the year), so all numeric operators could be used. The month abbreviations are implemented as operators that add the first day of the month to the following *int* value; CSE does not disallow their use in other numeric contexts.

For *strings*, *object names*, and *choices*, the CSE input language currently has no operators except the `?:` conditional operator and the concat() function. Note, though, that the choose, choose1, select, and hourval functions described below work with strings, object names, and choice values as well as numbers.

## System Variables

*System Variables* are built-in operands with useful values. To avoid confusion with other words, they begin with a `$`. Descriptions of the CSE system variables follow. Capitalization shown need not be matched. Most system variables change during a simulation run, resulting in the *variations* shown; they cannot be used where the context will not accept variation at least this fast. (The [Input Data Section][input-data] gives the *variability*, or maximum acceptable variation, for each object member.)

{{
    csv_table("$dayOfYear, Day of year of simulation&comma; 1 - 365; 1 corresponds to Jan-1. (Note that this is not the day of the simulation unless begDay is Jan-1.)<br>**Variation:** daily.
$month, Month of year&comma; 1 - 12.<br>**Variation:** monthly.
$dayOfMonth, Day of month&comma; 1 - 31.<br>**Variation:** daily.
$hour, Hour of day&comma; 1 - 24&comma; in local time; 1 corresponds to midnight - 1 AM.<br>**Variation:** hourly.
$hourST, Hour of day&comma; 1 - 24&comma; in standard time; 1 corresponds to midnight - 1 AM.<br>**Variation:** hourly.
$subhour, Subhour of hour&comma; 1 - N (number of subhours).<br>**Variation:** subhourly.
$dayOfWeek, Day of week&comma; 1 - 7; 1 corresponds to Sunday&comma; 2 to Monday&comma; etc. Note that $dayOfWeek is 4 (Wed) during autosizing.<br>**Variation:** daily.
$DOWH, Day of week 1-7 except 8 on every observed holiday. Note that $DOWH is 4 (Wed) during autosizing.<br>**Variation:** daily.
$isHoliday, 1 on days that a holiday is observed (regardless of the true date of the holiday); 0 on other days.<br>**Variation:** daily.
$isHoliTrue, 1 on days that are the true date of a holiday&comma; otherwise 0.<br>**Variation:** daily.
$isWeHol, 1 on weekend days or days that are observed as holidays.<br>**Variation:** daily.
$isWeekend, 1 on Saturday and Sunday&comma; 0 on any day from Monday to Friday.<br>**Variation:** daily.
$isWeekday, 1 on Monday through Friday&comma; 0 on Saturday and Sunday.<br>**Variation:** daily.
$isBegWeek, 1 for any day immediately following a weekend day or observed holiday that is neither a weekend day or an observed holiday.<br>**Variation:** daily.
$isWorkDay, 1 on non-holiday Monday through Friday&comma; 0 on holidays&comma; Saturday and Sunday.<br>**Variation:** daily.
$isNonWorkDay, 1 on Saturday&comma; Sunday and observed holidays&comma; 0 on non-holiday Monday through Friday.<br>**Variation:** daily.
$isBegWorkWeek, 1 on the first workday after a non-workday&comma; 0 all other days.<br>**Variation:** daily.
$isDT, 1 if Daylight Saving time is in effect&comma; 0 otherwise.<br>**Variation:** hourly.
$autoSizing, 1 during autosizing calculations&comma; 0 during main simulation.<br>**Variation:** for each phase.
$dsDay, Design day type&comma; 0 during main simulation&comma; 1 during heating autosize&comma; 2 during cool autosize.<br>**Variation:** daily.")
}}

**Weather variables**: the following allow access to the current hour's weather conditions in you CSE expressions. Units of measure are shown in parentheses. All have **Variation:** hourly.

{{
    csv_table("$radBeam, Solar beam irradiance (on a sun-tracking surface) this hour (Btu/ft2)
$radDiff, Solar diffuse irradiance (on horizontal surface) this hour (Btu/ft2)
$tDbO, Outdoor drybulb temperature this hour (degrees F)
$tWbO, Outdoor wetbulb temperature this hour (degrees F)
$wO, Outdoor humidity ratio this hour (lb H2O/lb dry air)
$windDirDeg, Wind direction (compass degrees)
$windSpeed, Wind speed (mph)")
}}

@nested-dl
## Built-in Functions

Built-in functions perform a number of useful scheduling and conditional operations in expressions. Built-in functions have the combined variation of their arguments; for *hourval*, the minimum result variation is hourly. For definitions of [*numeric*][numeric-any-type] and [*anyType*][numeric-any-type], see [Expression Types][expression-types].

---

### `brkt`

**Function**       
:   limits a value to be in a given range

**Syntax**       
:   *numeric* **brkt**( *numeric min, numeric val, numeric max* )

**Remark**    
:   If *val* is less than *min*, returns *min*; if *val* is greater than *max,* returns *max*; if *val* is in between, returns *val*.

**Example**
:   In an AIRHANDLER object, the following statement would specify a supply temperature equal to 130 minus the outdoor air temperature, but not less than 55 nor greater than 80:
    
    ```
    ahTsSp = brkt(55, 130 - $tDbO, 80);
    ```
    
    This would produce a 55-degree setpoint in hot weather, an 80-degree setpoint in cold weather, and a transition from 55 to 70 as the outdoor temperature moved from 75 to 50.

---


### `fix`

**Function**       
:   converts *float* to *int*

**Syntax**       
:   *int* **fix**( *float val* )

**Remark**    
:   *val* is converted to *int* by truncation.

**Examples**
:   
    ```
    fix(1.3)     // 1
    fix(1.99)    // 1
    fix(-4.4)    // -4
    ```

---

### `toFloat`

**Function**
:   converts *int* to *float*

**Syntax**
:   *float* **toFloat**( *int val* )

---

### `min`

**Function**
:   returns the lowest quantity from a list of values.

**Syntax**
:   *numeric* **min**( *numeric value1, numeric value2, ...*
                 *numeric valuen* )

**Remark**
:   there can be any number of arguments separated by commas;
                 if floats and ints are intermixed, the result is float.

---

### `max`

**Function**
:   returns the highest quantity from a list of values.

**Syntax**
:   *numeric* **max** ( *numeric value1, numeric value2,* ..., *numeric valuen* )

---

### `choose`

**Function**
:   returns the nth value from a list. If *arg0* is 0, *value0* is returned; for 1, *value1* is returned, etc.


**Syntax**
:   *anyType* **choose** ( *int arg0, anyType value0, anyType* *value1, ... anyType valuen* ) or *anyType* **choose** ( *int arg0, anyType value0, ... anyType* *valuen*, **default** *valueDef*)


**Remarks**
:   Any number of *value* arguments may be given. If **default** and another value is given, this value will be used if *arg0* is less than 0 or too large; otherwise, an error will occur.

---

### `choose1`

**Function**
:   same as choose except *arg0* is 1-based. Choose1 returns the second argument *value1* for *arg0* = 1, the third argument *value2* when *arg0* = 2, etc.

**Syntax**
:   *anyType* **choose1** ( *int arg0, anyType value1, anyType* *value2, ... anyType valuen* ) or *anyType* **choose1** ( *int arg0, anyType value1,* ... *anyType valuen,* **default** *valueDef*)

**Remarks**
:   **choose1** is a function that is well suited for use with daily system variables. For example, if a user wanted to denote different values for different days of the week, the following use of **choose1** could be implemented:

    `tuTC = choose1($dayOfWeek, MonTemp, TueTemp, ...)`

    !!! tip
        For hourly data, the **hourval** function would be a better choice, because it doesn't require the explicit declaration of the `$hour` system variable.

---

### `select`

**Function**
:   contains Boolean-value pairs; returns the value associated with the first Boolean that evaluates to true (non-0).

**Syntax**
:   *anyType* ( *Boolean arg1, anyType value1, Boolean arg2,* *anyType value2, ...* **default** *anyType*) (the **default** part is optional)

**Remark**
:   **select** is a function that simulates if-then logic during simulation (for people familiar with C, it works much like a series of imbedded conditionals: (a?b:(a?b:c)) ).

**Example**
:   `select` can be used to simulate a **dynamic** (run-time) **if-else** **statement**:

        gnPower = select( $isHoliday, HD_GAIN,  // if ($isHoliday)
        default WD_GAIN)  // else

    This technique can be combined with other functions to
    schedule items on a hourly and daily basis. For example, an
    internal gain that has different schedules for holidays,
    weekdays, and weekends could be defined as follows:

        // 24-hour lighting power schedules for weekend, weekday, holiday:
        #define WE_LIGHT  hourval( .024, .022, .021, .021, .021, .026, \
                     .038, .059, .056, .060, .059, .046, \
                     .045, .005, .005, .005, .057, .064, \
                     .064, .052, .050, .055, .044, .027 )

        #define WD_LIGHT  hourval( .024, .022, .021, .021, .021, .026, \
                     .038, .059, .056, .060, .059, .046, \
                     .045, .005, .005, .005, .057, .064, \
                     .064, .052, .050, .055, .044, .027 )

        #define HD_LIGHT  hourval( .024, .022, .021, .021, .021, .026, \
                     .038, .059, .056, .060, .059, .046, \
                     .045, .005, .500, .005, .057, .064, \
                     .064, .052, .050, .055, .044, .027 )

        // set power member of zone's GAIN object for lighting
        gnPower = BTU_Elec( ZAREA*0.1 ) *          // .1 kW/ft2 times...

            select( $isHoliday, HD_LIGHT,   // Holidays
                    $isWeekend, WE_LIGHT,   // Saturday & Sunday
                    default     WD_LIGHT ); // Week Days

    In the above, three subexpressions using **hourval** (next)
    are first defined as macros, for ease of reading and later
    change. Then, gnPower (the power member of a GAIN object)
    is set, using **select** to choose the appropriate one of
    the three **hourval** calls for the type of day. The
    expression for gnPower is a *live expression* with hourly
    variation, that is, CSE will evaluate it an set gnPower to
    the latest value each hour of the simulation. The variation
    comes from **hourval**, which varies hourly (also,
    `$isHoliday` and `$isWeekend` vary daily, but the faster
    variation determines the variation of the result).

---

### `hourval`

**Function**
:   from a list of 24 values, returns the value corresponding to the hour of day.

**Syntax**
:   *anyType hourval ( anyType value1, anyType value2,* …, *anyType value24* )

    *anyType hourval ( anyType value1, anyType value2*, …, **default** *anyType*)

**Remark**
:   **hourval** is evaluated at runtime and uses the hour of the day being simulated to choose the corresponding value from the 24 suppplied values.

    If less than 24 *value* arguments are given, **default** and another value (or expression) should be supplied to be used for hours not explicitly specified.

**Example**
:   see **select**, just above.

---

### `abs`

**Function**
:   converts numeric to its absolute value

**Syntax**
:   numeric **abs**( numeric val)

---

### `sqrt`

**Function**
:   Calculates and returns the positive square root of *val* ( *val* must be $\geq$ 0).

**Syntax**
:   *float* **sqrt** ( *float val*)

---

### `exp`

**Function**
:   Calculates and returns the exponential of *val* (=e*^val^*)

**Syntax**
:   *float* **exp**( *float val*)

---

### `logE`

**Function**
:   Calculates and returns the base e logarithm of *val* ( *val*
               must be $\geq$ 0).

**Syntax**
:   *float* **logE**( *float val*)

---

### `log10`

**Function**
:   Calculates and returns the base 10 logarithm of *val* ( *val* must be $\geq$ 0).

**Syntax**
:   *float* **log10**( *float val*)

---

### `sin`

**Function**
:   Calculates and returns the sine of *val* (val in radians)

**Syntax**
:   *float* **sin**( *float val*)

---

### `sind`

**Function**
:   Calculates and returns the sine of *val* (val in degrees)

**Syntax**
:   *float* **sind**( *float val*)

---

### `asin`

**Function**
:   Calculates and returns (in radians) the arcsine of *val*

**Syntax**
:   *float* **asin**( *float val*)

---

### `asind`

**Function**
:   Calculates and returns (in degrees) the arcsine of *val*

**Syntax**
:   *float* **asind**( *float val*)

---

### `cos`

**Function**
:   Calculates and returns the cosine of *val* (val in radians)

**Syntax**
:   *float* **cos**( *float val*)

---

### `cosd`

**Function**
:   Calculates and returns the cosine of *val* (val in degrees)

**Syntax**
:   *float* **cosd**( *float val*)

---

### `acos`

**Function**
:   Calculates and returns (in radians) the arccosine of *val*

**Syntax**
:   *float* **acos**( *float val*)

---

### `acosd`

**Function**
:   Calculates and returns (in degrees) the arccosine of *val*

**Syntax**
:   *float* **acosd**( *float val*)

---

### `tan`

**Function**
:   Calculates and returns the tangent of *val* (val in radians)

**Syntax**
:   *float* **tan**( *float val*)

---

### `tand`

**Function**
:   Calculates and returns the tangent of *val* (val in degrees)

**Syntax**
:   *float* **tand**( *float val*)

---

### `atan`

**Function**
:   Calculates and returns (in radians) the arctangent of *val*

**Syntax**
:   *float* **atan**( *float val*)

---

### `atand`

**Function**
:   Calculates and returns (in degrees) the arctangent of *val*

**Syntax**
:   *float* **atand**( *float val*)

---

### `atan2`

**Function**
:   Calculates and returns (in radians) the arctangent of y/x (handling x = 0)

**Syntax**
:   *float* **atan2**( *float y, float x*)

---

### `atan2d`

**Function**
:   Calculates and returns (in degrees) the arctangent of y/x (handling x = 0)

**Syntax**
:   *float* **atan2d**( *float y, float x*)

---

### `pow`

**Function**
:   Calculates and returns *val* raised to the *x*th power (=*val*^x^). *val* and *x* cannot both be 0. If *val* &lt; 0, *x* must be integral.

**Syntax**
:   *float* **pow**( *float val, numeric x*)

---

### `enthalpy`

**Function**
:   Returns enthalpy of moist air (Btu/lb) for dry bulb temperature (F) and humidity ratio (lb/lb)

**Syntax**
:   *float* **enthalpy**( *float tDb, float w*)

---

### `wFromDbWb`

**Function**
:   Returns humidity ratio (lb/lb) of moist air from dry bulb and wet bulb temperatures (F)

**Syntax**
:   *float* **wFromDbWb**( *float tDb, float tWb*)

---

### `wFromDbRh`

**Function**
:   Returns humidity ratio (lb/lb) of moist air from dry bulb temperature (F) and relative humidity (0 – 1)

**Syntax**
:   *float* **wFromDbRh**( *float tDb, float rh*)

---

### `rhFromDbW`

**Function**
:   Returns relative humidity (0 – 1) of moist air from dry bulb temperature (F) and humidity ratio (lb/lb).  

**Syntax**
:   *float* **rhFromDbW**( *float tDb, float w*)

**Remark**
:   The return value is constrained to 0 <= rh <= 1 (that is, physically impossible combinations of tDb and w are silently tolerated).

---

<!--
TODO: test psychrometric functions 7-22-2011
-->
### `concat`

**Function**
:    Returns *string* concatenation of arguments

**Syntax**
:   *string* **concat**( *string s1, string s2, ... string sn*)

**Example**
:   Assuming Jan 1 falls on Thurs and the simulation day is May 3:<br>
    **concat**( @Top.dateStr, " falls on a ", select( $isWeekend, "weekend", default "weekday"))
    returns "Sun 03-May falls on a weekend"

---

### `import`

**Function**
:    Returns *float* read from an import file.

**Syntax**
:   *float* **import**( *string importFile, string colName*)\
    *float* **import**( *string importFile, int colN*)

**Remark**
:   Columns can be referenced by name or 1-based index.
    
    See [IMPORTFILE][importfile] for details on use of import()

---

### `importStr`

**Function**
:   Returns *string* read from an import file.

**Syntax**
:   *string* **importStr**( *string importFile, string colName*)\
    *string* **importStr**( *string importFile, int colN*)

**Remark**
:   See [IMPORTFILE][importfile] for details on use of importStr()

---

### `contin`

**Function**
:   Returns continuous control value, e.g. for lighting control

**Syntax**
:   *float* **contin**( *float* mpf, *float* mlf, *float* sp, *float* val)

**Remark**
:   **contin** is evaluated at runtime and returns a value in the range 0 – 1 ???

<!-- TODO: complete documentation for contin()   7-26-2012 -->

---

### `stepped`

**Function**
:   Returns stepped reverse-acting control value, e.g. for lighting control

**Syntax**
:   *float* **stepped**( *int* nsteps, *float* sp, *float* val)

**Remark**
:   **stepped** is evaluated at runtime and returns a value in the range 0 – 1. If val &lt;= 0, 1 is returned; if val &gt;= sp, 0 is returned; otherwise, a stepped intermediate value is returned (see example)

**Example**
:   $$
    \textbf{stepped}(3, 12, \text{val}) = \begin{cases}
    1       & \text{if } \text{val} < 4 \\
    0.667   & \text{if } 4 \leq \text{val} < 8 \\
    0.333   & \text{if } 8 \leq \text{val} < 12 \\
    0       & \text{if } \text{val} \geq 12
    \end{cases}
    $$



---
