# Urban Heat Island Simulation Framework  
### Real-Time Urban Thermal Modeling in C++ (SFML)

A high-performance, interactive simulation system for modeling
Urban Heat Island (UHI) dynamics using material-dependent heat transfer,
solar forcing, and real-world map-based initialization.

This project enables location-specific urban climate experiments
through direct ingestion of satellite imagery and physically motivated
thermal modeling.

---

## Overview

Urban Heat Islands emerge from the interaction between built surfaces,
natural land cover, and solar radiation.

This framework represents urban environments as computational grids
whose thermal evolution is governed by diffusion and external forcing.
Users may construct environments manually or generate them automatically
from satellite imagery, enabling rapid modeling of real locations.

The system is designed for experimentation, visualization, and
method development in urban climate analysis.

---

### Map-Based Initialization

![Satellite Import](screenshots/delhi_base1%20high%20res.png)

### Material Classification

![Material Map](screenshots/material%20map%20ss%20debug%20p3.png)

### Temperature Distribution (Day-Time Heating)

![12:20 Heatmap](screenshots/1220%20ss%20p3.png)

![16:13 Heatmap](screenshots/1613%20ss%20p3.png)

![18:40 Heatmap](screenshots/1840%20ss%20p3.png)


### Night-Time Cooling

![23:40 Heatmap](screenshots/2340%20ss%20p3.png)

![02:07 Heatmap](screenshots/0207%20ss%20p3.png)


---

## Key Capabilities

### Real-World Environment Generation

- Import satellite or map screenshots (e.g., Google Maps)
- Automatic pixel-to-material classification
- Raster-to-grid conversion pipeline
- Rapid initialization of real urban layouts
- Hybrid manual refinement

### Thermal Modeling Engine

- Discrete heat diffusion solver
- Material-dependent conductivity and heat capacity
- Explicit time integration
- Stability-controlled stepping
- Boundary-aware propagation

### Solar Forcing System

- Physically motivated diurnal cycle
- Configurable day length
- Dynamic insolation modeling
- Night-time cooling behavior

### Interactive Simulation Platform

- Real-time material editing
- Scalable brush tools
- Live parameter control
- Deterministic evolution
- High-frequency visualization

### Scientific Visualization

- Continuous temperature heatmaps
- Material classification layers
- Statistical overlays (min/max/mean)
- Multi-mode rendering
- Exportable screenshots

---

## System Architecture

The simulation operates on a structured Cartesian grid:

- Each cell represents a surface element
- Material properties determine thermal behavior
- Double-buffered temperature fields ensure numerical stability
- Update and render loops are decoupled for performance

The design emphasizes predictability, extensibility, and computational
efficiency.

---

## Image-Based Environment Pipeline

This framework includes an integrated pipeline for transforming
satellite imagery into simulation-ready domains.

### Processing Stages

1. Image acquisition
2. Color normalization
3. Feature segmentation
4. Surface classification
5. Material assignment
6. Grid resampling

### Detected Surface Types

- Built infrastructure (concrete/asphalt)
- Vegetation
- Water bodies
- Bare soil
- Road networks

Classification is performed using heuristic spectral and intensity
thresholds. Generated domains may be refined interactively.

This pipeline enables rapid case studies of real urban regions.

---

## Mathematical Model

### Spatial Discretization

- Uniform 2D lattice
- Resolution: `GRID_W × GRID_H`
- Cell size: `CELL_SIZE_PX`

Each cell stores temperature and material parameters.

### Heat Transfer

Temperature evolution follows a discrete diffusion formulation:

∂T/∂t = α.∇²T + S


Where:

- `T` is temperature
- `α` is material diffusivity
- `S` is solar forcing

Implemented using explicit finite differences.

### Solar Forcing

Incoming radiation is modeled as a periodic function with:

- Zero nocturnal input
- Peak at local noon
- Configurable amplitude and period

This approximates daily heating cycles.

---

## Workflow

A typical modeling workflow:

1. Acquire satellite imagery
2. Import and classify surface data
3. Generate material grid
4. Refine domain interactively
5. Configure simulation parameters
6. Run time evolution
7. Analyze temperature fields
8. Export results

This supports rapid experimentation and scenario comparison.

---

### Requirements

- C++17 or later
- SFML 3

---

### Limitations

This framework implements a reduced-order simplified urban climate model.

Current limitations include:

- Two-dimensional representation

- No explicit building geometry

- No radiative surface exchange

- No turbulent heat flux modeling

- Simplified atmospheric coupling

The system prioritizes interpretability and interactivity over
full-scale physical fidelity.

---

### Development Roadmap

Planned extensions:

- Wind advection modeling

- Radiative balance integration

- Diffusion stability limit for discrete timesteps

- Dataset-based validation

- Policy scenario modules

- Automated batch experiments

- Enhanced analysis tools

---

### Author

Dheirya Agrawal
2026
Independent Research Project
