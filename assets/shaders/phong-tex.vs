#version 300 es
precision highp float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aColor;

uniform mat4 u_ModelMatrix;
uniform mat4 u_ViewProjectionMatrix;
uniform mat3 u_NormalMatrix; // derived from model matrix

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 VertexColor;

void main() {
    FragPos = vec3(u_ModelMatrix * vec4(aPos, 1.0));
    Normal = normalize(u_NormalMatrix * aNormal);
    TexCoord = aTexCoord;
    VertexColor = aColor;
    gl_Position = u_ViewProjectionMatrix * vec4(FragPos, 1.0);
}