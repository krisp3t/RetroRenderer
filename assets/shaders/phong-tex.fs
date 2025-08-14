#version 300 es
precision highp float;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 VertexColor;

out vec4 FragColor;

uniform vec3 u_LightPos;
uniform vec3 u_ViewPos;
uniform vec3 u_LightColor;
uniform sampler2D u_Texture;
uniform float u_AmbientStrength;
uniform float u_SpecularStrength;
uniform float u_Shininess;

void main() {
    // Ambient
    vec3 ambient = u_AmbientStrength * u_LightColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(u_LightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * u_LightColor;

    // Specular
    vec3 viewDir = normalize(u_ViewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Shininess);
    vec3 specular = u_SpecularStrength * spec * u_LightColor;

    vec3 TexColor = texture(u_Texture, TexCoord).rgb;

    vec3 result = (ambient + diffuse + specular) * TexColor;
    FragColor = vec4(result, 1.0);
}