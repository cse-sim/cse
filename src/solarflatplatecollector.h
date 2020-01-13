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
  inline double set_tilt(double tilt) { tilt_ = tilt; }
  inline double set_azimuth(double azm) { azimuth_ = azm; }
  inline double set_FR_tau_alpha(double tau_alpha) { FR_tau_alpha_ = tau_alpha; }
  inline double set_FR_UL(double FRUL) {FR_UL_ = FRUL; }
  inline double set_gross_area(double area) { gross_area_ = area; }
  // Fluid properties (Cp, density)
  inline double set_specific_heat_fluid(double Cp) { specific_heat_fluid_ = Cp; }
  inline double set_density_fluid(double density) { density_fluid_ = density; }

  double outlet_temp() { return outlet_temp_; }

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