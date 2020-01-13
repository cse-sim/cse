#include "solarflatplatecollector.h"
#include <algorithm>

SolarFlatPlateCollector::SolarFlatPlateCollector()
  : tilt_(0.0),
    azimuth_(0.0),
    FR_tau_alpha_(0.6),
    FR_UL_(-4),
    gross_area_(1.0),
    specific_heat_fluid_(4181.6),
    density_fluid_(1000.0),
    efficiency_(1.0),
    outlet_temp_(283)
{
}

SolarFlatPlateCollector::SolarFlatPlateCollector(double gross_area,
                                                 double tilt,
                                                 double azimuth,
                                                 double FR_tau_alpha,
                                                 double FR_UL)
  : tilt_(tilt),
    azimuth_(azimuth),
    FR_tau_alpha_(FR_tau_alpha),
    FR_UL_(FR_UL),
    gross_area_(gross_area),
    specific_heat_fluid_(4181.6),
    density_fluid_(1000.0),
    efficiency_(1.0),
    outlet_temp_(283)
{
}

void SolarFlatPlateCollector::calculate(double Q_incident,
                                        double mass_flow_rate,
                                        double ambient_temp,
                                        double inlet_temp)
{
  // instantaneous collector efficiency, [-]
  if (Q_incident != 0.0)
  {
    efficiency_ = FR_UL_ * ((inlet_temp - ambient_temp) / Q_incident) + FR_tau_alpha_;
    efficiency_ = std::min(1.0, efficiency_);
    // instantaneous solar gain, [W]
    auto heat_gain = Q_incident * gross_area_ * efficiency_;
    outlet_temp_ = inlet_temp + heat_gain / (mass_flow_rate * specific_heat_fluid_);
  }
  else
  {
    efficiency_ = 0.0;
    outlet_temp_ = inlet_temp;
  }
}
