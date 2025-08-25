C++ real-time software rasterizer. The primary goal of this project is to deepen understanding of 3D graphics fundamentals, rendering pipeline and graphics hardware.

Full OpenGL implementation (hardware rendering) is also available as a comparison.

-- Work in progress, actively being worked on --

## Screenshots
![Screenshot 2024-12-08 230126](https://github.com/user-attachments/assets/0177bdb7-dbbd-4d12-adbf-01671a34f70f)


## Table of Contents

## Motivation
A great starting point to understand the graphics pipeline, since all stages have to be manually programmed in a software rasterizer, whereas they are run hardwired in GPUs when using hardware acceleration.

## Features
- 3D scene loading with scene graph inspector, where you can manipulate nodes runtime.
- Orbiting, free moving camera.
- Fully customizable vertex and fragment shader with texture binding. Flat and varying interpolation available.
- Frustum, backface culling, depth test with z-buffer.
- Customizable pipeline with many configuration options, available runtime in Dear Imgui windows.
- Perspective correct, affine texture mapping.
- Rasterization rules adhering rasterizer.
- Flat, Gourard, Phong, Blinn-Phong shading.
- Gamma correction.

## Build Instructions
This project uses cmake as a build system and vcpkg as a package manager. [More build instructions will be available as the project will be closer to stable status]

### Run Instructions
Launch the project with launch profile "Launch RetroRenderer in workspaceRoot" to assure assets will be found correctly.

## References
- [Joey de Vries: LearnOpenGL](https://learnopengl.com/)
- [ChiliTomatoNoodle: 3D Programming Fundamentals](https://www.youtube.com/watch?v=uehGqieEbus&list=PLqCJpWy5Fohe8ucwhksiv9hTF5sfid8lA)
- [Dmitry V. Sokolov: TinyRenderer](https://github.com/ssloy/tinyrenderer)
- [Jean-Colas Prunier: Scratchapixel](https://www.scratchapixel.com/)

## Assets
- [Seamless Sky Backgrounds](https://opengameart.org/content/seamless-sky-backgrounds)
