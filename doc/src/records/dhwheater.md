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

Type of water heater.  This categorization is based on CEC and federal rating standards that change from time to time.

  --------------------- ---------------------------------------
  SMALLSTORAGE          A storage water heater having an energy
                        factor (EF) rating.  Generally, a gas-fired
                        storage water heater with
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

Heat source for water heater.  CSE implements uses efficiency-based models for
all whTypes (as documented in RACM, App. B).  In addition, the detailed Ecotope HPWH model is
available for electric (air source heat pump and resistance) SMALLSTORAGE water heaters.

  ------------- ---------------------------------------
  RESISTANCE    Electric resistance heating element\
                Deprecated for whType=SMALLSTORAGE (use RESISTANCEX)

  RESISTANCEX   Electric resistance heating element, detailed HPWH model

  ASHP          Air source heat pump, EF model\
                Deprecated for whType=SMALLSTORAGE (use ASHPX)

  ASHPX         Air source heat pump, detailed HPWH model

  FUEL          Fuel-fired burner
  ------------- ---------------------------------------

  **Units**   **Legal Range**        **Default**   **Required**   **Variability**
  ----------- ---------------------- ------------- -------------- -----------------
              *Codes listed above*   FUEL          No             constant

**whVol=*float***

Storage tank volume. Must be omitted or 0 for instantaneous whTypes.  Used by HPWH model (whHeatSrc=RESISTANCEX or whHeatSrc=ASHPX). Required when whHeatSrc=RESISTANCEX or whHeatSrc=ASHPX with whASHPType=GENERIC.  For all other configurations, whVol is documentation-only.  ?Update?

  -----------------------------------------------------------------------
  **Units**   **Legal Range**   **Default**     **Required**       **Variability**
  ----------- ----------------- --------------- ------------------ -----------------
  gal         $\ge$ 0.1          per whASHPType  For some HPWH       constant
              (caution: small    if HPWH         configurations,
              values may cause   else 50         see above
              runtime errors) 
  -----------------------------------------------------------------------

**whEF=*float***

Rated energy factor that specifies DHWHEATER efficiency under test conditions.  Used
by CSE to derive annual water heating efficiency and/or other characteristics as described
below.  Calculation methods are documented in RACM, Appendix B.

  ----------------------------------------------------------------------------
  Configuration                 whEF default    Use
  ---------------------------- ---------------  -------------------------
  whType=SMALLSTORAGE\               0.82        Derivation of whLDEF
  whHeatSrc=RESISTANCE or FUEL     

  whType=SMALLSTORAGE\               0.82        Derivation of whLDEF\
  whHeatSrc=ASHP                                 note inappropriate default\
                                                 (deprecated, use ASHPX)

  whType=SMALLSTORAGE\             (req'd)      Tank losses\
  whHeatSrc=ASHPX\                              Overall efficiency
  whASHPType=GENERIC              

  whType=SMALLSTORAGE\             (req'd)      Tank losses\
  whHeatSrc=RESISTANCEX                         Note: maximum whEF=0.98.

  whType=SMALLINSTANTANEOUS          0.82       Annual efficiency = whEF*0.92
  whHeatSrc=RESISTANCE or FUEL

  Any other                        (unused)

  ----------------------------------------------------------------------


  --------------------------------------------------------------------------------
  **Units**  **Legal**                    **Default** **Required** **Variability**
             **Range**
  ---------- ---------------------------- ----------- ------------ ---------------
              $>$ 0\                      *See above* *See above*  constant
              *Caution: maximum*          
              *not checked. Unrealistic*
              *values will cause runtime*
              *errors and/or invalid*
              *results*       

  --------------------------------------------------------------------------------

**whLDEF=*float***

Load-dependent energy factor for DHWHEATERs with whType=SMALLSTORAGE and whHeatSrc=FUEL
or whHeatSrc=RESISTANCE.  If not given, whLDEF is derived using a preliminary simulation
activated via DHWSYS wsCalcMode=PRERUN.  See RACM Appendix B.

  ------------------------------------------------------------------
  **Units** **Legal** **Default**    **Required**    **Variability**
            **Range**
  --------- --------- -------------- -------------- ----------------
            $>$ 0     Calculated via When whType =   constant
                      DHWSYS PreRun  SMALLSTORAGE   
                      mechanism      and PreRun not
                                     used           

  ------------------------------------------------------------------

**whZone=*znName***

Name of zone where water heater is located, used only in detailed HPWH models (whHeatSrc=ASHPX or whHeatSrc=RESISTANCEX), otherwise no effect. Zone conditions are used for tank heat loss calculations.  Heat exchanged with the DHWHEATER are included in the zone heat balance.  whZone also provides the default for whASHPSrcZn (see below). whZone and whTEx cannot both be specified.

------------------------------------------------------------------
  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              name of a ZONE    Not in a zone   No             constant
                                (heat losses
                                discarded)
  ------------------------------------------------------------------

**whTEx=*float***

Water heater surround temperature, used only in detailed HPWH models (whHeatSrc=ASHPX or whHeatSrc=RESISTANCEX), otherwise no effect.  whZone and whTEx cannot both be specified.

  **Units**   **Legal Range**   **Default**                                         **Required**   **Variability**
  ----------- ----------------- --------------------------------------------------- -------------- -----------------
  ^o^F        $\ge$ 0           whZone air temperature if specified, else 70 ^o^F   No             hourly

**whASHPType=*choice***

Air source heat pump type, valid only if whHeatSrc=ASHPX. These choices are supported by the detailed HPWH model.  Except for Generic, all heater characteristics are set by HPWH based on whASHPType.

<%= csv_table(<<END, :row_header=> true)
"Choice","Specified type"
"Generic","General generic (parameterized by wh_EF and wh_vol)"
"AOSmithPHPT60","60 gallon Voltex"
"AOSmithPHPT80","80 gallon Voltex"
"AOSmithHPTU50","50 gallon AOSmith HPTU"
"AOSmithHPTU66","66 gallon AOSmith HPTU"
"AOSmithHPTU80","80 gallon AOSmith HPTU"
"Sanden40","Sanden 40 gallon CO2 external heat pump"
"Sanden80","Sanden 80 gallon CO2 external heat pump"
"GE2012","2012 era GeoSpring"
"GE2014","2014 50 gal GE run in the efficiency mode"
"GE2014StdMode","2014 50 gal GE run in standard mode"
"GE2014StdMode80","2014 80 gal GE run in standard mode"
"RheemHB50","newish Rheem (2014 model?)"
"RheemHBDR2250","50 gallon, 2250 W resistance Rheem HB Duct Ready"
"RheemHBDR4550","50 gallon, 4500 W resistance Rheem HB Duct Ready"
"RheemHBDR2265","65 gallon, 2250 W resistance Rheem HB Duct Ready"
"RheemHBDR4565","65 gallon, 4500 W resistance Rheem HB Duct Ready"
"RheemHBDR2280","80 gallon, 2250 W resistance Rheem HB Duct Ready"
"RheemHBDR4580","80 gallon, 4500 W resistance Rheem HB Duct Ready"
"Stiebel220E","Stiebel Eltron (2014 model?)"
"GenericTier1","Generic Tier 1"
"GenericTier2","Generic Tier 2"
"GenericTier3","Generic Tier 3"
"UEF2Generic","Experimental UEF=2"
"BasicIntegrated","Typical integrated HPWH"
"ResTank","Resistance heater (no compressor).  Superceded by whHeatSrc=RESITANCEX"
"ResTankNoUA","Resistance heater (no compressor) with no tank losses.  Superseded by whHeatSrc=RESISTANCEX."
"AOSmithHPTU80DR","80 gallon AOSmith HPTU with fixed backup setpoint (experimental for demand response testing)"
"AOSmithSHPT50","50 gal AOSmith SHPT"
"AOSmithSHPT66","66 gal AOSmith SHPT"
"AOSmithSHPT80","80 gal AOSmith SHPT"
END
%>


  **Units**   **Legal Range**        **Default**   **Required**           **Variability**
  ----------- ---------------------- ------------- ---------------------- -----------------
              *Codes listed above*   --            When whHeatSrc=ASHPX   constant

**whASHPSrcZn=*znName***

Name of zone that serves as heat pump heat source used when whHeatSrc=ASHPX.  Used for tank heat loss calculations and default for whASHPSrcZn.  Heat exchanges are included in zone heat balance.  whASHPSrcZn and whASHPSrcT cannot both be specified.

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

Heat pump source air temperature used when whHeatSrc=ASHPX.  Heat removed from this source is added to the heated water but has no other effect.  whASHPSrcZn and whASHPSrcT cannot both be specified.

  **Units**   **Legal Range**   **Default**                                           **Required**   **Variability**
  ----------- ----------------- ----------------------------------------------------- -------------- -----------------
  ^o^F        $\ge$ 0           whASHPZn air temperature if specified, else 70 ^o^F   No             hourly

**whASHPResUse=*float***

  Specifies activation temperature difference for resistance heating, used only when whHeatSrc=ASHPX and whASHPType=GENERIC.  Refer to HPWH engineering documentation for model details.

  **Units**   **Legal Range**   **Default**   **Required**  **Variability**
  ----------- ----------------- ------------- ------------- -------------------------
      ^o^C           $\ge$ 0            7.22           N          constant

**whResHtPwr=*float***

  Specifies resistance upper element power, used only with whHeatSrc=RESISTANCEX.

  **Units**   **Legal Range**   **Default**   **Required**  **Variability**
  ----------- ----------------- ------------- ------------- -------------------------
    W             $\ge$ 0            4500           N          constant

**whResHtPwr2=*float***

  Specifies resistance lower element power, used only with whHeatSrc=RESISTANCEX.

  **Units**   **Legal Range**   **Default**   **Required**  **Variability**
  ----------- ----------------- ------------- ------------- -------------------------
      W             $\ge$ 0       whResHtPwr           N          constant

**whHPAF=*float***

Heat pump adjustment factor, applied to whLDEF when modeling whType=SMALLSTORAGE and whHeatSrc=ASHP. This value should be derived according to RACM App B Table B-6.  Deprecated: the detailed HPWH model (whHeatSrc=ASHPX) is recommended for air source heat pumps.

  **Units**   **Legal Range**   **Default**   **Required**                                    **Variability**
  ----------- ----------------- ------------- ----------------------------------------------- -----------------
              $>$ 0             1             When whType=SMALLSTORAGE and whHeatSrc=ASHP   constant

**whEff=*float***

Water heating efficiency, used in modeling whType=LARGESTORAGE and whType=LARGEINSTANTANEOUS.

  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ----------------------- ------------- -------------- -----------------
              0 &lt; whEff $\leq$ 1   .82           No             constant

**whSBL=*float***

Standby loss, used in modeling whType=LARGESTORAGE.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  Btuh        $\ge$ 0           0             No             constant

**whPilotPwr=*float***

Pilot light consumption, included in fuel energy use of DHWHEATERs with whHeatSrc=FUEL.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  Btuh        $\ge$ 0           0             No             hourly

**whParElec=*float***

Parasitic electricity power, included in electrical energy use of all DHWHEATERs.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  W           $\ge$ 0           0             No             hourly

**whElecMtr=*mtrName***

Name of METER object, if any, by which DHWHEATER electrical energy use is recorded (under end use DHW).

  **Units**   **Legal Range**     **Default**                 **Required**   **Variability**
  ----------- ------------------- --------------------------- -------------- -----------------
              *name of a METER*   *Parent DHWSYS wsElecMtr*   No             constant

**whxBUEndUse=*choice***

Specifies the whElecMtr end use, if any, to which extra backup energy is accumulated. In some water heater types, extra backup energy is modeled to maintain output temperature at wsTUse.  This energy is included in end use dhwBU.  whxBUEndUse allows separate reporting of extra backup energy for testing purposes.

**Units**   **Legal Range**     **Default**                 **Required**   **Variability**
----------- ------------------- --------------------------- -------------- -----------------
            *end use code*            (no accumulation)             No             constant

**whFuelMtr =*mtrName***

Name of METER object, if any, by which DHWHEATER fuel energy use is recorded (under end use DHW).

  **Units**   **Legal Range**     **Default**                 **Required**   **Variability**
  ----------- ------------------- --------------------------- -------------- -----------------
              *name of a METER*   *Parent DHWSYS wsFuelMtr*   No             constant

**endDHWHEATER**

Optionally indicates the end of the DHWHEATER definition.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             

**Related Probes:**

- @[DHWHeater](#p_dhwheater)
