#version 450 core

// ~~~~~~~~ Inputs and Uniforms ~~~~~~~~

layout(location = 0) in vec3 W_fragNor;
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



// ~~~~~~~~ Forward Declarations ~~~~~~~~

// https://www.youtube.com/watch?v=LKnqECcg6Gw
#define POWMIX(_C1, _C2, _A) sqrt(mix((_C1)*(_C1), (_C2)*(_C2), (_A)))

// Short hand for axes unit vectors
const vec3 xhat = vec3(1.0, 0.0, 0.0);
const vec3 yhat = vec3(0.0, 1.0, 0.0);
const vec3 zhat = vec3(0.0, 0.0, 1.0);

/// Normal visualization shading that eliminates dark spots.
vec3 shadeByNormal(in vec3 normal);

/// Cheap diffuse lighting factor from a constant light source
float shadeConstantDiffuse(in vec3 normal, in vec3 lightDir);



// ~~~~~~~~ Function Definitions ~~~~~~~~

void main(){
    vec3 normal = normalize(W_fragNor);

    vec3 normVis = shadeByNormal(normal);

    // Hard coded diffuse lighting on the logo
    // Lab 10: Replace with full Blinn-Phong
    const vec3 vulkanRed = vec3(0.266356, 0.009134, 0.01096);
    float diffuse1 = shadeConstantDiffuse(normal, normalize(vec3(-1.0, -1.0, 0.5)));
    float diffuse2 = shadeConstantDiffuse(normal, normalize(vec3(.5, 1.0, -.5)));
    vec3 shadeLogo = 0.5*vulkanRed + 1.5*vulkanRed*(diffuse1 + diffuse2);

    // if shadeStyle == 1 shade it red instead of with normal based shading
    vec3 color = mix(normVis, shadeLogo, float(uAnimShade.shadeStyle == 1));

    fragColor = vec4(color, 1.0); 
}

const vec3 cyan = vec3(0.0, 1.0, 1.0);
const vec3 yellow = vec3(1.0, 1.0, 0.0);
const vec3 purple = vec3(1.0, 0.0, 1.0);
const vec3 blue = vec3(0.0, 0.0, 1.0);
const vec3 green = vec3(0.0, 1.0, 0.0);

float shadeConstantDiffuse(in vec3 normal, in vec3 lightDir){
    return(max(0.0, dot(normal, normalize(lightDir))));
}

vec3 shadeByNormal(in vec3 normal){
    // Color normals in a way that does not create dark spots
    vec3 xComponent = POWMIX(blue, yellow, (dot(normal, xhat) + 1.0) * 0.5)*(.33333);
    vec3 yComponent = POWMIX(green, purple, (dot(normal, yhat) + 1.0) * 0.5)*(.33333);
    vec3 zComponent = cyan * dot(normal, -zhat) * .33333;
    return(xComponent + yComponent + zComponent);
}