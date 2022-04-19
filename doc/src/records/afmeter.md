# AFMETER

An AFMETER object is a user-defined "device" that records zone air flows as simulated by CSE. The user defines AFMETERs and assigns them to zones (see ZONE znAFMtr).

Air flow is recorded in standard air cfm (density 0.075 lb/ft3) at subhour, hour, day, month, and year intervals.  At intervals of an hour and longer, values are averaged.  Flows are categorized according to IZXFER izAFCat.

If any AFMETERs are defined, an additional AFMETER "sum_of_AFMETERs" is automatically created where the sums of all user-define AFMETERs are accumulated.

Note that *only* AirNet flows are recorded.

AFMETER results can be REPORTed using rpType=AFMTR (or EXPORTed using exType=AFMTR).


**afMtrName**

Name of meter: required for assigning air flows to the AFMETER.

<%= member_table(
  units: "",
  legal_range: "*63 characters*",
  default: "*none*",
  required: "Yes",
  variability: "constant")
%>

**endAFMeter**

Indicates the end of the meter definition. Alternatively, the end of the meter definition can be indicated by the declaration of another object or by END.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "constant")
%>

**Related Probes:**

- @[afmeter](#p_afmeter)
