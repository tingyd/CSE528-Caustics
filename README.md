# CSE528-Caustics

GPU Monte Carlo Path Tracing + CPU Jensen Photon Mapping

This project implements two rendering pipelines inside one application:

- **GPU Mode (Key 1)** — Real-time Monte Carlo path tracer using OpenGL compute shaders  
- **CPU Mode (Key 2)** — Jensen 1996 photon mapping (caustic + global photon maps)

---

##  Build & Run Instructions

### 1. Create a build directory
```bash
mkdir build
cd build
```
2. Configure the project with CMake
```bash
cmake ..
```
3. Compile
```bash
make
```
4. Run the renderer
```bash
./renderer
```
