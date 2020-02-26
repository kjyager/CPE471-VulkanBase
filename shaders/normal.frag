#version 450 core

layout(location = 0) in vec3 fragNor;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform ViewTransforms { 
    mat4 V;
    mat4 P;
    vec3 W_ViewDir; // World view direction.
} uView;

const vec3 cyan = vec3(0.0, 1.0, 1.0);
const vec3 yellow = vec3(1.0, 1.0, 0.0);
const vec3 purple = vec3(1.0, 0.0, 1.0);
const vec3 blue = vec3(0.0, 0.0, 1.0);
const vec3 green = vec3(0.0, 1.0, 0.0);


void main(){
    vec3 normal = normalize(fragNor);

    vec3 z_component = cyan * dot(normal, -uView.W_ViewDir) * .33333;
    vec3 x_component = mix(blue, yellow, (dot(normal, uView.V[0].xyz) + 1.0) * 0.5)*(.33333);
    vec3 y_component = mix(green, purple, (dot(normal, uView.V[1].xyz) + 1.0) * 0.5)*(.33333);
    vec3 norm_vis = z_component + x_component + y_component; 

    fragColor = vec4(norm_vis, 1.0); 
}