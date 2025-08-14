#version 300 es
precision highp float;

layout(location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = aPos;
    // remove translation from the view matrix so cube always stays centered on camera
    mat4 rotView = mat4(mat3(view));
    gl_Position = projection * rotView * vec4(aPos, 1.0);
}