# ACCUMULATOR

An ACCUMULATOR object is a user-defined "device" that records arbitrary values computed during the CSE simulation.

TODO: expand description and example

ACCUMULATOR results must be reported using user-defined REPORTs or EXPORTs.  For example --

    ACCUMULATOR "Zone Heating Set Point"
      acmValue = @Zone[ 1].znTH // Subhourly output


    REPORT rpType=UDT rpFreq=Month rpDayBeg=Jan 1 rpDayEnd=Dec 31
        REPORTCOL colHead="Month"           colVal=$Month                                                          colWid= 3
        REPORTCOL colHead="Monthly Total"   colVal=@Accumulator["Zone Heating Set Point"].M.acmSum  colDec=0 colWid=10
        REPORTCOL colHead="Monthly Average" colVal=@Accumulator["Zone Heating Set Point"].M.acmMean colDec=0 colWid=10
    

    ACCUMULATOR "Window Transmitted Solar Gain" 
      acmValue = @xsurf[ 1].glzTrans // Subhourly output

    REPORT rpType=UDT rpFreq=Hour rpDayBeg=Jul 21 rpDayEnd=Jul 21
        REPORTCOL colHead="Month"                                                 colVal=$Month                                                          colWid= 3
        REPORTCOL colHead="Day"                                                   colVal=$Day                                                            colWid= 3
        REPORTCOL colHead="Hour"                                                  colVal=$Hour                                                           colWid= 3
        REPORTCOL colHead="Hourly Window Transmitted Solar Gain"                  colVal=@Accumulator["Window Transmitted Solar Gain"].H.acmSum  colDec=0 colWid=10
        REPORTCOL colHead="Hourly Average Subhour Window Transmitted Solar Gain"  colVal=@Accumulator["Window Transmitted Solar Gain"].H.acmMean colDec=0 colWid=10
        REPORTCOL colHead="Hourly Max Subhour Window Transmitted Solar Gain"      colVal=@Accumulator["Window Transmitted Solar Gain"].H.acmMax  colDec=0 colWid=10
        REPORTCOL colHead="Hourly Min Subhour Window Transmitted Solar Gain"      colVal=@Accumulator["Window Transmitted Solar Gain"].H.acmMin  colDec=0 colWid=10

**acmName**

Name of ACCUMULATOR: required for referencing in reports.

<%= member_table(
  units: "",
  legal_range: "*63 characters*",
  default: "*none*",
  required: "Yes",
  variability: "constant") %>

**acmValue=*float***

The value being accumulated.  Generally expression.

<%= member_table(
  units: "any",
  legal_range: "",
  default: "",
  required: "Yes",
  variability: "subhourly") %>


**endACCUMULATOR**

Indicates the end of the ACCUMULATOR definition. Alternatively, the end of the definition can be indicated by the declaration of another object or by END.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "constant") %>

**Related Probes:**

- @[accumulator](#p_accumulator)