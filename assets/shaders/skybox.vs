layout(location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 u_ViewMatrix;
uniform mat4 u_ProjectionMatrix;

void main()
{
    TexCoords = aPos;
    // remove translation from view matrix so skybox is infinitely far
    mat4 view = mat4(mat3(u_ViewMatrix));
    gl_Position = u_ProjectionMatrix * view * vec4(aPos, 1.0);
}