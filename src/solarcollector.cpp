#include "solarcollector.h"
#include "CNGLOB.H"
#include "slpak.h"

namespace
{
double deg_to_rad(3.1415 / 180.0); // Conversion for Degrees to Radians
}

SolarCollector::SolarCollector() : 
  iam1_(0.0), 
  iam2_(0.0) {
}

#if 0
void SolarCollector::sim_solar_collector(int const EquipTypeNum, 
                                         std::string const &CompName,
                                         int &CompIndex, 
                                         bool const InitLoopEquip,
                                         bool const FirstHVACIteration)
{
  using DataPlant::TypeOf_SolarCollectorFlatPlate;
  using DataPlant::TypeOf_SolarCollectorICS;
  using General::TrimSigDigits;

  // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
  int CollectorNum;

  // FLOW:
  if (GetInputFlag)
  {
    GetSolarCollectorInput();
    GetInputFlag = false;
  }

  if (CompIndex == 0)
  {
    CollectorNum = UtilityRoutines::FindItemInList(CompName, Collector);
    if (CollectorNum == 0)
    {
      ShowFatalError("SimSolarCollector: Specified solar collector not Valid =" + CompName);
    }
    CompIndex = CollectorNum;
  }
  else
  {
    CollectorNum = CompIndex;
    if (CollectorNum > NumOfCollectors || CollectorNum < 1)
    {
      ShowFatalError("SimSolarCollector: Invalid CompIndex passed=" + TrimSigDigits(CollectorNum) +
                     ", Number of Units=" + TrimSigDigits(NumOfCollectors) +
                     ", Entered Unit name=" + CompName);
    }
    if (CheckEquipName(CollectorNum))
    {
      if (CompName != Collector(CollectorNum).Name)
      {
        ShowFatalError("SimSolarCollector: Invalid CompIndex passed=" +
                       TrimSigDigits(CollectorNum) + ", Unit name=" + CompName +
                       ", stored Unit Name for that index=" + Collector(CollectorNum).Name);
      }
      CheckEquipName(CollectorNum) = false;
    }
  }

  InitSolarCollector(CollectorNum);

  {
    auto const SELECT_CASE_var(Collector(CollectorNum).TypeNum);
    // Select and CALL models based on collector type
    if (SELECT_CASE_var == TypeOf_SolarCollectorFlatPlate)
    {

      CalcSolarCollector(CollectorNum);
    }
    else if (SELECT_CASE_var == TypeOf_SolarCollectorICS)
    {

      CalcICSSolarCollector(CollectorNum);
    }
    else
    {
    }
  }

  UpdateSolarCollector(CollectorNum);

  ReportSolarCollector(CollectorNum);
}
#endif

#define TBD 1.0 // Vars yet to be decided

double transmitted_radiance_perez(double tilt,
                                  double azimuth)
{
	// Calculate horizontal incidence
	static float dchoriz[3] = { 0.f, 0.f, 1.f };	// dir cos of a horiz surface
	float verSun = 0.f;				// hour's vertical fract of beam (cosi to horiz); init 0 for when sun down.
	float azm, cosz;
	int sunup = 					// nz if sun above horizon this hour
		slsurfhr(dchoriz, Top.iHrST, &verSun, &azm, &cosz);
	// Calculate plane-of-array incidence
	float cosi; // cos(incidence)
	//   azm and tilt vary
	float dcos[3]; // direction cosines
	slsdc(azimuth, tilt, dcos);
	int sunupSrf = slsurfhr( dcos, Top.iHrST, &cosi);
	float fBeam = 1.f; // unshaded fraction
	if (sunupSrf < 0)
		return RCBAD;	// shading error

  double poaBeam, radIBeam, radIBeamEff, aoi;
	if (sunupSrf)	
	{
		poaBeam = Top.radBeamHrAv*cosi;
		radIBeam = poaBeam*fBeam;  // incident beam (including shading)
		radIBeamEff = max(poaBeam*(1.f - pv_sif * (1.f - fBeam)),0.f);
		aoi = acos(cosi);
	}
	else
	{	poaBeam = radIBeam = radIBeamEff = 0.f;  // incident beam
		aoi = kPiOver2;
	}

	// Modified Perez sky model
	float zRad = acos(cosz);
	float zDeg = DEG(zRad);
	float a = max(0.f, cosi);
	float b = max(cos(RAD(85.f)), cosz);
	const float kappa = 5.534e-6f;
	float e = ((Top.radDiffHrAv + Top.radBeamHrAv) / (Top.radDiffHrAv + 0.0001f) + kappa*pow3(zDeg)) / (1 + kappa*pow3(zDeg));
	float am0 = 1 / (cosz + 0.15f*pow(93.9f - zDeg, -1.253f));  // Kasten 1966 air mass model
	float delt = Top.radDiffHrAv*am0 / IrSItoIP(1367.f);

	int bin = e < 1.065f ? 0 :
		e >= 1.065f && e < 1.23f ? 1 :
		e >= 1.23f && e < 1.5f ? 2 :
		e >= 1.5f && e < 1.95f ? 3 :
		e >= 1.95f && e < 2.8f ? 4 :
		e >= 2.8f && e < 4.5f ? 5 :
		e >= 4.5f && e < 6.2f ? 6 : 7;

	// Perez sky model factors
	static const float f11[8] = { -0.0083117f, 0.1299457f, 0.3296958f, 0.5682053f, 0.873028f, 1.1326077f, 1.0601591f, 0.677747f };
	static const float f12[8] = { 0.5877285f, 0.6825954f, 0.4868735f, 0.1874525f, -0.3920403f, -1.2367284f, -1.5999137f, -0.3272588f };
	static const float f13[8] = { -0.0620636f, -0.1513752f, -0.2210958f, -0.295129f, -0.3616149f, -0.4118494f, -0.3589221f, -0.2504286f };
	static const float f21[8] = { -0.0596012f, -0.0189325f, 0.055414f, 0.1088631f, 0.2255647f, 0.2877813f, 0.2642124f, 0.1561313f };
	static const float f22[8] = { 0.0721249f, 0.065965f, -0.0639588f, -0.1519229f, -0.4620442f, -0.8230357f, -1.127234f, -1.3765031f };
	static const float f23[8] = { -0.0220216f, -0.0288748f, -0.0260542f, -0.0139754f, 0.0012448f, 0.0558651f, 0.1310694f, 0.2506212f };

	float F1 = max(0.f, f11[bin] + delt*f12[bin] + zRad*f13[bin]);
	float F2 = f21[bin] + delt*f22[bin] + zRad*f23[bin];

	float poaDiffI, poaDiffC, poaDiffH, poaDiffG;  // Components of diffuse solar (isotropic, circumsolar, horizon, ground reflected)
	// Set up properties of array
	const double cosTilt = cos(tilt);
	const float vfGrndDf = .5 - .5*cosTilt;  // view factor to ground
	const float vfSkyDf = .5 + .5*cosTilt;  // view factor to sky

	if (zDeg >= 0.f && zDeg <= 87.5f) {
		poaDiffI = (1.f - F1)*vfSkyDf;
		poaDiffC = F1*a / b;
		poaDiffH = F2*sin(tilt);
	}
	else {
		poaDiffI = vfSkyDf;
		poaDiffC = 0.f;
		poaDiffH = 0.f;
	}

	poaDiffG = pv_grndRefl*vfGrndDf;

	float poaDiff = Top.radDiffHrAv*(poaDiffI + poaDiffC + poaDiffH + poaDiffG);  // sky diffuse and ground reflected diffuse
	float poaGrnd = Top.radBeamHrAv*pv_grndRefl*vfGrndDf*verSun;  // ground reflected beam
	auto poa = poaBeam + poaDiff + poaGrnd;
	auto radI = radIBeam + poaDiff + poaGrnd;
	auto radIEff = radIBeamEff + poaDiff + poaGrnd;

	//// Correct for off-normal transmittance
	//float theta1 = pv_aoi;
	//float theta2, tauAR, theta3, tauGl;

	//rc |= pv_CalcRefr(airRefrInd, pv_covRefrInd, theta1, theta2, tauAR);
	//rc |= pv_CalcRefr(pv_covRefrInd, glRefrInd, theta2, theta3, tauGl);

	//float tauC = tauAR*tauGl;

	//pv_radTrans = pv_radIBeamEff *tauC / pv_tauNorm + poaDiff + poaGrnd;

  return radIBeamEff;
}

void SolarCollector::simple_solar_collector(double tilt, 
                                            double azimuth,
                                            double slope,
                                            double intercept,
                                            double gross_area,
                                            double &gain,
                                            double &efficiency)
{
  double Q_inc = transmitted_radiance_perez(tilt, azimuth);
  double t_inlet = TBD;
  double t_amb = outdoor_drybulb_temp_;
  // instantaneous collector efficiency, [-]
  efficiency = (Q_inc != 0) ? slope * ((t_inlet - t_amb) / Q_inc) + intercept
                              : slope * ((t_inlet - t_amb) / Q_inc);

  // instantaneous solar gain, [W]
  auto calc_gain =  Q_inc * gross_area * efficiency;
  gain = (calc_gain > 0) ? calc_gain : 0.0;
}

 void SolarCollector::init_solar_collector()
{
  double const BigNumber(9999.9); // Component desired mass flow rate

  glycol_density_ = CollectorFluidProperties::get_density_glycol();

  mass_flow_rate_ = VolFlowRateMax * glycol_density_;

  // Collector(CollectorNum).InletTemp = Node(InletNode).Temp;

  // Collector(CollectorNum).MassFlowRate = Collector(CollectorNum).MassFlowRateMax;
}

void SolarCollector::calc_solar_collector(double q_incident,
                                          double theta,
                                          double surface_tilt_deg)
{
  // Real64 Tilt;  // Surface tilt angle (degrees)
  // Real64 ThetaBeam;        // Incident angle of beam radiation (radians)
  // Real64 InletTemp;        // Inlet temperature from plant (C)
  // Real64 OutletTemp;       // Outlet temperature or stagnation temperature in the collector (C)
  // Real64 OutletTempPrev;    // Outlet temperature saved from previous iteration for convergence
  // check (C) Real64 mCpATest;     // = MassFlowRateTest * Cp / Area (tested area) Real64
  // TestTypeMod;  // Modifier for test correlation type:  INLET, AVERAGE, or OUTLET

  // Calculate incident angle modifier
  double IncidentAngleModifier = 0.0; // Net incident angle modifier combining beam, sky, and ground radiation
  if (q_incident > 0.0)
  {
    // Calculate equivalent incident angles for sky and ground radiation according to Brandemuehl
    // and Beckman (1980)
    auto ThetaSky = (59.68 - 0.1388 * surface_tilt_deg + 0.001497 * pow2(surface_tilt_deg)) * deg_to_rad;
    auto ThetaGnd = (90.0 - 0.5788 * surface_tilt_deg + 0.002693 * pow2(surface_tilt_deg)) * deg_to_rad;

    IncidentAngleModifier = (QRadSWOutIncidentBeam * incident_angle_modifier(theta) +
                             QRadSWOutIncidentSkyDiffuse * incident_angle_modifier(ThetaSky) +
                             QRadSWOutIncidentGndDiffuse * incident_angle_modifier(ThetaGnd)) /
                             QRadSWOutIncident;
  }

  auto Cp = CollectorFluidProperties::get_specific_heat_glycol(inlet_temp_);

  auto area = 99.9;
  auto mCpA = mass_flow_rate_ * Cp / area;

  // CR 7920, changed next line to use tested area, not current surface area
  // mCpATest = Parameters(ParamNum)%TestMassFlowRate * Cp / Area
  mCpATest = Parameters(ParamNum).TestMassFlowRate * Cp /
             Parameters(Collector(CollectorNum).Parameters).Area;

  auto Iteration = 1u;
  auto OutletTemp = 0.0;
  auto OutletTempPrev = 999.9; // Set to a ridiculous number so that DO loop runs at least once
  auto TempConvergTol = 0.0;

  while (std::abs(OutletTemp - OutletTempPrev) > TempConvergTol)
  { // Check for temperature convergence

    OutletTempPrev = OutletTemp; // Save previous outlet temperature

    double FRULpTest = 0.0; // FR * ULoss "prime" for test conditions = (eff1 + eff2 * deltaT)
    FRULpTest = coeff1_ + coeff2_ * (inlet_temp_ - outdoor_drybulb_temp_);
    auto TestTypeMod = 1.0;

    // FR * tau * alpha at normal incidence = Y-intercept of collector efficiency
    double FRTAN = coeff0_ * TestTypeMod;
    // FR * ULoss = 1st order coefficient of collector efficiency
    double FRUL = coeff1_ * TestTypeMod;
    // FR * ULoss / T = 2nd order coefficent of collector efficiency
    double FRULT = coeff2_ * TestTypeMod;
    FRULpTest *= TestTypeMod;

    double FpULTest; // F prime * ULoss for test conditions = collector efficiency factor * overall
                     // loss coefficient
    double Efficiency = 0.0; // Thermal efficiency of solar energy conversion
    double Q = 0.0;
    if (mass_flow_rate_ > 0.0)
    { // Calculate efficiency and heat transfer with flow

      if ((1.0 + FRULpTest / mCpATest) > 0.0)
      {
        FpULTest = -mCpATest * std::log(1.0 + FRULpTest / mCpATest);
      }
      else
      {
        FpULTest = FRULpTest; // Avoid LOG( <0 )
      }

      double FlowMod; // Modifier for flow rate different from test flow rate
      if ((-FpULTest / mCpA) < 700.0)
      {
        FlowMod = mCpA * (1.0 - std::exp(-FpULTest / mCpA));
      }
      else
      { // avoid EXP(too large #)
        // FlowMod = FlowMod; // Self-assignment commented out
      }
      if ((-FpULTest / mCpATest) < 700.0)
      {
        FlowMod /= (mCpATest * (1.0 - std::exp(-FpULTest / mCpATest)));
      }
      else
      {
        // FlowMod = FlowMod; // Self-assignment commented out
      }

      // Calculate fluid heat gain (or loss)
      // Heat loss is possible if there is no incident radiation and fluid is still flowing.
      Q = (FRTAN * IncidentAngleModifier * QRadSWOutIncident(SurfNum) +
           FRULpTest * (inlet_temp_ - outdoor_drybulb_temp_)) *
          area * FlowMod;

      OutletTemp = inlet_temp_ + Q / (mass_flow_rate_ * Cp);

      // CR 7877 bound unreasonable result
      if (OutletTemp < -100)
      {
        OutletTemp = -100.0;
        Q = mass_flow_rate_ * Cp * (OutletTemp - inlet_temp_);
      }
      if (OutletTemp > 200)
      {
        OutletTemp = 200.0;
        Q = mass_flow_rate_ * Cp * (OutletTemp - inlet_temp_);
      }

      if (QRadSWOutIncident(SurfNum) > 0.0)
      { // Calculate thermal efficiency
        // NOTE: Efficiency can be > 1 if Q > QRadSWOutIncident because of favorable delta T, i.e.
        // warm outdoor temperature
        Efficiency = Q / (QRadSWOutIncident(SurfNum) *
                          area); // Q has units of W; QRadSWOutIncident has units of W/m2
      }
      else
      {
        Efficiency = 0.0;
      }
    }
    else
    { // Calculate stagnation temperature of fluid in collector (no flow)
      auto A = -FRULT;
      auto B = -FRUL + 2.0 * FRULT * outdoor_drybulb_temp_;
      auto C = -FRULT * pow2(outdoor_drybulb_temp_) + FRUL * outdoor_drybulb_temp_ -
               FRTAN * IncidentAngleModifier * QRadSWOutIncident(SurfNum);
      auto qEquation = (pow2(B) - 4.0 * A * C);
      if (qEquation < 0.0)
      {
        if (Collector(CollectorNum).ErrIndex == 0)
        {
          // ShowSevereMessage(
          //  "CalcSolarCollector: " + ccSimPlantEquipTypes(Collector(CollectorNum).TypeNum) + "=\""
          //  + Collector(CollectorNum).Name + "\", possible bad input coefficients.");
          // ShowContinueError("...coefficients cause negative quadratic equation part in
          // calculating "
          //                  "temperature of stagnant fluid.");
          // ShowContinueError(
          //  "...examine input coefficients for accuracy. Calculation will be treated as linear.");
        }
        // ShowRecurringSevereErrorAtEnd(
        //  "CalcSolarCollector: " + ccSimPlantEquipTypes(Collector(CollectorNum).TypeNum) + "=\"" +
        //    Collector(CollectorNum).Name + "\", coefficient error continues.",
        //  Collector(CollectorNum).ErrIndex, qEquation, qEquation);
      }
      if (FRULT == 0.0 || qEquation < 0.0)
      { // Linear, 1st order solution
        OutletTemp =
          outdoor_drybulb_temp_ - FRTAN * IncidentAngleModifier * QRadSWOutIncident(SurfNum) / FRUL;
      }
      else
      { // Quadratic, 2nd order solution
        OutletTemp = (-B + std::sqrt(qEquation)) / (2.0 * A);
      }
    }

    if (Parameters(ParamNum).TestType == INLET)
      break; // Inlet temperature test correlations do not need to iterate

    if (Iteration > 100)
    {
      if (Collector(CollectorNum).IterErrIndex == 0)
      {
        // ShowWarningMessage(
        //  "CalcSolarCollector: " + ccSimPlantEquipTypes(Collector(CollectorNum).TypeNum) + "=\"" +
        //  Collector(CollectorNum).Name + "\":  Solution did not converge.");
      }
      // ShowRecurringWarningErrorAtEnd(
      //  "CalcSolarCollector: " + ccSimPlantEquipTypes(Collector(CollectorNum).TypeNum) + "=\"" +
      //    Collector(CollectorNum).Name + "\", solution not converge error continues.",
      //  Collector(CollectorNum).IterErrIndex);
      break;
    }
    else
    {
      ++Iteration;
    }

  } // Check for temperature convergence

  Collector(CollectorNum).IncidentAngleModifier = IncidentAngleModifier;
  power_ = Q;
  Collector(CollectorNum).HeatGain = max(Q, 0.0);
  Collector(CollectorNum).HeatLoss = min(Q, 0.0);
  Collector(CollectorNum).OutletTemp = OutletTemp;
  efficiency_ = Efficiency;
}

double SolarCollector::incident_angle_modifier(double incident_angle_rad)
{
  // cut off IAM for angles greater than 60 degrees. (CR 7534)
  double CutoffAngle = 60.0 * deg_to_rad;
  double IAM = 0.0;
  if (std::abs(incident_angle_rad) <= CutoffAngle)
  {
    double s = (1.0 / std::cos(incident_angle_rad)) - 1.0;
    IAM = 1.0 + iam1_ * s + iam2_ * pow2(s);
    IAM = max(IAM, 0.0); // Never allow to be less than zero, but greater than one is a possibility
    IAM = min(IAM, 10.0); // Unlikely to be > 10; just enforce it here

  }

  return IAM;
}
