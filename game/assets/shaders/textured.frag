#version 420

uniform sampler2D tex;

in vec2 v_texCoord;

out vec4 finalColor;

void main() {
	finalColor = vec4(texture(tex, v_texCoord).rgb, 1.0);
}
