#version 330 core

layout (location = 0) in vec3 aPos;
uniform vec3 camera_pos = vec3(0, 0, 0);
uniform mat3 camera_rot = mat3(
    vec3(1, 0, 0),
    vec3(0, 1, 0),
    vec3(0, 0, 1)
);
uniform float zoom = 1;

void main() {
    gl_Position = vec4(camera_rot * (aPos - camera_pos), 1 / zoom);
}