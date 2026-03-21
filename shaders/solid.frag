#version 330 core

out vec4 FragColor;
uniform vec4 fill_color;

void main() {
   FragColor = fill_color;
}