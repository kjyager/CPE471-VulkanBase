#version 450 core

layout(location = 0) in vec3 fragNor;
layout(location = 1) in vec3 W_fragPos;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform WorldInfo { 
    mat4 V;
    mat4 P;
    float time;
} uWorld;

layout(binding = 2) uniform AnimShadeData {
    int shadeStyle;
} uAnimShade;

#define EULERSNUM 2.718281828

// Short hand for axes unit vectors
const vec3 xhat = vec3(1.0, 0.0, 0.0);
const vec3 yhat = vec3(0.0, 1.0, 0.0);
const vec3 zhat = vec3(0.0, 0.0, 1.0);

const vec3 cyan = vec3(0.0, 1.0, 1.0);
const vec3 yellow = vec3(1.0, 1.0, 0.0);
const vec3 purple = vec3(1.0, 0.0, 1.0);
const vec3 blue = vec3(0.0, 0.0, 1.0);
const vec3 green = vec3(0.0, 1.0, 0.0);
const vec3 red = vec3(1.0, 0.0, 0.0);

const vec3 vulkanRed = vec3(0.266356, 0.009134, 0.01096);

// https://www.youtube.com/watch?v=LKnqECcg6Gw
vec3 powmix(in vec3 c1, in vec3 c2, in float alpha){
    return(sqrt(mix(c1*c1, c2*c2, alpha)));
}

// Recommended typical use is width = 1, delay = 0, halfexp = 2 // Exponent must be positive whole number greater than 0
float pulse(in float t, in float width, in float delay, in uint halfexp);

void main(){
    vec3 normal = normalize(fragNor);

    // Color normals in a way that does not creat dark spots
    vec3 x_component = powmix(blue, yellow, (dot(normal, xhat) + 1.0) * 0.5)*(.33333);
    vec3 y_component = powmix(green, purple, (dot(normal, yhat) + 1.0) * 0.5)*(.33333);
    vec3 z_component = cyan * dot(normal, -zhat) * .33333;
    vec3 norm_vis = z_component + x_component + y_component;

    // Hard coded diffuse lighting on the logo
    float fakeDiffuse1 = max(0.0, dot(normal, normalize(vec3(-1.0, -1.0, 0.5))));
    float fakeDiffuse2 = max(0.0, dot(normal, normalize(vec3(.5, 1.0, -.5))));
    vec3 glow = vec3(max(0.5, 2.5*pulse(uWorld.time, 2.0, 1.0, 1)));
    vec3 shadeRed = (glow*vulkanRed) + 1.5*vulkanRed*(fakeDiffuse1+fakeDiffuse2);

    // if shadeStyle == 1 shade it red instead of with normal based shading
    vec3 color = mix(norm_vis, shadeRed, float(uAnimShade.shadeStyle == 1));

    fragColor = vec4(color, 1.0); 
}

#define EULERSNUM 2.718281828
float pulse(in float t, in float width, in float delay, in uint halfexp){
    const float e = EULERSNUM;
    const float limit = 1e-3;
    float power = 2.0*float(halfexp);
    float period = 2.0*pow(abs(log(limit)), 1.0/power);
    float modt = mod(t/width, period+delay) - (period/2.0);
    // $ e^{-t^{power}}$ where t repeated using modulus
    return(exp(-pow(abs(modt), power)));
}