# AFMETER

An AFMETER object is a user-defined "device" that records zone air flows as simulated by CSE. The user defines AFMETERs and assigns them to zones (see ZONE znAFMtr).

Air flow is recorded in standard air cfm (density 0.075 lb/ft3) at subhour, hour, day, month, and year durations.  Flows are categorized according to IZXFER izAFCat.

Note that *only* AirNet flows are recorded.  


**afMtrName**

Name of meter: required for assigning energy uses to the meter elsewhere.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        Yes            constant

**endAFMeter**

Indicates the end of the meter definition. Alternatively, the end of the meter definition can be indicated by the declaration of another object or by END.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant

**Related Probes:**

- @[afmeter](#p_afmeter)
