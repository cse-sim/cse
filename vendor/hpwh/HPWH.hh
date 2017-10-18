#ifndef HPWH_hh
#define HPWH_hh

#include <string>
#include <sstream>
#include <cmath>
#include <iostream>
#include <iomanip>

#include <cstdio>
#include <cstdlib>   //for exit
#include <vector>

//#define HPWH_ABRIDGED
/**<  If HPWH_ABRIDGED is defined, then some function definitions will be
 *  excluded from compiling.  This is done in order to reduce the size of the
 * final compiled code.  */

class HPWH {
 public:
  static const int version_major = 1;
  static const int version_minor = 3;
  //oh man, why can't i initialize this here?
  static const std::string version_maint;


  static const float DENSITYWATER_kgperL;
  static const float CPWATER_kJperkgC;
  static const int CONDENSITY_SIZE = 12;  /**< this must be an integer, and only the value 12 
  //change at your own risk */
  static const int MAXOUTSTRING = 200;  /**< this is the maximum length for a debuging output string */
  static const float HEATDIST_MINVALUE; /**< any amount of heat distribution less than this is reduced to 0
  //this saves on computations */
  static const float UNINITIALIZED_LOCATIONTEMP;  /**< this is used to tell the
  simulation when the location temperature has not been initialized */


  HPWH();  /**< default constructor */
  HPWH(const HPWH &hpwh);  /**< copy constructor  */
  HPWH & operator=(const HPWH &hpwh);  /**< assignment operator  */
	~HPWH(); /**< destructor just a couple dynamic arrays to destroy - could be replaced by vectors eventually?   */

  ///specifies the various modes for the Demand Response (DR) abilities
  ///values may vary - names should be used
  enum DRMODES{
    DR_BLOCK = 0,   /**<this mode prohibits the elements from engaging and turns
                    off any currently running  */
    DR_ALLOW = 1,   /**< this mode allows the water heater to run normally  */
    DR_ENGAGE = 2  /**< this mode forces an element to turn on  */
    };

  ///specifies the allowable preset HPWH models
  ///values may vary - names should be used
  enum MODELS{
    // these models are used for testing purposes
    MODELS_restankNoUA = 1,       /**< a simple resistance tank, but with no tank losses  */
    MODELS_restankHugeUA = 2,     /**< a simple resistance tank, but with very large tank losses  */
    MODELS_restankRealistic = 3,  /**< a more-or-less realistic resistance tank  */
    MODELS_basicIntegrated = 4,   /**< a standard integrated HPWH  */
    MODELS_externalTest = 5,      /**< a single compressor tank, using "external" topology  */

    // these models are based on real tanks and measured lab data 
    // AO Smith models
    MODELS_AOSmithPHPT60 = 102,         /**< this is the Ecotope model for the 60 gallon Voltex HPWH  */
    MODELS_AOSmithPHPT80 = 103,         /**<  Voltex 80 gallon tank  */
    MODELS_AOSmithHPTU50 = 104,    /**< 50 gallon AOSmith HPTU */ 
    MODELS_AOSmithHPTU66 = 105,    /**< 66 gallon AOSmith HPTU */ 
    MODELS_AOSmithHPTU80 = 106,    /**< 80 gallon AOSmith HPTU */
    MODELS_AOSmithHPTU80_DR = 107,    /**< 80 gallon AOSmith HPTU */  

    // GE Models
    MODELS_GE2012 = 110,      /**<  The 2012 era GeoSpring  */
    MODELS_GE2014STDMode = 111,    /**< 2014 GE model run in standard mode */ 
    MODELS_GE2014STDMode_80 = 113,    /**< 2014 GE model run in standard mode, 80 gallon unit */ 
    MODELS_GE2014 = 112,           /**< 2014 GE model run in the efficiency mode */
    MODELS_GE2014_80 = 114,           /**< 2014 GE model run in the efficiency mode, 80 gallon unit */
    MODELS_GE2014_80DR = 115,           /**< 2014 GE model run in the efficiency mode, 80 gallon unit */

    // Sanden CO2 transcritical heat pump water heaters
    MODELS_Sanden40 = 120,        /**<  Sanden 40 gallon CO2 external heat pump  */
    MODELS_Sanden80 = 121,        /**<  Sanden 80 gallon CO2 external heat pump  */

    // The new-ish Rheem
    MODELS_RheemHB50 = 140,        /**< Rheem 2014 (?) Model */

    // The new-ish Stiebel
    MODELS_Stiebel220E = 150,      /**< Stiebel Eltron (2014 model?) */

    // Generic water heaters, corresponding to the tiers 1, 2, and 3
    MODELS_Generic1 = 160,         /**< Generic Tier 1 */
    MODELS_Generic2 = 161,         /**< Generic Tier 2 */
    MODELS_Generic3 = 162,          /**< Generic Tier 3 */


    MODELS_UEF2generic = 170,   /**< UEF 2.0, modified GE2014STDMode case */
    MODELS_genericCustomUEF = 171,   /**< used for creating "generic" model with custom uef*/

    // Non-preset models
    MODELS_CustomFile = 200,      /**< HPWH parameters were input via file */
    MODELS_CustomResTank = 201      /**< HPWH parameters were input via HPWHinit_resTank */
    };

  ///specifies the modes for writing output
  ///the specified values are used for >= comparisons, so the numerical order is relevant
  enum VERBOSITY {
    VRB_silent = 0,     /**< print no outputs  */
    VRB_reluctant = 10,  /**< print only outputs for fatal errors  */
    VRB_minuteOut = 15,    /**< print minutely output  */
    VRB_typical = 20,    /**< print some basic debugging info  */
    VRB_emetic = 30      /**< print all the things  */
    };
    

  enum UNITS{
    UNITS_C,          /**< celsius  */
    UNITS_F,          /**< fahrenheit  */
    UNITS_KWH,        /**< kilowatt hours  */
    UNITS_BTU,        /**< british thermal units  */
    UNITS_KJ,         /**< kilojoules  */
    UNITS_GAL,        /**< gallons  */
    UNITS_L,          /**< liters  */
    UNITS_kJperHrC,   /**< UA, metric units  */
    UNITS_BTUperHrF   /**< UA, imperial units  */
    };

  /** specifies the type of heat source  */
  enum HEATSOURCE_TYPE {
    TYPE_none,        /**< a default to check to make sure it's been set  */
    TYPE_resistance,  /**< a resistance element  */
    TYPE_compressor   /**< a vapor cycle compressor  */
    };

    
  ///this is the value that the public functions will return in case of a simulation
  ///destroying error
  static const int HPWH_ABORT = -274000;

  static std::string getVersion();
  /**< This function returns a string with the current version number */
    
	int HPWHinit_presets(MODELS presetNum);
	/**< This function will load in a set of parameters that are hardcoded in this function - 
	 * which particular set of parameters is selected by presetNum.
	 * This is similar to the way the HPWHsim currently operates, as used in SEEM,
	 * but not quite as versatile.
	 * My impression is that this could be a useful input paradigm for CSE
	 * 
	 * The return value is 0 for successful initialization, HPWH_ABORT otherwise
	 */

	int HPWHinit_file(std::string configFile);
	/**< This function will load in a set of parameters from a file
	 * The file name is the input - there should be at most one set of parameters per file
	 * This is useful for testing new variations, and for the sort of variability
	 * that we typically do when creating SEEM runs
	 * Appropriate use of this function can be found in the documentation
   *
   * 
	 * The return value is 0 for successful initialization, HPWH_ABORT otherwise
	 */

  int HPWHinit_resTank();  /**< Default resistance tank, EF 0.95, volume 47.5 */
  int HPWHinit_resTank(double tankVol_L, double energyFactor, double upperPower_W, double lowerPower_W);
  /**< This function will initialize a HPWH object to be a resistance tank.  Since
   * resistance tanks are so simple, they can be specified with only four variables:
   * tank volume, energy factor, and the power of the upper and lower elements.  Energy
   * factor is converted into UA internally, although an external setter for UA is
   * also provided in case the energy factor is unknown.
   *
   * Several assumptions regarding the tank configuration are assumed: the lower element
   * is at the bottom, the upper element is at the top third.  The logics are also set
   * to standard setting, with upper as VIP activating when the top third is too cold.
   */
   
  int HPWHinit_genericHPWH(double tankVol_L, double energyFactor, double resUse_C);
  /**< This function will initialize a HPWH object to be a non-specific HPWH model
   * with an energy factor as specified.  Since energy
   * factor is not strongly correlated with energy use, most settings
   * are taken from the GE2015_STDMode model.
   */

  int runOneStep(double inletT_C, double drawVolume_L, 
                  double ambientT_C, double externalT_C,
                  DRMODES DRstatus, double minutesPerStep);
	/**< This function will progress the simulation forward in time by one step
	 * all calculated outputs are stored in private variables and accessed through functions
	 * 
	 * The return value is 0 for successful simulation run, HPWH_ABORT otherwise
	 */
	 
	int runNSteps(int N,  double *inletT_C, double *drawVolume_L, 
                  double *tankAmbientT_C, double *heatSourceAmbientT_C,
                  DRMODES *DRstatus, double minutesPerStep);
	/**< This function will progress the simulation forward in time by N (equal) steps
	 * The calculated values will be summed or averaged, as appropriate, and 
	 * then stored in the usual variables to be accessed through functions
	 * 
	 * The return value is 0 for successful simulation run, HPWH_ABORT otherwise
	 */


  void setVerbosity(VERBOSITY hpwhVrb);
  /**< sets the verbosity to the specified level  */
  void setMessageCallback( void (*callbackFunc)(const std::string message, void* pContext), void* pContext);
  /**< sets the function to be used for message passing  */
  void printHeatSourceInfo();
  /**< this prints out the heat source info, nicely formatted
      specifically input/output energy/power, and runtime
      will print to cout if messageCallback pointer is unspecified
      does not use verbosity, as it is public and expected to be called only when needed  */
  void printTankTemps();
  /**< this prints out all the node temps, kind of nicely formatted
      does not use verbosity, as it is public and expected to be called only when needed  */
  int WriteCSVHeading(FILE* outFILE, const char* preamble = "") const;
  int WriteCSVRow(FILE* outFILE, const char* preamble = "") const;
  /**< a couple of function to write the outputs to a file
      they both will return 0 for success
      the preamble should be supplied with a trailing comma, as these functions do
      not add one.  Additionally, a newline is written with each call.  */



  bool isSetpointFixed();  /**< is the setpoint allowed to be changed */
  int setSetpoint(double newSetpoint /**<default units C*/);
  int setSetpoint(double newSetpoint, UNITS units);
  /**< a function to change the setpoint - useful for dynamically setting it
      The return value is 0 for successful setting, HPWH_ABORT for units failure  */
  double getSetpoint();
  /**< a function to check the setpoint - returns setpoint in celcius  */

  int resetTankToSetpoint();
  /**< this function resets the tank temperature profile to be completely at setpoint
      The return value is 0 for successful completion  */

  int setAirFlowFreedom(double fanFraction);
  /**< This is a simple setter for the AirFlowFreedom */

  int setDoTempDepression(bool doTempDepress);
  /**< This is a simple setter for the temperature depression option */

  int setTankSize(double HPWH_size_L);
  int setTankSize(double HPWH_size, UNITS units);
  /**< This is a simple setter for the tank volume in L or GAL */

  int setUA(double UA_kJperHrC);
  int setUA(double UA, UNITS units);
  /**< This is a setter for the UA, with or without units specified - default is metric */

  int getUA( double& UA, UNITS units=UNITS_kJperHrC) const;

  
  
	int getNumNodes() const;
	/**< returns the number of nodes  */
  double getTankNodeTemp(int nodeNum /**default units C*/) const;
  double getTankNodeTemp(int nodeNum, UNITS units) const;
	/**< returns the temperature of the water at the specified node - with specified units
      or HPWH_ABORT for incorrect node number or unit failure  */

	double getNthSimTcouple(int N /**default units C*/) const;
  double getNthSimTcouple(int N, UNITS units) const;
  /**< returns the temperature from a set of 6 virtual "thermocouples", which are constructed
      from the node temperature array.  Specify t-couple from 1-6, 1 at the bottom
      using specified units
      returns HPWH_ABORT for N < 0, > 6, or incorrect units  */

	int getNumHeatSources() const;
	/**< returns the number of heat sources  */
	double getNthHeatSourceEnergyInput(int N /**default units kWh*/) const;
	double getNthHeatSourceEnergyInput(int N, UNITS units) const;
	/**< returns the energy input to the Nth heat source, with the specified units
      energy used by the heat source is positive - should always be positive
      returns HPWH_ABORT for N out of bounds or incorrect units  */

  
  double getNthHeatSourceEnergyOutput(int N /**default units kWh*/) const;
	double getNthHeatSourceEnergyOutput(int N, UNITS units) const;
	/**< returns the energy output from the Nth heat source, with the specified units
      energy put into the water is positive - should always be positive
      returns HPWH_ABORT for N out of bounds or incorrect units  */
	double getNthHeatSourceRunTime(int N) const;
	/**< returns the run time for the Nth heat source, in minutes
      note: they may sum to more than 1 time step for concurrently running heat sources
      returns HPWH_ABORT for N out of bounds  */
  int isNthHeatSourceRunning(int N) const;
  /**< returns 1 if the Nth heat source is currently engaged, 0 if it is not, and
      returns HPWH_ABORT for N out of bounds  */
  HEATSOURCE_TYPE getNthHeatSourceType(int N) const;
  /**< returns the enum value for what type of heat source the Nth heat source is  */

  

	double getOutletTemp(/**default units C*/) const;
	double getOutletTemp(UNITS units) const;
	/**< returns the outlet temperature in the specified units
      returns 0 when no draw occurs, or HPWH_ABORT for incorrect unit specifier  */

	double getEnergyRemovedFromEnvironment(/**default units kWh*/) const;
	double getEnergyRemovedFromEnvironment(UNITS units) const;
	/**< get the total energy removed from the environment by all heat sources in specified units
      (not net energy - does not include standby)
      moving heat from the space to the water is the positive direction
      returns HPWH_ABORT for incorrect units  */
  double getStandbyLosses(/**default units kWh*/) const;
	double getStandbyLosses(UNITS units) const;
	/**< get the amount of heat lost through the tank in specified units
      moving heat from the water to the space is the positive direction
      negative should occur seldom
      returns HPWH_ABORT for incorrect units  */

  double getTankHeatContent_kJ() const;
  /**< get the heat content of the tank, relative to zero celsius
   * returns using kilojoules */

	/** An overloaded function that uses some member variables, instead of taking them as inputs  */
  int runOneStep(double drawVolume_L, double ambientT_C,
                  double externalT_C, DRMODES DRstatus) {
        return runOneStep(member_inletT_C, drawVolume_L, ambientT_C,
                        externalT_C, DRstatus, member_minutesPerStep);
  };
	/** Setters for the what are typically input variables  */
  void setInletT(double newInletT_C) {member_inletT_C = newInletT_C;};
  void setMinutesPerStep(double newMinutesPerStep) {member_minutesPerStep = newMinutesPerStep;};


 
 private:
  class HeatSource;

	void updateTankTemps(double draw, double inletT, double ambientT, double minutesPerStep);
	bool areAllHeatSourcesOff() const;
	/**< test if all the heat sources are off  */
	void turnAllHeatSourcesOff();
	/**< disengage each heat source  */
	
	double topThirdAvg_C() const;
	double bottomThirdAvg_C() const;
	double bottomHalfAvg_C() const;
	double bottomTwelthAvg_C() const;
	double bottomSixthAvg_C() const;
	double secondSixthAvg_C() const;
	double thirdSixthAvg_C() const;
	double fourthSixthAvg_C() const;
	double fifthSixthAvg_C() const;
	double topSixthAvg_C() const;
  
	/**< functions to calculate what the temperature in a portion of the tank is  */

  void calcDerivedValues();
	/**< a helper function for the inits, calculating condentropy and the lowest node  */
  int checkInputs();
	/**< a helper function to run a few checks on the HPWH input parameters  */


  void sayMessage(const std::string message) const;
	/**< if the messagePriority is >= the hpwh verbosity,
    either pass your message out to the callback function or print it to cout
    otherwise do nothing  */
  void msg( const char* fmt, ...) const;
  void msgV( const char* fmt, va_list ap=NULL) const;

  bool simHasFailed;
	/**< did an internal error cause the simulation to fail?  */
 
	bool isHeating;
	/**< is the hpwh currently heating or not?  */

	bool setpointFixed;
	/**< does the HPWH allow the setpoint to vary  */

  VERBOSITY hpwhVerbosity;
	/**< an enum to let the sim know how much output to say  */

  void (*messageCallback)(const std::string message, void* contextPtr);
	/**< function pointer to indicate an external message processing function  */
  void* messageCallbackContextPtr;
	/**< caller context pointer for external message processing  */
 

  MODELS hpwhModel;
  /**< The hpwh should know which preset initialized it, or if it was from a file */
  
	int numHeatSources;
	/**< how many heat sources this HPWH has  */
	HeatSource *setOfSources;
	/**< an array containing the HeatSources, in order of priority  */
	
	int numNodes;
	/**< the number of nodes in the tank - must be >= 12, in multiples of 12  */
	double tankVolume_L;
	/**< the volume in liters of the tank  */
	double tankUA_kJperHrC;
	/**< the UA of the tank, in metric units  */
	
	double setpoint_C;
	/**< the setpoint of the tank  */
	double *tankTemps_C;
	/**< an array holding the temperature of each node - 0 is the bottom node, numNodes is the top  */


  // Some outputs
	double outletTemp_C;
	/**< the temperature of the outlet water - taken from top of tank, 0 if no flow  */
	double energyRemovedFromEnvironment_kWh;
	/**< the total energy removed from the environment, to heat the water  */
	double standbyLosses_kWh;
	/**< the amount of heat lost to standby  */
	
	
  // special variables for adding abilities
	bool tankMixesOnDraw;
	/**<  whether or not the bottom third of the tank should mix during draws  */
	bool doTempDepression;
	/**<  whether the HPWH should use the alternate ambient temperature that  
        gets depressed when a compressor is running
        NOTE: this only works for 1 minute steps  */
  double locationTemperature;
	/**<  this is the special location temperature that stands in for the the 
        ambient temperature if you are doing temp. depression  */

  /** a couple variables to hold values which are typically inputs  */
  double member_inletT_C;
  double member_minutesPerStep;

};  //end of HPWH class




class HPWH::HeatSource {
 public:
  friend class HPWH;

	HeatSource(){}  /**< default constructor, does not create a useful HeatSource */
	HeatSource(HPWH *parentHPWH);
  /**< constructor assigns a pointer to the hpwh that owns this heat source  */
  HeatSource(const HeatSource &hSource);  ///copy constructor
  HeatSource& operator=(const HeatSource &hSource); ///assignment operator
  /**< the copy constructor and assignment operator basically just checks if there
      are backup/companion pointers - these can't be copied */

  void setupAsResistiveElement(int node, double Watts);
  /**< configure the heat source to be a resisive element, positioned at the
      specified node, with the specified power in watts */
  
	bool isEngaged() const;
  /**< return whether or not the heat source is engaged */
	void engageHeatSource(double heatSourceAmbientT_C);
  /**< turn heat source on, i.e. set isEngaged to TRUE */
	void disengageHeatSource();
  /**< turn heat source off, i.e. set isEngaged to FALSE */
	
	bool shouldHeat(double heatSourceAmbientT_C) const;
  /**< queries the heat source as to whether or not it should turn on */
	bool shutsOff(double heatSourceAmbientT_C) const;
  /**< queries the heat source whether should shut off (typically lowT shutoff) */

	void addHeat_temp(double externalT_C, double minutesPerStep);
	void addHeat(double externalT_C, double minutesToRun);
  /**< adds heat to the hpwh - this is the function that interprets the 
      various configurations (internal/external, resistance/heat pump) to add heat */

	void setCondensity(double cnd1, double cnd2, double cnd3, double cnd4, 
                     double cnd5, double cnd6, double cnd7, double cnd8, 
                     double cnd9, double cnd10, double cnd11, double cnd12);
  /**< a function to set the condensity values, it pretties up the init funcs. */
	
	
 private:
  //start with a few type definitions  
  enum COIL_CONFIG {
    CONFIG_SUBMERGED,
    CONFIG_WRAPPED,
    CONFIG_EXTERNAL
    };

  /**this is the set of logics available for shouldHeat */
  enum ONLOGIC{
    ONLOGIC_topThird,     /**< is the difference between setpoint and the topThird of
                            the tank is greater than decision point, turn on */
    ONLOGIC_topThird_absolute,     /**< if the average temp of the topThird of
                            the tank greater than decision point, turn on */
    ONLOGIC_bottomThird,  /**< is the difference between setpoint and the bottomThird of
                            the tank is greater than decision point, turn on */
    ONLOGIC_standby,       /**< if the difference between the top node and the setpoint is
                            greater than the decision point, turn on */
    ONLOGIC_bottomSixth,    /**< if the difference between the first (bottom) sixth of
                            the tank and the setpoint is greater than the decision point, turn on */
    ONLOGIC_secondSixth,    /**< if the difference between the second sixth of
                            the tank and the setpoint is greater than the decision point, turn on */
    ONLOGIC_thirdSixth,    /**< if the difference between the third sixth of
                            the tank and the setpoint is greater than the decision point, turn on */
    ONLOGIC_fourthSixth,    /**< if the difference between the fourth sixth of
                            the tank and the setpoint is greater than the decision point, turn on */
    ONLOGIC_fifthSixth,    /**< if the difference between the fifth sixth of
                            the tank and the setpoint is greater than the decision point, turn on */
    ONLOGIC_topSixth    /**< if the difference between the sixth (top) sixth of
                            the tank and the setpoint is greater than the decision point, turn on */
    };
  /** this is the set of logics available for shutsDown  */
  enum OFFLOGIC{
    OFFLOGIC_lowT,                /**< if temp is below decision point, shut off */
    OFFLOGIC_highT,                /**< if temp is abovr decision point, shut off */
    OFFLOGIC_lowTreheat,          /**< if temp is above decision point, just while running, shut off */
    OFFLOGIC_topNodeMaxTemp,   /**< if the top node temp is above decision point, shut off */
    OFFLOGIC_bottomNodeMaxTemp,   /**< if the bottom node temp is above decision point, shut off */
    OFFLOGIC_bottomTwelthMaxTemp, /**< if the bottom twelth of the tank is above decision point, shut off */
    OFFLOGIC_largeDraw,           /**< if the bottom third of the tank is below decision point, shut off */
    OFFLOGIC_largerDraw           /**< if the bottom half of the tank is below decision point, shut off */
    };

	/** the heating logic instructions come in pairs - a string to select
    which logic function to use, and a double to give the setpoint
    for that function */
  template <typename T>
	struct heatingLogicPair{
		T selector;
		double decisionPoint;
		/**< and a constructor to allow creating anonymous structs for easy assignment */
		heatingLogicPair(T x, double y) : selector(x), decisionPoint(y) {};
		};


	/** the creator of the heat source, necessary to access HPWH variables */
  HPWH *hpwh;
	
  // these are the heat source state/output variables
	bool isOn;
	/**< is the heat source running or not	 */
	
  // some outputs
	double runtime_min;
	/**< this is the percentage of the step that the heat source was running */
	double energyInput_kWh;
	/**< the energy used by the heat source */
	double energyOutput_kWh;
	/**< the energy put into the water by the heat source */

// these are the heat source property variables
	bool isVIP;
	/**< is this heat source a high priority heat source? (e.g. upper resisitor) */
	HeatSource* backupHeatSource;
	/**< a pointer to the heat source which serves as backup to this one
      should be NULL if no backup exists */
	HeatSource* companionHeatSource;
	/**< a pointer to the heat source which will run concurrently with this one
      it still will only turn on if shutsOff is false */
	
	double condensity[CONDENSITY_SIZE];
	/**< The condensity function is always composed of 12 nodes.  
      It represents the location within the tank where heat will be distributed,
      and it also is used to calculate the condenser temperature for inputPower/COP calcs.
      It is conceptually linked to the way condenser coils are wrapped around 
      (or within) the tank, however a resistance heat source can also be simulated
      by specifying the entire condensity in one node. */
  double shrinkage;
  /**< the shrinkage is a derived value, using parameters alpha, beta,
      and the condentropy, which is derived from the condensity
      alpha and beta are not intended to be settable
      see the hpwh_init functions for calculation of shrinkage */
  
	double T1_F, T2_F;
	/**< the two temperatures where the input and COP curves are defined */
	double inputPower_T1_constant_W, inputPower_T2_constant_W;
	double inputPower_T1_linear_WperF, inputPower_T2_linear_WperF;
	double inputPower_T1_quadratic_WperF2, inputPower_T2_quadratic_WperF2;
	/**< these are the coefficients for the quadratic function 
      defining the input power as a function of the condenser temperature */
	double COP_T1_constant, COP_T2_constant;
	double COP_T1_linear, COP_T2_linear;
	double COP_T1_quadratic, COP_T2_quadratic;
	/**< these are the coefficients for the quadratic function 
      defining the COP as a function of the condenser temperature */


	/** a vector to hold the set of logical choices for turning this element on */
	std::vector<heatingLogicPair<ONLOGIC> > turnOnLogicSet;
	/** a vector to hold the set of logical choices that can cause an element to turn off */
	std::vector<heatingLogicPair<OFFLOGIC> > shutOffLogicSet;

  void addTurnOnLogic(ONLOGIC selector, double decisionPoint);
  void addShutOffLogic(OFFLOGIC selector, double decisionPoint);
  /**< these are two small functions to remove some of the cruft in initiation functions */

	double hysteresis_dC;
	/**< a hysteresis term that prevents short cycling due to heat pump self-interaction
      when the heat source is engaged, it is subtracted from lowT cutoffs and
      added to lowTreheat cutoffs */

	bool depressesTemperature;
	/**<  heat pumps can depress the temperature of their space in certain instances - 
      whether or not this occurs is a bool in HPWH, but a heat source must 
      know if it is capable of contributing to this effect or not
      NOTE: this only works for 1 minute steps
      ALSO:  this is set according the the heat source type, not user-specified */

  double airflowFreedom;
  /**< airflowFreedom is the fraction of full flow.  This is used to de-rate compressor
      cop (not capacity) for cases where the air flow is restricted - typically ducting */


	COIL_CONFIG configuration; /**<  submerged, wrapped, external */
  HEATSOURCE_TYPE typeOfHeatSource;  /**< compressor, resistance */

	int lowestNode;
  /**< hold the number of the first non-zero condensity entry */




  // some private functions, mostly used for heating the water with the addHeat function

 	double addHeatAboveNode(double cap_kJ, int node, double minutesToRun);
  /**< adds heat to the set of nodes that are at the same temperature, above the
      specified node number */
  double addHeatExternal(double externalT_C, double minutesToRun, double &cap_BTUperHr, double &input_BTUperHr, double &cop);
  /**<  Add heat from a source outside of the tank. Assume the condensity is where
      the water is drawn from and hot water is put at the top of the tank. */
  
	/**  I wrote some methods to help with the add heat interface - MJL  */
  void getCapacity(double externalT_C, double condenserTemp_C, double &input_BTUperHr, double &cap_BTUperHr, double &cop);
  void calcHeatDist(std::vector<double> &heatDistribution);

	double getCondenserTemp();
  /**< returns the temperature of the condensor - it's a weighted average of the
      tank temperature, using the condensity as weights */
  
  /**<  A few helper functions */
  double expitFunc(double x, double offset);
  void normalize(std::vector<double> &distribution);

};  // end of HeatSource class


// a few extra functions for unit converesion
inline double dF_TO_dC(double temperature) { return (temperature*5.0/9.0); }
inline double F_TO_C(double temperature) { return ((temperature - 32.0)*5.0/9.0); }
inline double C_TO_F(double temperature) { return (((9.0/5.0)*temperature) + 32.0); }
inline double KWH_TO_BTU(double kwh) { return (3412.14 * kwh); }
inline double KWH_TO_KJ(double kwh) { return (kwh * 3600.0); }
inline double BTU_TO_KWH(double btu) { return (btu / 3412.14); }
inline double KJ_TO_KWH(double kj) { return (kj/3600.0); }
inline double BTU_TO_KJ(double btu) { return (btu * 1.055); }
inline double GAL_TO_L(double gallons) { return (gallons * 3.78541); }
inline double UAf_TO_UAc(double UAf) { return (UAf * 1.8 / 0.9478); }

#endif
