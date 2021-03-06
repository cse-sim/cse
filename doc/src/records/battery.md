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

Meter end use to which the BATTERY's charged/discharged energy should be accumulated. Note that the battery end use is seen from the standpoint of a "load" on the electric grid. That is, when the battery is being charged, the end use will show up as positive. When the battery is being discharged (i.e., when it is offsetting other loads), it is seen as negative.

<%= insert_file('doc/src/enduses.md') %>

|**Units**|   **Legal Range**  |**Default**|**Required**|**Variability**|
|---------|--------------------|-----------|------------|---------------|
|         |*Codes listed above*|     BT    |No          |constant       |

**btChgEff=*float***

The charging efficiency of storing electricity into the BATTERY system. A value of 1.0 means that no energy is lost and 100% of charge energy enters and is stored in the battery.

|**Units**|**Legal Range**  |**Default**|**Required**|**Variability**|
|---------|-----------------|-----------|------------|---------------|
|         |0 < x $\le$ 1|0.975      |No          |hourly      |

**btDschgEff=*float***

The discharge efficiency for when the BATTERY system is discharging power. A value of 1.0 means that no energy is lost and 100% of discharge energy leaves the system.

|**Units**| **Legal Range**   |**Default**|**Required**|**Variability**|
|---------|-------------------|-----------|------------|---------------|
|         | 0 < x $\le$ 1 |0.975      |     No     |hourly      |

**btMaxCap=*float***

This is the maximum amount of energy that can be stored in the BATTERY system in kilowatt-hours. Once the BATTERY has reached its maximum capacity, no additional energy will be stored.

|**Units**|**Legal Range**|**Default**|**Required**|**Variability**|
|---------|---------------|-----------|------------|---------------|
| KWhr    | x $\ge$ 0     | 16        |No          |constant       |

**btInitSOE=*float***

The initial state of energy of the BATTERY system as a fraction of the total capacity. If `btInitSOE` is specified, the battery state-of-energy at the beginning of the actual simulation will be set to the amount specified, regardless of whether there was a warm-up period or not. If `btInitSOE` is NOT specififed, it will default to 1.0 (i.e., 100%) at the beginning of the warmup period (if any).

|**Units**|**Legal Range**    |**Default**|**Required**|**Variability**|
|---------|-------------------|-----------|------------|---------------|
|         | 0 $\le$ x $\le$ 0 |    1.0    |No          |constant       |

**btInitCycles=*int***

The number of cycles on the battery at the beginning of the run.

<% if show_comments %>

Note: a more robust life model will need not only cycle counts but cycles by depth of discharge to capture "shallow cycling" vs "deep cycling". A further enhancement is to capture "time at temperature". We may want to look into the battery literature and also more into this application to better understand what kind of life model may be a good fit in terms of information requirements vs fidelity.

<% end %>

|**Units**|**Legal Range**|**Default**|**Required**|**Variability**|
|---------|---------------|-----------|------------|---------------|
|number of cycles|x $\ge$ 0|0         |No          |runly          |

**btMaxChgPwr=*float***

The maximum rate at which the BATTERY can be charged in kilowatts (i.e., energy flowing *into* the BATTERY).

|**Units**|**Legal Range**|**Default**|**Required**|**Variability**|
|---------|---------------|-----------|------------|---------------|
| kW      | x $\ge$ 0     | 4         |No          |hourly      |

**btMaxDschgPwr=*float***

The maximum rate at which the BATTERY can be discharged in kilowatts (i.e., energy flowing *out of* the BATTERY).

|**Units**|**Legal Range**|**Default**|**Required**|**Variability**|
|---------|---------------|-----------|------------|---------------|
| kW      | x $\ge$ 0     | 4         |No          |hourly         |

**btControlAlg=*choice***

Selects charge/discharge control algorithm.  btChgReq (next) specifies the desired battery charge or discharge rate.  btControlAlg allows selection of alternative algorithms for deriving btChgReq.

-------------- -----------------------------------------------------------------------------------------
DEFAULT        btChgReq is used as input or defaulted (see below)

TDVPEAKSAVE    btChgReq input (if any) is ignored.  Instead, a California-specific algorithm is used
               that saves battery charge until peak TDV (Time Dependant Valuation) hours of the day,
               shifting energy generated on site (e.g. PV) to supply feed the grid during critical
               periods.  The algorithm requires availability of hourly TDV data, see Top.tdvFName.
-------------- -----------------------------------------------------------------------------------------

Note btControlAlg has hourly variability, allowing dynamic algorithm selection.  In California compliance applications, TDVPEAKSAVE is typically used only on days with high TDV peaks.

|**Units**|**Legal Range**     |**Default**       |**Required**|**Variability**|
|---------|--------------------|------------------|------------|---------------|
|      | DEFAULT or TDVPEAKSAVE| DEFAULT          |No          |hourly         |


**btChgReq=*float***

The power request to charge (or discharge if negative) the battery. If omitted, the default strategy is used (attempt to satisfy all loads and absorb all available excess power).  btChgReq and the default strategy requested power are literally *requests*: that is, more power will not be delivered than is available; more power will not be absorbed than capacity exits to store; and the battery's power limits will be respected.

btChgReq can be set by an expression to allow complex energy management/dispatch strategies.

|**Units**|**Legal Range** |**Default**       |**Required**|**Variability**|
|---------|----------------|------------------|------------|---------------|
| kW      |                | btMeter net load |No          |hourly         |

**btUseUsrChg=*choice***

Former yes/no choice that currently has no effect.  Deprecated, will be removed in a future version.

**endBATTERY**

Optionally indicates the end of the BATTERY definition. Alternatively, the end of the definition can be indicated by END or by beginning another object.


|**Units**|**Legal Range** |**Default**       |**Required**|**Variability**|
|---------|----------------|------------------|------------|---------------|
|         |                | *N/A*            | No         | constant      |

<!--
Probes? Control strategies?

SOE

-->

**Related Probes:**

- @[battery](#p_battery)
