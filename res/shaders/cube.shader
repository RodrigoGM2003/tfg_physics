#shader vertex
#version 330 core

layout(location = 0) in vec3 position;

layout(location = 1) in vec4 color;

layout(location = 2) in vec3 normal;

uniform mat4 u_model;
uniform mat4 u_cam_matrix;

out vec3 v_normal;
out vec3 v_position;
out vec4 v_color;

void main(){
    v_position = vec3(u_model * vec4(position, 1.0));

    gl_Position = u_cam_matrix * vec4(v_position, 1.0);

    v_normal = vec3(u_model * vec4(normal, 0.0));
    v_color = color;
};

#shader fragment
#version 330 core

layout(location = 0) out vec4 color;

in vec3 v_normal;
in vec3 v_position;
in vec4 v_color;

uniform vec4 u_light_color;
uniform vec3 u_light_pos;
uniform vec3 u_cam_pos;

uniform float u_a;
uniform float u_b;

vec4 pointLight(){
    vec3 light_vector = u_light_pos - v_position;
    float distance = length(light_vector);
    float intensity = 1.0 / (u_a * distance * distance + u_b * distance + 1.0);

    float ambient = 0.2;

    vec3 normal = normalize(v_normal);
    vec3 light_dir = normalize(light_vector);

    float diffuse = max(dot(normal, light_dir), 0.0);


    vec4 specular = vec4(0.0);

    if(diffuse != 0.0){
        float specular_light = 0.5f;
        vec3 view_dir = normalize(u_cam_pos - v_position);
        vec3 reflect_dir = reflect(-light_dir, normal);

        vec3 halfway_dir = normalize(light_dir + view_dir);

        float spec = pow(max(dot(normal, halfway_dir), 0.0), 16);
        specular = specular_light * spec * u_light_color;
    };

    return v_color * u_light_color * (diffuse + ambient) * intensity + (specular * intensity);
}

void main(){
    color = pointLight();
};