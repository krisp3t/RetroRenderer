#version 330 core

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 u_ModelMatrix;
uniform mat4 u_ViewProjectionMatrix;

out vec3 FragPos;
out vec2 TexCoord;

void main() {
    FragPos = vec3(u_ModelMatrix * vec4(aPos, 1.0));
    TexCoord = aTexCoord;
    gl_Position = u_ViewProjectionMatrix * vec4(FragPos, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
in vec3 FragPos;
in vec2 TexCoord;

out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
#endif
