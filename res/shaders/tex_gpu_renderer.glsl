
#shader vertex
#version 440 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

// Shared SSBO with compute shader - use the same binding point (1)
layout(std430, binding = 1) buffer TransformBuffer {
    mat4 transforms[];
};

layout(std430, binding = 4) buffer ResultsBuffer {
    int results[];
};

// Shared SSBO with compute shader
layout(std430, binding = 10) buffer ColorBuffer {
    vec4 colors[];
};

// SSBO for texture coordinates
layout(std430, binding = 5) buffer TexCoordBuffer {
    vec2 texCoords[];
};

uniform mat4 u_cam_matrix;

out vec3 v_normal;
out vec3 v_position;
out vec4 v_color;
out vec2 v_texCoord;

void main() {
    // Cache the transform matrix - read once
    mat4 transform = transforms[gl_InstanceID];
    
    // Calculate world position once
    vec4 worldPos = transform * vec4(position, 1.0);
    v_position = worldPos.xyz;
    
    // Transform to clip space
    gl_Position = u_cam_matrix * worldPos;
    
    // Extract rotation matrix for normal transformation
    mat3 normalMatrix = mat3(transform);
    v_normal = normalMatrix * normal;
    
    // Get texture coordinates from SSBO
    // This assumes one texture coordinate per vertex
    int texCoordIndex = gl_VertexID % 3; // For triangles (3 vertices)
    v_texCoord = texCoords[texCoordIndex];
    
    // Pass the base color to fragment shader
    v_color = results[gl_InstanceID] > 0 ? vec4(1.0f, 0.0f, 0.0f, 1.0f) : colors[gl_InstanceID];
}

#shader fragment
#version 440 core

layout(location = 0) out vec4 color;

in vec3 v_normal;
in vec3 v_position;
in vec4 v_color;
in vec2 v_texCoord;

uniform vec4 u_light_color;
uniform vec3 u_light_pos;
uniform vec3 u_cam_pos;

uniform float u_a;
uniform float u_b;
uniform sampler2D u_noise_texture; // White noise texture
uniform float u_noise_intensity = 0.3; // Control noise influence

// Optimized point light calculation
vec4 pointLight(vec4 base_color) {
    // Light direction and distance calculations
    vec3 light_vector = u_light_pos - v_position;
    float distance_squared = dot(light_vector, light_vector);
    float distance = sqrt(distance_squared);
    
    // Intensity calculation
    float intensity = 1.0 / (u_a * distance_squared + u_b * distance + 1.0);
    
    // Normalize vectors
    vec3 normal = normalize(v_normal);
    vec3 light_dir = normalize(light_vector);
    
    // Calculate diffuse
    float diffuse = max(dot(normal, light_dir), 0.0);
    float ambient = 0.2;
    
    // Calculate lighting
    vec4 final_color = base_color * u_light_color * (diffuse + ambient) * intensity;
    
    // Only calculate specular if diffuse > 0
    if (diffuse > 0.0) {
        float specular_light = 0.5;
        vec3 view_dir = normalize(u_cam_pos - v_position);
        
        // Blinn-Phong halfway vector
        vec3 halfway_dir = normalize(light_dir + view_dir);
        
        // Specular term
        float spec = pow(max(dot(normal, halfway_dir), 0.0), 16);
        final_color += (specular_light * spec * u_light_color) * intensity;
    }
    
    return final_color;
}

void main() {
    // Sample the noise texture (grayscale)
    float noise = texture(u_noise_texture, v_texCoord).r;
    
    // Apply noise to the base color while preserving the original color's hue
    // Mix between full color and a noise-modulated version
    vec4 noisy_color = v_color * (1.0 - u_noise_intensity + u_noise_intensity * noise);
    
    // Calculate lighting with the noise-modulated color
    color = pointLight(noisy_color);
}