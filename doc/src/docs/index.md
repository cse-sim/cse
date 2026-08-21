![cse logo](assets/images/logo-cse-white.svg#only-dark)
![cse logo](assets/images/logo-cse-midnight.svg#only-light)

## About

CSE began as the **C**alifornia **S**imulation **E**ngine, built primarily for residential energy code compliance in California. The "C" has since outgrown its origins — call it **C**ommon, **C**omprehensive, **C**apable, or simply CSE — as the engine has evolved into a general-purpose building simulation platform used by researchers, standards developers, and software integrators well beyond California.

CSE models annual building energy use for heating, cooling, ventilation, and lighting. It is driven by plain-text input files, making it well-suited for scripted workflows, parametric studies, and integration into larger compliance or analysis tools such as [CBECC](https://github.com/california-energy-commission/CBECC).


## Key Features

- **Open source** — CSE is freely available on [GitHub](https://github.com/cse-sim/cse) under a permissive license, making it easy to inspect, extend, and integrate into custom tools.

- **Plain-text, stable input format** — Input files are human-readable, diff cleanly, and remain backwards-compatible across releases — making CSE easy to integrate into automated pipelines and long-lived toolchains without migration overhead.

- **Fast runtimes** — Annual simulations complete in seconds, making CSE practical for parametric sweeps and large compliance batch runs.

- **Powerful expression language** — Any input can be a time-varying expression referencing weather, time-of-year, or other model state variables, enabling precise dynamic control without preprocessor workarounds. There is no need to define special "sensors" or "actuators".

- **Pressure-based airflow network** — Infiltration and mechanical ventilation are modeled as a fully-coupled pressure network, capturing interactions between envelope leakage, fans, and HVAC that other simulation engines treat independently.

- **Photovoltaic & battery storage** — PV arrays and battery systems are first-class model components, with shading analysis, string losses, and configurable dispatch controls including grid-responsive peak-saving strategies.

- **Minute-resolution domestic hot water** — DHW draws are simulated as discrete events at one-minute timesteps, enabling accurate evaluation of heat pump water heater controls and tank recovery dynamics that cannot be accurately represented in engines with coupled DHW timesteps.


## Comparison with EnergyPlus

EnergyPlus is a powerful, widely used simulation engine and a good fit for many applications,
though not all. CSE was built with different priorities, and the tradeoffs that follow are
worth noting.

- **Runtime** — For comparable annual simulations, CSE runs faster than EnergyPlus.[^1] Both
  engines have gotten faster since that comparison was published, but the relative advantage
  still makes CSE well suited to parametric sweeps and large compliance batch runs, where
  runtime differences compound across many simulations.

- **Architecture** — CSE has utilized modern practices for object-oriented design from the beginning, rather
  than the global state variables EnergyPlus relies on.[^1] That structure makes CSE's codebase
  less error-prone and easier to extend safely; EnergyPlus's global-state design, by contrast,
  is not thread-safe.

- **Accuracy** — In direct comparison, CSE and EnergyPlus track measured data from the Central
  Valley Research Homes project comparably.[^1] Neither tool shows a clear accuracy advantage;
  the differences between them are in runtime and architecture, not fidelity.

- **Scope & focus** — Both tools set out to be general-purpose building energy simulation
  engines, but their development has grown in different directions. CSE has concentrated on
  residential-scale modeling: domestic hot water, duct systems, envelope, and infiltration.
  EnergyPlus has grown a much broader library of commercial air and plant systems.

[^1]: Kruis, N., M. Larson, B. Wilcox, and C.S. Barnaby (2019). "A Comparison of CSE and
    EnergyPlus for Residential Energy Calculations." Proceedings of the 16th IBPSA Conference,
    Rome, Italy. https://doi.org/10.26868/25222708.2019.210839


## Validation & Adoption

CSE has been evaluated against ANSI/ASHRAE Standard 140 ("BESTEST") and serves as a reference
program for the acceptance criteria of the standard's thermal fabric test cases, as well as
for RESNET's HERS rating software accreditation testing. Beyond that baseline, CSE has a track
record of adoption and validation across multiple contexts:

- **Official compliance engine** — Within [CBECC](https://github.com/california-energy-commission/CBECC),
  the California Energy Commission's compliance software for Title 24, CSE performs the
  simulations for residential buildings (including multifamily dwelling units and common
  areas); non-residential buildings are modeled with EnergyPlus.
- **Utility program adoption** — CSE was used by Big Ladder Software to model and calibrate
  advanced heat pump savings rates for the [Northwest Energy Efficiency Alliance
  (NEEA)](https://neea.org/resource/advanced-heat-pump-savings-rate-modeling/).
- **Peer-reviewed validation** — CSE's algorithms are documented and validated against
  measured data from the Central Valley Research Homes project in Barnaby, C., B. Wilcox,
  and P. Niles (2013). "Development and Validation of the California Simulation Engine."
  Proceedings of Building Simulation 2013, Chambéry, France.
- **Cross-engine comparison** — Kruis, N., M. Larson, B. Wilcox, and C.S. Barnaby (2019).
  "A Comparison of CSE and EnergyPlus for Residential Energy Calculations." Proceedings of
  the 16th IBPSA Conference, Rome, Italy. https://doi.org/10.26868/25222708.2019.210839


## Installation

Pre-built binaries are available on the [CSE GitHub releases page](https://github.com/cse-sim/cse/releases). Download the appropriate binary for your platform and place it in a directory on your system `PATH`.

To build from source, see the [CSE repository](https://github.com/cse-sim/cse) for build instructions.

!!! info "Single-Page Option"
    The documentation is primarily a multi-page HTML site. You can also view the entire manual as a [single page](./single_page).
