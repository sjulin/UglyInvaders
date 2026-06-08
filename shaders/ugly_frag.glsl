#version 460

layout(location = 0) in vec3 inTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform SceneUBO {
    mat4 view;
    mat4 proj;
    vec3 eye;
    float padding1;
    vec3 center;
    float padding2;
    vec3 up;
    float padding3;
    vec3 zenithColor;      // Top of sky
    float padding4;
    vec3 midHighColor;     // Upper middle
    float padding5;
    vec3 horizonColor;     // Horizon
    float padding6;
    vec3 midLowColor;      // Lower middle
    float padding7;
    vec3 nadirColor;       // Bottom (ground)
    float cloudCoverage;   // Cloud density/coverage (0-1)
    vec3 cloudOffset;      // Cloud animation offset
    float cloudSharpness;  // Cloud contrast/sharpness
} scene;

layout(set = 1, binding = 0) uniform sampler2D textures[16];

layout(push_constant) uniform PushConstants {
    uint instanceIndex;
} pc;

// 3D noise functions for seamless clouds
float hash3(vec3 p) {
    p = fract(p * vec3(127.1, 311.7, 74.7));
    p += dot(p, p.yzx + 19.19);
    return fract((p.x + p.y) * p.z);
}

float noise3D(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    
    return mix(
        mix(mix(hash3(i + vec3(0,0,0)), hash3(i + vec3(1,0,0)), f.x),
            mix(hash3(i + vec3(0,1,0)), hash3(i + vec3(1,1,0)), f.x), f.y),
        mix(mix(hash3(i + vec3(0,0,1)), hash3(i + vec3(1,0,1)), f.x),
            mix(hash3(i + vec3(0,1,1)), hash3(i + vec3(1,1,1)), f.x), f.y),
        f.z);
}

void main() {
    vec3 dir = normalize(inTexCoord);
    
    // Smooth 5-color gradient without branches
    float t = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);  // Remap from [-1,1] to [0,1]
    
    // Create blend weights for each transition
    float t0 = clamp(t * 4.0, 0.0, 1.0);           // nadir -> midLow
    float t1 = clamp(t * 4.0 - 1.0, 0.0, 1.0);     // midLow -> horizon
    float t2 = clamp(t * 4.0 - 2.0, 0.0, 1.0);     // horizon -> midHigh
    float t3 = clamp(t * 4.0 - 3.0, 0.0, 1.0);     // midHigh -> zenith
    
    // Blend progressively through all colors
    vec3 skyColor = scene.nadirColor;
    skyColor = mix(skyColor, scene.midLowColor, t0);
    skyColor = mix(skyColor, scene.horizonColor, t1);
    skyColor = mix(skyColor, scene.midHighColor, t2);
    skyColor = mix(skyColor, scene.zenithColor, t3);
    
    // Add cloud-like pattern only to sky (dir.y > 0) using seamless 3D noise
    if (dir.y > 0.0 && scene.cloudCoverage > 0.0) {
        // Use 3D direction with animation offset - no seams or singularities
        vec3 animDir = dir + scene.cloudOffset;
        
        // Multiple octaves for detailed, fluffy clouds
        float n = noise3D(animDir * 3.0) * 0.5;     // Base large shapes
        n += noise3D(animDir * 7.0) * 0.25;         // Medium detail
        n += noise3D(animDir * 14.0) * 0.125;       // Fine detail
        n += noise3D(animDir * 28.0) * 0.0625;      // Very fine detail
        n += noise3D(animDir * 56.0) * 0.03125;     // Fluffy edges
        
        // Apply coverage and sharpness
        n = pow(max(n - (1.0 - scene.cloudCoverage) * 0.5, 0.0) / scene.cloudCoverage, scene.cloudSharpness);
        
        vec3 clouds = vec3(n) * 1.2;
        skyColor += clouds * abs(dir.y);  // More visible at zenith, less at horizon

    }
    
    outColor = vec4(skyColor, 1.0);
}
