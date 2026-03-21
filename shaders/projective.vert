#version 330 core

layout (location = 0) in vec3 aPos;
uniform vec3 camera_pos;
uniform mat3 camera_rot;

void main() {
    gl_Position = vec4(camera_rot * (aPos - camera_pos), 1.0);
}