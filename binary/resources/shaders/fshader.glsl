#version 330 core

in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vFragPos;
in vec3 vViewPos;

uniform vec4 uColor;
uniform sampler2D uTexture;
uniform bool useTexture;
uniform bool uEnableLighting;

// Sistema de iluminación
uniform vec3 uAmbientLight;        
uniform vec3 uDirLightDir;         
uniform vec3 uDirLightColor;       
uniform vec3 uMovingLightPos;   
uniform vec3 uMovingLightColor;   
uniform bool uMovingLightEnabled;  

out vec4 outColor;

void main() {
    // Color base
    vec4 baseColor;
    if (useTexture) {
        vec4 texColor = texture(uTexture, vTexCoord);
        baseColor = texColor * uColor;
    } else {
        baseColor = uColor;
    }
    
    // Si la iluminación está desactivada, devolver color base
    if (!uEnableLighting) {
        outColor = baseColor;
        return;
    }
    
    vec3 norm = normalize(vNormal);
    vec3 viewDir = normalize(vViewPos - vFragPos);
    
    vec3 ambient = uAmbientLight;
    
    vec3 dirLightDir = normalize(-uDirLightDir);
    
    // Difusa
    float dirDiff = max(dot(norm, dirLightDir), 0.0);
    vec3 dirDiffuse = dirDiff * uDirLightColor;
    
    // Especular
    vec3 dirReflectDir = reflect(-dirLightDir, norm);
    float dirSpec = pow(max(dot(viewDir, dirReflectDir), 0.0), 32.0);
    vec3 dirSpecular = 0.3 * dirSpec * uDirLightColor;
    
    vec3 movingDiffuse = vec3(0.0);
    vec3 movingSpecular = vec3(0.0);
    
    if (uMovingLightEnabled) {
        vec3 movingLightDir = normalize(uMovingLightPos - vFragPos);
        float distance = length(uMovingLightPos - vFragPos);
        
        // Atenuación por distancia (reducida para mayor alcance)
        float attenuation = 1.0 / (1.0 + 0.045 * distance + 0.0075 * distance * distance);
        
        // Difusa
        float movingDiff = max(dot(norm, movingLightDir), 0.0);
        movingDiffuse = movingDiff * uMovingLightColor * attenuation;
        
        // Especular
        vec3 movingReflectDir = reflect(-movingLightDir, norm);
        float movingSpec = pow(max(dot(viewDir, movingReflectDir), 0.0), 64.0);
        movingSpecular = 1.5 * movingSpec * uMovingLightColor * attenuation;
    }
    
    vec3 lighting = ambient + dirDiffuse + dirSpecular + movingDiffuse + movingSpecular;
    
    outColor = vec4(lighting, 1.0) * baseColor;
    outColor.a = baseColor.a;
}
