# DHWHEATER

DHWHEATER constructs an object representing a domestic hot water heater (or several if identical).

**whName**

Optional name of water heater; give after the word “DHWHEATER” if desired.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**whMult=*integer***

Number of identical water heaters of this type. Any value $>1$ is equivalent to repeated entry of the same DHWHEATER.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              $>$ 0             1             No             constant

**whType=*choice***

Specifies type of water heater.

  --------------------- ---------------------------------------
  SMALLSTORAGE          A gas-fired storage water heater with
                        input of 75,000 Btuh or less, an
                        oil-fired storage water heater with
                        input of 105,000 Btuh or less, an
                        electric storage water heater with
                        input of 12 kW or less, or a heat pump
                        water heater rated at 24 amps or less.

  LARGESTORAGE          Any storage water heater that is not
                        SMALLSTORAGE.

  SMALLINSTANTANEOUS    A water heater that has an input rating
                        of at least 4,000 Btuh per gallon of
                        stored water. Small instantaneous water
                        heaters include: gas instantaneous
                        water heaters with an input of 200,000
                        Btu per hour or less, oil instantaneous
                        water heaters with an input of 210,000
                        Btu per hour or less, and electric
                        instantaneous water heaters with an
                        input of 12 kW or less.

  LARGEINSTANTANEOUS    An instantaneous water heater that does
                        not conform to the definition of
                        SMALLINSTANTANEOUS, an indirect
                        fuel-fired water heater, or a hot water
                        supply boiler.
  --------------------- ---------------------------------------

  **Units**   **Legal Range**        **Default**    **Required**   **Variability**
  ----------- ---------------------- -------------- -------------- -----------------
              *Codes listed above*   SMALLSTORAGE   No             constant

**whHeatSrc=*choice***

Specifies heat source for water heater.

  ------------- ---------------------------------------
  RESISTANCE    Electric resistance heating element.

  ASHP          Air source heat pump

  ASHPX         Air source heat pump (detailed HPWH
                model)

  FUEL          Fuel-fired burner
  ------------- ---------------------------------------

  **Units**   **Legal Range**        **Default**   **Required**   **Variability**
  ----------- ---------------------- ------------- -------------- -----------------
              *Codes listed above*   FUEL          No             constant

**whVol=*float***

Specifies tank volume. Documentation only. Must be omitted or 0 for instantaneous whTypes.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  gal         $\ge$ 0           0             No             constant

**whEF=*float***

Rated energy factor, used in modeling whType SMALLSTORAGE and SMALLINSTANTANEOUS.

  ------------------------------------------------------------------------
  **Units**  **Legal**       **Default**  **Required**       \*\*Variabili
             **Range**                                       ty*
                                                             *
  ---------- --------------- ------------ ------------------ -------------
             $>$ 0           .82          When whType =      constant
                                          SMALLSTORAGE and   
                                          whLDEF not         
                                          specified or       
                                          SMALLINSTANTANEOUS 
  ------------------------------------------------------------------------

**whLDEF=*float***

Load-dependent energy factor, used in modeling whType SMALLSTORAGE. Can

  ----------------------------------------------------------------------
  **Units**  **Legal**      **Default**    **Required**   **Variability*
             **Range**                                    *
  ---------- -------------- -------------- -------------- --------------
             $>$ 0          Calculated via When whType =  constant
                            DHWSYS PreRun  SMALLSTORAGE   
                            mechanism      and PreRun not 
                                           used           
  ----------------------------------------------------------------------

**whZone=*znName***

Name of zone where water heater is located. Zone conditions are for tank heat loss calculations and default for whASHPSrcZn (see below). No effect unless whHeatSrc = ASHPX. whZone and whTEx cannot both be specified.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              name of a ZONE    Not in zone   No             constant

**whTEx=*float***

Water heater surround temperature. No effect unless whHeatSrc=ASHPX. whZone and whTEx cannot both be specified.

  **Units**   **Legal Range**   **Default**                                         **Required**   **Variability**
  ----------- ----------------- --------------------------------------------------- -------------- -----------------
  ^o^F        $\ge$ 0           whZone air temperature if specified, else 70 ^o^F   No             hourly

**whASHPType=*choice***

Specifies type of air source heat pump, valid only if whHeatSrc=ASHPX. These choice are those supported by Ecotope HPWH model.

  ------------------ ---------------------------------------
  Voltex60           

  Voltex80           

  GEGeospring        

  BasicIntegrated    Typical integrated HPWH

  ResTank            Resistance-only water heater (no
                     compressor)
  ------------------ ---------------------------------------

  **Units**   **Legal Range**        **Default**   **Required**           **Variability**
  ----------- ---------------------- ------------- ---------------------- -----------------
              *Codes listed above*   --            When whHeatSrc=ASHPX   constant

**whASHPSrcZn=*znName***

Name of zone that serves as heat source when whHeatSrc = ASHPX. Used for tank heat loss calculations and default for whASHPSrcZn. Illegal unless whHeatSrc = ASHPX. whASHPSrcZn and whASHPSrcT cannot both be specified.

  -----------------------------------------------------------------------
  **Units**   **Legal**       **Default**     **Required**  **Variability
              **Range**                                     **
  ----------- --------------- --------------- ------------- -------------
              name of a ZONE  Same as whZone  No            constant
                              if whASHPSrcT                 
                              not specified.                
                              If no zone is                 
                              specified by                  
                              input or                      
                              default, heat                 
                              extracted by                  
                              ASHP has no                   
                              effect.                       
  -----------------------------------------------------------------------

**whASHPSrcT=*float***

ASHP source air temperature. Illegal unless whHeatSrc=ASHPX. whASHPSrcZn and whASHPSrcT cannot both be specified.

  **Units**   **Legal Range**   **Default**                                           **Required**   **Variability**
  ----------- ----------------- ----------------------------------------------------- -------------- -----------------
  ^o^F        $\ge$ 0           whASHPZn air temperature if specified, else 70 ^o^F   No             hourly

**whHPAF=*float***

Heat pump adjustment factor, used when modeling whType=SMALLSTORAGE and whHeatSrc=ASHP. This value should be derived according to App B Table B-6.

  **Units**   **Legal Range**   **Default**   **Required**                                    **Variability**
  ----------- ----------------- ------------- ----------------------------------------------- -----------------
              $>$ 0             1             When whType=SMALLSTORAGE and whHeatSrc = ASHP   constant

**whEff=*float***

Water heating efficiency, used in modeling LARGESTORAGE and LARGEINSTANTANEOUS.

  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ----------------------- ------------- -------------- -----------------
              0 &lt; whEff $\leq$ 1   .82           No             constant

**whSBL=*float***

Standby loss, used in modeling LARGESTORAGE

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  Btuh        $\ge$ 0           0             No             constant

**whPilotPwr=*float***

Pilot light consumption, included in energy use of DHWHEATERs with whHeatSrc=FUEL.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  Btuh        $\ge$ 0           0             No             hourly

**whParElec=*float***

Parasitic electricity power, included in energy use of all DHWHEATERs.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  W           $\ge$ 0           0             No             hourly

whElecMtr=*mtrName*

Name of METER object, if any, by which DHWHEATER electrical energy use is recorded (under end use DHW).

  **Units**   **Legal Range**     **Default**                 **Required**   **Variability**
  ----------- ------------------- --------------------------- -------------- -----------------
              *name of a METER*   *Parent DHWSYS wsElecMtr*   No             constant

**whFuelMtr =*mtrName***

Name of METER object, if any, by which DHWHEATER fuel energy use is recorded.

  **Units**   **Legal Range**     **Default**                 **Required**   **Variability**
  ----------- ------------------- --------------------------- -------------- -----------------
              *name of a METER*   *Parent DHWSYS wsFuelMtr*   No             constant

**endDHW**

HeaterOptionally indicates the end of the DHWHEATER definition.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             


