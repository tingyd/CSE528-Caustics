# CSE528-Caustics

GPU Monte Carlo Path Tracing + CPU Jensen Photon Mapping

This project implements two rendering pipelines inside one application:

- **GPU Mode (Key 1)** — Real-time Monte Carlo path tracer using OpenGL compute shaders  
- **CPU Mode (Key 2)** — Jensen 1996 photon mapping (caustic + global photon maps)

---

##  Build & Run Instructions

```bash
mkdir build
cd build
cmake ..
make
./renderer
```

Dependencies:
- **C++17**
- **OpenGL 4.3+**
- **GLFW**
- **GLAD**
- **GLM** 
- **OpenMP**
