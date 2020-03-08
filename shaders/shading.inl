#ifndef GLSL_SHADING_INCLUDE_
#define GLSL_SHADING_INCLUDE_

// https://www.youtube.com/watch?v=LKnqECcg6Gw
#define POWMIX(_C1, _C2, _A) sqrt(mix((_C1)*(_C1), (_C2)*(_C2), (_A)))

// Short hand for axes unit vectors
const vec3 xhat = vec3(1.0, 0.0, 0.0);
const vec3 yhat = vec3(0.0, 1.0, 0.0);
const vec3 zhat = vec3(0.0, 0.0, 1.0);

/// Cheap diffuse lighting factor from a constant light source
float shadeConstantDiffuse(in vec3 normal, in vec3 lightDir){
    return(max(0.0, dot(normal, normalize(lightDir))));
}

const vec3 cyan = vec3(0.0, 1.0, 1.0);
const vec3 yellow = vec3(1.0, 1.0, 0.0);
const vec3 purple = vec3(1.0, 0.0, 1.0);
const vec3 blue = vec3(0.0, 0.0, 1.0);
const vec3 green = vec3(0.0, 1.0, 0.0);

/// Normal visualization shading that eliminates dark spots.
vec3 shadeByNormal(in vec3 normal){
    // Color normals in a way that does not create dark spots
    vec3 xComponent = POWMIX(blue, yellow, (dot(normal, xhat) + 1.0) * 0.5)*(.33333);
    vec3 yComponent = POWMIX(green, purple, (dot(normal, yhat) + 1.0) * 0.5)*(.33333);
    vec3 zComponent = cyan * dot(normal, -zhat) * .33333;
    return(xComponent + yComponent + zComponent);
}

#endif