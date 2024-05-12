# PERFORMANCEMAP

**pmName**

Name of performance map.  Necessary to allow reference from e.g. RSYS.

**endPERFORMANCEMAP**

Optionally indicates the end of PERFORMANCEMAP definition.  It is good practice to include *endPerformanceMap* after associated PMGRIDAXIS and PMLOOKDATA.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "constant") %>



# PMGRIDAXIS

**pmGXName**

**pmGXValues=*float array***

**pmGXRefValue=*float***

**endPMGRIDAXIS**

Optionally indicates the end of PMGRIDAXIS definition.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "constant") %>



# PMLOOKUPDATA

**pmLUName**

**pmLUValues=*float array***

**endPMLOOKUPDATA**

Optionally indicates the end of PERFORMANCEMAP definition.  It is good practice to include *endPerformanceMap* after associated PMGRIDAXIS and PMLOOKDATA.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "constant") %>