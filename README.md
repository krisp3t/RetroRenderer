C++ real-time software rasterizer with OpenGL implementation for comparison. 

A great starting point to understand the graphics pipeline, since all stages have to be manually programmed in a software rasterizer, whereas they are run hardwired in GPUs when using hardware acceleration.

-- Work in progress, actively being worked on --

## Dependencies
- ninja
- vcpkg (either system installation or as git submodule)
- assimp
- SDL2
- OpenGL (if building with OpenGL support)
- glm

## Instructions
Launch the project with launch profile "Launch RetroRenderer in workspaceRoot" to assure assets will be found correctly.