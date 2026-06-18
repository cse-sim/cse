![Text](assets/images/logo-cse-white.svg)

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


## Installation

Pre-built binaries are available on the [CSE GitHub releases page](https://github.com/cse-sim/cse/releases). Download the appropriate binary for your platform and place it in a directory on your system `PATH`.

To build from source, see the [CSE repository](https://github.com/cse-sim/cse) for build instructions.

!!! info "Single-Page Option"
    The documentation is primarily a multi-page HTML site. You can also view the entire manual as a [single page](./single_page).
