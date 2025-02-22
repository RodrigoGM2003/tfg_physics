#shader vertex
#version 330 core

layout(location = 0) in vec4 position;

layout(location = 1) in vec4 color;

layout(location = 2) in vec3 normal;

uniform mat4 u_model;
uniform mat4 u_cam_matrix;

void main(){
    gl_Position = u_cam_matrix * u_model * position;
};

#shader fragment
#version 330 core

layout(location = 0) out vec4 color;

uniform vec4 u_light_color;

void main(){
    color = u_light_color;
};