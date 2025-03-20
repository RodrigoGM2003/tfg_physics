#shader vertex
#version 440 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

// Shared SSBO with compute shader - use the same binding point (1)
layout(std430, binding = 1) buffer TransformBuffer {
    mat4 transforms[];
};

// Shared SSBO with compute shader - use the same binding point (1)
layout(std430, binding = 10) buffer ColorBuffer {
    vec4 colors[];
};

uniform mat4 u_cam_matrix;
uniform vec4 u_instance_color; // New uniform for constant color

out vec3 v_normal;
out vec3 v_position;
out vec4 v_color;

void main() {
    // Cache the transform matrix - read once
    mat4 transform = transforms[gl_InstanceID];
    
    // Calculate world position once
    vec4 worldPos = transform * vec4(position, 1.0);
    v_position = worldPos.xyz;
    
    // Transform to clip space
    gl_Position = u_cam_matrix * worldPos;
    
    // Extract rotation matrix for normal transformation (more efficient)
    mat3 normalMatrix = mat3(transform);
    v_normal = normalMatrix * normal;
    
    // Use constant color from uniform (or keep using attribute if needed)
    v_color = colors[gl_InstanceID];
}

#shader fragment
#shader fragment
#version 440 core

layout(location = 0) out vec4 color;

in vec3 v_normal;
in vec3 v_position;
in vec4 v_color;

uniform vec4 u_light_color;
uniform vec3 u_light_pos;
uniform vec3 u_cam_pos;

uniform float u_a;
uniform float u_b;

// Optimized point light calculation
vec4 pointLight() {
    // Light direction and distance calculations (optimized)
    vec3 light_vector = u_light_pos - v_position;
    float distance_squared = dot(light_vector, light_vector);
    float distance = sqrt(distance_squared);
    
    // More efficient intensity calculation
    float intensity = 1.0 / (u_a * distance_squared + u_b * distance + 1.0);
    
    // Normalize vectors once
    vec3 normal = normalize(v_normal);
    vec3 light_dir = normalize(light_vector);
    
    // Calculate diffuse once
    float diffuse = max(dot(normal, light_dir), 0.0);
    float ambient = 0.2;
    
    // Avoid specular calculation if diffuse is zero
    vec4 final_color = v_color * u_light_color * (diffuse + ambient) * intensity;
    
    // Only calculate specular if diffuse > 0 (avoid branch with step function)
    if (diffuse > 0.0) {
        float specular_light = 0.5;
        vec3 view_dir = normalize(u_cam_pos - v_position);
        
        // Blinn-Phong halfway vector (more efficient than reflect)
        vec3 halfway_dir = normalize(light_dir + view_dir);
        
        // Specular term
        float spec = pow(max(dot(normal, halfway_dir), 0.0), 16);
        final_color += (specular_light * spec * u_light_color) * intensity;
    }
    
    return final_color;
}

void main() {
    color = pointLight();
}