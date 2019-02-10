#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// Input uniform values
uniform mat4 mvp;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Output vertex attributes (to fragment shader)
out vec2 fragTexCoord;
out vec4 fragColor;
out vec4 fragNormal;
out vec4 fragPosWorld;

void main()
{
    // Send vertex attributes to fragment shader
    mat4 modelView = view * model;
    mat4 modelViewProj = projection * modelView;

    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragNormal = modelView * vec4(vertexNormal, 0.0);
    fragPosWorld = modelView * vec4(vertexPosition, 1.0);
    // Calculate final vertex position
    gl_Position = modelViewProj*vec4(vertexPosition, 1.0);
}