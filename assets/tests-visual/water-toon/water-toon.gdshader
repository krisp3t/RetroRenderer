shader_type spatial;
render_mode blend_mix; // used to have specular phong

uniform sampler2D screen_texture : hint_screen_texture, filter_linear_mipmap;
uniform sampler2D depth_texture : hint_depth_texture, filter_linear_mipmap;

// waves
uniform sampler2D noise1;
uniform sampler2D noise2;
uniform sampler2D normalmap : hint_normal;
uniform vec4 water_color : source_color;
uniform vec2 wave_strength = vec2(0.5, 0.25);
uniform vec2 wave_frequency = vec2(12.0, 12.0);
uniform vec2 time_factor = vec2(1.0, 2.0);
uniform float speed : hint_range(-0.1, 0.1) = 0.0;
uniform float refraction : hint_range(0, 1) = 0.5;

// foam
uniform vec4 foam_color : source_color;
uniform float edge_scale = 0.25;
uniform float near = 0.1;
uniform float far = 100.f;

// fog
uniform float fog_distance : hint_range(1.0, 1000.0) = 100.0;
uniform float fog_fade_range : hint_range(1.0, 1000.0) = 50.0;
uniform vec4 fog_color : source_color = vec4(0.5, 0.5, 0.5, 1.0);
uniform float fog_intensity : hint_range(1.0, 10.0) = 5.0;

float rim(float depth)
{
	depth = 2.f * depth - 1.f;
	return near * far / (far + depth * (near - far));
}

float waves(vec2 pos, float time)
{
	return (wave_strength.y * sin(pos.y * wave_frequency.y + time * time_factor.y)) + (wave_strength.x * sin(pos.x * wave_frequency.x + time * time_factor.x));
}

void vertex()
{
	VERTEX.y += waves(VERTEX.xz, TIME);

}

void fragment()
{
    vec3 screen_color = texture(screen_texture, SCREEN_UV).rgb;
    float screen_alpha = texture(screen_texture, SCREEN_UV).a;
    float depth = texture(depth_texture, SCREEN_UV).r; // depth from 0 to 1

    vec3 ndc = vec3(SCREEN_UV, depth * 2.0 - 1.0); // Convert to NDC (in OpenGL, depth is -1 to 1)
    vec4 view = INV_PROJECTION_MATRIX * vec4(ndc, 1.0); // Convert to view space, z gives us distance to point
    view.xyz /= view.w; // Perspective division
    float linear_depth = -view.z; // Camera is facing -z

	// Sample noise textures
	float time = TIME * speed;
	vec3 n1 = texture(noise1, UV + time).rgb;
	vec3 n2 = texture(noise2, UV - time * 0.2).rgb;
	float noise_sum = (n1.r + n2.r) - 1.f;

	// Toon shading of water
	float value = 0.0;
	if (noise_sum > 0.0 && noise_sum < 0.4) value = 0.1;
	if (noise_sum > 0.4 && noise_sum < 0.8) value = 0.0;
	if (noise_sum > 0.8) value = 1.f;
	
	// Sample normal map
	vec2 uv_movement = UV * 4.f;
	uv_movement += TIME * speed * 4.f;
	// Refraction
	float z_depth = rim(depth);
	float z_pos = rim(FRAGCOORD.z);
	float diff = z_depth - z_pos;		
	vec2 displacement = vec2(noise_sum * refraction);
	diff += displacement.x * 50.f;
	vec4 alpha = vec4(1.0);
	alpha = texture(screen_texture, SCREEN_UV + displacement);

	vec4 col = mix(foam_color, water_color, step(edge_scale, diff));
	vec3 water_c = vec3(value) + mix(alpha.rgb, col.rgb, water_color.a);
	float fog_factor = clamp((z_pos - (fog_distance - fog_fade_range)) / fog_fade_range, 0.0, 1.0);
    vec3 final_color = mix(water_c, fog_color.rgb, fog_factor * fog_intensity);
	
	ALBEDO = final_color;
	ALPHA = clamp(value + water_color.a, 0.0, 1.0);
	NORMAL_MAP = texture(normalmap, uv_movement).rgb;
	ROUGHNESS = 0.1;
	SPECULAR = 1.f;
}