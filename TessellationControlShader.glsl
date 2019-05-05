#version 440

//numbers of control points (CPs) in the output patch
layout (vertices = 3) out;

uniform vec3 camPos;

in vec3 position_TCS_in[];
in vec2 tex_TCS_in[];
in vec3 normals_TCS_in[];
in vec3 tangent_TCS_in[];

out vec3 position_TES_in[];
out vec2 tex_TES_in[];
out vec3 normals_TES_in[];
out vec3 tangent_TES_in[];

float getTessLevel(float dist, float dist2){
    float averageDist = (dist + dist2) / 2.0f;

    if(averageDist <= 2.0f) {
        return 10.0f;
    }
    else if(averageDist <= 5.0f){
        return 8.0f;
    }
    else if(averageDist <= 8.0f){
        return 5.0f;
    }
    else if(averageDist <= 10.0f){
        return 3.0f;
    }
    else {
        return 1.0f;
    }
} 

void main() {
    //setting the CPs to the output patch
    position_TES_in[gl_InvocationID] = position_TCS_in[gl_InvocationID];
    tex_TES_in[gl_InvocationID] = tex_TCS_in[gl_InvocationID];
    normals_TES_in[gl_InvocationID] = normals_TCS_in[gl_InvocationID];
    tangent_TES_in[gl_InvocationID] = tangent_TCS_in[gl_InvocationID];

    //calculating the distance from the camera to the CPs
    float camToCPDistance0 = distance(camPos, position_TES_in[0]);
    float camToCPDistance1 = distance(camPos, position_TES_in[1]);
    float camToCPDistance2 = distance(camPos, position_TES_in[2]);

    //calculating the tessellation levels
    //TessLevelOuter roughly determines the number of segments on each edge
    gl_TessLevelOuter[0] = getTessLevel(camToCPDistance1, camToCPDistance2);
    gl_TessLevelOuter[1] = getTessLevel(camToCPDistance2, camToCPDistance0);
    gl_TessLevelOuter[2] = getTessLevel(camToCPDistance0, camToCPDistance1);
    //TessLevelInner roughly determines how many rings the triangle will contain
    gl_TessLevelInner[0] = gl_TessLevelOuter[2];
}