# Variation Frequencies Revisited

At risk of beating the topic to death, we're going to review once more the frequencies with which a CSE value can change (*variations)*, with some comments on the corresponding *variabilities*.

{{
    csv_table("subhourly, changes in each \"subhour\" used in simulation. Subhours are commonly 15-minute intervals for models using znModel=CNE or 2-minute intervals for CSE znModels.
  hourly, changes every simulated hour. The simulated weather and many other aspects of the simulation change hourly; it is customary to schedule setpoint changes&comma; HVAC system operation&comma; etc. in whole hours.
  daily, changes at each simulated midnite.
  monthly, changes between simulated months.
  monthly-hourly&comma; or \"hourly on first day of each month\",   changes once an hour on the first day of each month; the 24 hourly values from the first day of the month are used for the rest of the month. This variation and variability is used for data dependent on the sun's position&comma; to save calculation time over computing it every hour of every day.
  run start time, value is derived from other inputs before simulation begins&comma; then does not change.<br> Members that cannot change during the simulation but which are not needed to derive other values before the simulation begins have \"run start time\" *variability*.
  input time, value is known before CSE starts to check data and derive \"run start time\" values. <br> Expressions with \"input time\" variation may be used in many members that cannot accept any variation during the run. Many members documented in the [Input Data][input-data] Section as having \"constant\" variability may actually accept expressions with \"input time\" variation; to find out&comma; try it: set the member to an expression containing a proposed probe and see if an error message results. <br> \"Input time\" differs from \"constant\" in that it includes object names (forward references are allowed&comma; and resolved just before other data checks) and probes that are forward references to constant values.
  constant, does not vary. But a \"constant\" member of a class denoted as R (with no I) in the probes report produced by CSE -p is actually not available until run start time.")
}}
  
Also there are end-of varieties of all of the above; these are values computed during simulation: end of each hour, end of run, etc. Such values may be reported (using a probe in a UDT report), but will produce an error message if probed in an expression for an input member value.

<!--
## Probes: Issues and Cautions

Watch yourself when using znRes[ ].prior when combined with AUTOSIZE – there is no prior!
-->
