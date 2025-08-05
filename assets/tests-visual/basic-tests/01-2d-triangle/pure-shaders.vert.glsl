#version 330 core

void main() {
    vec3 offsets[3] = vec3[](
        vec3(0.0f, 0.5f, 0.0f),
        vec3(-0.5f, -0.5f, 0.0f),
        vec3(0.5f, -0.5f, 0.0f)
    );
    gl_Position = vec4(offsets[gl_VertexID], 1.0f);
}