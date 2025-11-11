# DHWSOLARCOLLECTOR

Solar Collector Array. May be multiple collectors on the same DHWSOLARSYS system. All inlets come from the DHWSOLARTANK.

Uses SRCC Ratings.

### scArea

Type: float

Collector area.

{{
  member_table({
    "units": "ft^2",
    "legal_range": "> 0", 
    "default": "0",
    "required": "Yes",
    "variability": "constant" 
  })
}}

### scMult

Number of identical collectors, default 1

{{
  member_table({
    "units": "",
    "legal_range": "> 0", 
    "default": "1",
    "required": "No",
    "variability": "constant" 
  })
}}

### scTilt

Type: float

Array tilt.

{{
  member_table({
    "units": "deg",
    "legal_range": "", 
    "default": "0",
    "required": "Yes",
    "variability": "constant" 
  })
}}

### scAzm

Type: float

Array azimuth.

{{
  member_table({
    "units": "deg",
    "legal_range": "", 
    "default": "0",
    "required": "Yes",
    "variability": "constant" 
  })
}}

### scFRUL

Type: float

Fit slope

{{
  member_table({
    "units": "Btuh/ft^2-°F",
    "legal_range": "", 
    "default": "-0.727",
    "required": "No",
    "variability": "constant" 
  })
}}

### scFRTA

Type: float

Fit y-intercept

{{
  member_table({
    "units": "*none*",
    "legal_range": "> 0", 
    "default": "0.758",
    "required": "No",
    "variability": "constant" 
  })
}}

### scTestMassFlow

Type: flaot

Mass flow rate for collector loop SRCC rating.

{{
  member_table({
    "units": "lb/h-ft^2^",
    "legal_range": "x > 0", 
    "default": "14.79",
    "required": "No",
    "variability": "constant" 
  })
}}

### scKta60

Type: float

Incident angle modifier at 60 degree, from SRCC rating.

{{
  member_table({
    "units": "",
    "legal_range": "x ≥ 0", 
    "default": "0.72",
    "required": "No",
    "variability": "constant" 
  })
}}

### scOprMassFlow

Type: float

Collector loop operating mass flow rate.

{{
  member_table({
    "units": "lb/h-ft^2^",
    "legal_range": "x > 0", 
    "default": "0.0",
    "required": "No",
    "variability": "constant" 
  })
}}

### scPipingLength

Type: float

Collector piping length.

{{
  member_table({
    "units": "ft",
    "legal_range": "x ≥ 0", 
    "default": "0.0",
    "required": "No",
    "variability": "Hourly and at the end of interval" 
  })
}}

### scPipingInsulK

Type: float

Collector piping insulation conductivity.

{{
  member_table({
    "units": "",
    "legal_range": "x > 0", 
    "default": "0.02167",
    "required": "No",
    "variability": "Hourly and at the end of interval" 
  })
}}

### scPipingInsulThk

Type: float

Collector piping insulation thickness.

{{
  member_table({
    "units": "",
    "legal_range": "x ≥ 0", 
    "default": "1.0",
    "required": "No",
    "variability": "Hourly and at the end of interval" 
  })
}}

### scPipingExH

Type: float

Collector piping heat transfer coefficient.

{{
  member_table({
    "units": "",
    "legal_range": "x > 0", 
    "default": "1.5",
    "required": "No",
    "variability": "Hourly and at the end of interval" 
  })
}}

### scPipingExT

Type: float

Collector piping surround temperature.

{{
  member_table({
    "units": "°F",
    "legal_range": "x ≥ 32", 
    "default": "70.0",
    "required": "No",
    "variability": "hourly" 
  })
}}

### scPumpPwr

Type: float

{{
  member_table({
    "units": "Btu/h",
    "legal_range": "x ≥ 0", 
    "default": "from *scPumpflow*",
    "required": "No",
    "variability": "constant" 
  })
}}

### scPumpLiqHeatF

Type: float

Fraction of scPumpPwr added to liquid stream, the remainder is discarded.

{{
  member_table({
    "units": "",
    "legal_range": "x ≥ 0", 
    "default": "1.0",
    "required": "No",
    "variability": "Every run" 
  })
}}

### scPumpOnDeltaT

Type: float

Temperature difference between the tank and collector outlet where pump turns on
  
{{
  member_table({
    "units": "°F",
    "legal_range": "", 
    "default": "10.0",
    "required": "No",
    "variability": "constant" 
  })
}}

### scPumpOffDeltaT

Type: float

Temperature difference between the tank and collector outlet where pump turns off

{{
  member_table({
    "units": "°F",
    "legal_range": "", 
    "default": "5.0",
    "required": "No",
    "variability": "constant" 
  })
}}

### endDHWSOLARCOLLECTOR

Optionally indicates the end of the DHWSOLARCOLLECTOR definition.

{{
  member_table({
    "units": "",
    "legal_range": "", 
    "default": "*none*",
    "required": "No",
    "variability": "" 
  })
}}

