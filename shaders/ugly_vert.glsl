#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;

layout(location = 0) out vec3 outTexCoord;

layout(push_constant) uniform PushConstants {
    uint modIdx;
} pc;

struct GPUModel {
    mat4 modMtx;
    mat4 norMtx;
    uint mtrIdx;
    uint flags;
    uint padding[2];
};

layout(set = 0, binding = 0) readonly buffer GPUModelBuf {
    GPUModel models[];
};
layout(set = 0, binding = 1) uniform GPUScene {
    mat4 view;
    mat4 proj;
    vec3 eye;
    float padding1;
    vec3 center;
    float padding2;
    vec3 up;
    float padding3;
    vec3 zenithColor;
    float padding4;
    vec3 midHighColor;
    float padding5;
    vec3 horizonColor;
    float padding6;
    vec3 midLowColor;
    float padding7;
    vec3 nadirColor;
    float cloudCoverage;
    vec3 cloudOffset;
    float cloudSharpness;
} scene;

void main() {
    // Sky dome stays centered on camera
    mat4 viewNoTranslation = scene.view;
    viewNoTranslation[3][0] = 0.0;
    viewNoTranslation[3][1] = 0.0;
    viewNoTranslation[3][2] = 0.0;
    
    vec4 pos = scene.proj * viewNoTranslation * vec4(inPosition, 1.0);
    gl_Position = pos.xyww;  // Set depth to 1.0 (far plane)
    
    outTexCoord = inPosition;
}
