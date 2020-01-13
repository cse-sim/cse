#ifndef SOLARFLATPLATECOLLECTOR_H_
#define SOLARFLATPLATECOLLECTOR_H_

class SolarFlatPlateCollector
{
public:
  SolarFlatPlateCollector();
  SolarFlatPlateCollector(double gross_area, double tilt, double azimuth, double FR_tau_alpha=0.6, double FR_UL=-4.0);
  ~SolarFlatPlateCollector() = default;

  void calculate(double Q_incident, double mass_flow_rate, double t_amb, double t_inlet);

  //Setters
  //double tilt;         ///< radians
  //double azimuth;      ///< radians
  //double FR_tau_alpha; ///< Slope
  //double FR_UL;        ///< Intercept: W/m^2-K
  //double gross_area;   ///< m^2
  // Fluid properties (Cp, density)

private:
  // Collector properties
  double tilt_;         ///< radians
  double azimuth_;      ///< radians
  double FR_UL_;        ///< Slope, W/m^2-K
  double FR_tau_alpha_; ///< Intercept, dimensionless
  double gross_area_;   ///< m^2
  double specific_heat_fluid_; ///< J / kg-K
  double density_fluid_; ///< kg / m^3

  // Collector state
  double efficiency_;   ///< fraction
  double outlet_temp_;  ///< Kelvin
};

#endif // SOLARFLATPLATECOLLECTOR_H_