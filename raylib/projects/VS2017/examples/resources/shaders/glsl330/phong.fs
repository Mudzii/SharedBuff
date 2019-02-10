#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;
in vec4 fragNormal;
in vec4 fragPosWorld;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// in world space
uniform vec3 lightPos;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

void main()
{
    vec4 ambient = vec4(0.1,0.1,0.1,0.0);

    float diffuseCoeff = max(0, dot(normalize((view*vec4(lightPos,1.0)).xyz - fragPosWorld.xyz), normalize(fragNormal.xyz)));
    vec4 texelColor = vec4((texture(texture0, fragTexCoord) * colDiffuse).xyz, 1.0f);
    finalColor = texelColor * diffuseCoeff + texelColor * ambient;
    finalColor = vec4(finalColor.xyz, 1.0f) * fragColor;
}

