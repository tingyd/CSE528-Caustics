#version 430 core
in vec2 vUv;
out vec4 FragColor;
uniform sampler2D uRayImage;
void main() {
    vec3 col = texture(uRayImage, vUv).rgb;
    col = pow(col, vec3(1.0/2.2));  // Gamma correction
    FragColor = vec4(col, 1.0);
}