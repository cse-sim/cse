#ifndef SOLAR_COLLECTOR_H_
#define SOLAR_COLLECTOR_H_

#include <string>

// skip namespace for now
//

/// @class CollectorFluidProperties solarcollector.h
/// @brief This fluid properties class is specific to SolarCollector, just as a minimal support
/// class
///        to get things moving.

class CollectorFluidProperties
{
public:
  static double get_density_glycol() { return 1.0; }
  static double get_specific_heat_glycol(double inlet_temp) { return 1.0; }
};

/// @class SolarCollector solarcollector.h

class SolarCollector
{
public:
  SolarCollector();
  ~SolarCollector();

  // Units??
  double transmitted_radiance_perez(double tilt, 
                                    double azimuth);

  void simple_solar_collector(double tilt, 
                              double azimuth,
                              double slope,
                              double intercept,
                              double gross_area,
                              double &gain,
                              double &efficiency);

  void init_solar_collector();

  void calc_solar_collector(double q_incident, // W/m^2
                            double theta,      // solar incidence angle
                            double surface_tilt_deg);

private:
  double incident_angle_modifier(double incident_angle_rad);

  double glycol_density_; // rho
  double mass_flow_rate_; // m_dot
  double inlet_temp_;
  double outdoor_drybulb_temp_;
  double power_; // Q
  double efficiency_;

  double coeff0_; // Offset coefficient of the Efficiency Equation (tau_alpha)
  double coeff1_; // Linear coefficient of the Efficiency Equation (FR)
  double coeff2_; // Quadratic coefficient of the Efficiency Equation

  double iam1_;
  double iam2_;

  // Tilt, slope, intercept, area
};

#endif SOLAR_COLLECTOR_H_