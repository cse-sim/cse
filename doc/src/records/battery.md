# BATTERY

BATTERY describes input data for a model of an energy-storage system which is not tied to any specific energy storage technology. The battery model integrates the energy added and removed (accounting for efficiency losses). Note: although we use the term battery, the underlying model is flexible enough to model any energy storage system.

The modeler can set limits and constraints on capacities and flows and the associated efficiencies for this model.

**btName**

Name of the battery system. Given after the word BATTERY.

|**Units**|**Legal Range**|**Default**|**Required**|**Variability**|
|---------|---------------|-----------|------------|---------------|
|         |*63 characters*|*none*     |No          |constant       |

**btMeter=*choice***

Name of a meter by which the BATTERY's power input/output (i.e., charge/discharge) is recorded. Charges to the BATTERY system would be seen as a positive powerflow while discharges from the BATTERY system would be seen as a negative value.

|**Units**|**Legal Range**|**Default**|**Required**|**Variability**|
|---------|---------------|-----------|------------|---------------|
|         |meter name     |*none*     |No          |constant       |

**btEndUse=*choice***

Meter end use to which the BATTERY's charged/discharged energy should be accumulated.

  ------- -------------------------------------------------------------
  Clg     Cooling
  Htg     Heating (includes heat pump compressor)
  HPHTG   Heat pump backup heat
  DHW     Domestic (service) hot water
  DHWBU   Domestic (service) hot water heating backup (HPWH resistance)
  FANC    Fans, AC and cooling ventilation
  FANH    Fans, heating
  FANV    Fans, IAQ venting
  FAN     Fans, other purposes
  AUX     HVAC auxiliaries such as pumps
  PROC    Process
  LIT     Lighting
  RCP     Receptacles
  EXT     Exterior lighting
  REFR    Refrigeration
  DISH    Dishwashing
  DRY     Clothes drying
  WASH    Clothes washing
  COOK    Cooking
  USER1   User-defined category 1
  USER2   User-defined category 2
  PV      Photovoltaic power generation
  BT      Battery power generation
  ------- -------------------------------------------------------------

|**Units**|   **Legal Range**  |**Default**|**Required**|**Variability**|
|---------|--------------------|-----------|------------|---------------|
|         |*Codes listed above*|     BT    |No          |constant       |

**btChgEff=*float***

The charging efficiency of storing electricity into the BATTERY system. A value of 1.0 means that no energy is lost and 100% of charge energy enters and is stored in the battery.

|**Units**|**Legal Range**  |**Default**|**Required**|**Variability**|
|---------|-----------------|-----------|------------|---------------|
|         |0 $\le$ x $\le$ 1|0.9        |No          |subhourly      |

**btDschgEff=*float***

The discharge efficiency for when the BATTERY system is discharging power. A value of 1.0 means that no energy is lost and 100% of discharge energy leaves the system.

|**Units**| **Legal Range**   |**Default**|**Required**|**Variability**|
|---------|-------------------|-----------|------------|---------------|
|         | 0 $\le$ x $\le$ 1 |   0.9     |     No     |subhourly      |

**btMaxCap=*float***

This is the maximum amount of energy that can be stored in the BATTERY system in kilowatt-hours. Once the BATTERY has reached its maximum capacity, no additional energy will be stored.

|**Units**|**Legal Range**|**Default**|**Required**|**Variability**|
|---------|---------------|-----------|------------|---------------|
| KWhr    | x $\ge$ 0     | 1e11      |No          |subhourly      |

**btMaxChgPwr=*float***

The maximum rate at which the BATTERY can be charged in kilowatts (i.e., energy flowing *into* the BATTERY).

|**Units**|**Legal Range**|**Default**|**Required**|**Variability**|
|---------|---------------|-----------|------------|---------------|
| kW      | x $\ge$ 0     | 50e3      |No          |subhourly      |

**btMaxDschgPwr=*float***

The maximum rate at which the BATTERY can be discharged in kilowatts (i.e., energy flowing *out of* the BATTERY).

|**Units**|**Legal Range**|**Default**|**Required**|**Variability**|
|---------|---------------|-----------|------------|---------------|
| kW      | x $\ge$ 0     | 25e3      |No          |subhourly      |

**btRqstChg=*float***

The power request to charge (or discharge if negative) the battery in kilowatts. The value of this parameter gets limited by the physical limitations of the battery and can be set by an expression to allow complex energy management/dispatch strategies.

|**Units**|**Legal Range** |**Default**|**Required**|**Variability**|
|---------|----------------|-----------|------------|---------------|
| kW      |                | 0         |No          |subhourly      |

**endBATTERY**

Optionally indicates the end of the BATTERY definition. Alternatively, the end of the definition can be indicated by END or by beginning another object.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant

