#version 420

uniform mat4 MVP;

in vec3 a_position;
in vec2 a_texCoord;

out vec2 v_texCoord;

void main() {
	gl_Position = MVP * vec4(a_position, 1);
	v_texCoord = a_texCoord;
}
