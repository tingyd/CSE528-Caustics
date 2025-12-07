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

## Credits & Citations

**1. OpenGL Boilerplate (Window, Context, Shaders):**
- Derived from [LearnOpenGL.com](https://learnopengl.com/)
- Uses [GLFW](https://www.glfw.org/) for windowing and [GLAD](https://glad.dav1d.de/) for extension loading.

**2. Mathematical Utilities:**
- Uses the [GLM (OpenGL Mathematics)](https://github.com/g-truc/glm) library.

**3. Rendering Algorithms:**
- **CPU Photon Mapping:** Implemented based on *Jensen, H. W. (1996). Global illumination using photon maps.*
- **GPU Path Tracing:** Standard Monte Carlo integration via OpenGL Compute Shaders.
