#version 330 core

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;

uniform mat4 uPVM;
uniform mat4 uModel;
uniform mat4 uView;
uniform float uTime;
uniform bool uAnimateTail;

out vec2 vTexCoord;
out vec3 vNormal;
out vec3 vFragPos;
out vec3 vViewPos;

void main()
{
    vec3 pos    = inPosition;
    vec3 normal = inNormal;

    if (uAnimateTail)
    {
        float tailStartZ = -4.45;
        vec3 pivot = vec3(-0.263, -0.021, tailStartZ);

        if (pos.z < tailStartZ)
        {
            float distancia = tailStartZ - pos.z;
            float flex = distancia * 0.4;  // Más lejos = más se mueve
            
            float angle = sin(uTime * 4.0) * 0.5 * flex;
            
            vec3 p = pos - pivot;
            float c = cos(angle);
            float s = sin(angle);
            
            pos.x = c * p.x + s * p.z + pivot.x;
            pos.z = -s * p.x + c * p.z + pivot.z;

            vec3 rotatedNormal;
            rotatedNormal.x = c * normal.x + s * normal.z;
            rotatedNormal.y = normal.y;
            rotatedNormal.z = -s * normal.x + c * normal.z;
            normal = rotatedNormal;
        }
    }

    vFragPos = vec3(uModel * vec4(pos, 1.0));
    vNormal  = mat3(transpose(inverse(uModel))) * normal;
    vViewPos = -vec3(uView[3][0], uView[3][1], uView[3][2]);

    gl_Position = uPVM * vec4(pos, 1.0);
    vTexCoord = inTexCoord;
}
