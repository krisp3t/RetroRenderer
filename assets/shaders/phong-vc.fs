in vec3 FragPos;
in vec3 Normal;
in vec3 VertexColor;

out vec4 FragColor;

uniform vec3 u_LightPos;
uniform vec3 u_ViewPos;
uniform vec3 u_LightColor;
uniform sampler2D u_Texture;
uniform float u_AmbientStrength;
uniform float u_SpecularStrength;
uniform float u_Shininess;

float ComputeAttenuation(float lightDistance) {
    return 1.0 / (1.0 + 0.09 * lightDistance + 0.032 * lightDistance * lightDistance);
}

void main() {
    // Ambient
    vec3 ambient = u_AmbientStrength * u_LightColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightVector = u_LightPos - FragPos;
    float lightDistance = length(lightVector);
    vec3 lightDir = lightDistance > 0.00001 ? lightVector / lightDistance : vec3(0.0, 0.0, 1.0);
    float attenuation = ComputeAttenuation(lightDistance);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * u_LightColor * attenuation;

    // Specular
    vec3 viewDir = normalize(u_ViewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Shininess);
    vec3 specular = u_SpecularStrength * spec * u_LightColor * attenuation;

    vec3 result = (ambient + diffuse + specular) * VertexColor;
    FragColor = vec4(result, 1.0);
}
