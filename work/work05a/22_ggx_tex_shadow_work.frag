/*

22_ggx_tex_shadow_work.frag:  initial code for shadow rendering using shadow map

N.B. 1)  "21_ggx_tex_shadow_work.vert" must be used as vertex shader

N.B. 2) the shader considers only a directional light (simpler to manage for the creation of the shadow map). For more lights, of different kind, the shader must be modified to consider each case

N.B. 3)  the different effects are implemented using Shaders Subroutines

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2020/2021
Master degree in Computer Science
Universita' degli Studi di Milano

*/

#version 410 core

const float PI = 3.14159265359;

// output shader variable
out vec4 colorFrag;

// light incidence directions (calculated in vertex shader, interpolated by rasterization)
in vec3 lightDir;
// the transformed normal has been calculated per-vertex in the vertex shader
in vec3 vNormal;
// vector from fragment to camera (in view coordinate)
in vec3 vViewPosition;

// interpolated texture coordinates
in vec2 interp_UV;

// texture repetitions
uniform float repeat;

// texture sampler
uniform sampler2D tex;

uniform float alpha; // rugosity - 0 : smooth, 1: rough
uniform float F0; // fresnel reflectance at normal incidence
uniform float Kd; // weight of diffuse reflection

////////////////////////////////////////////////////////////////////

// the "type" of the Subroutine
//subroutine float shadow_map();

// Subroutine Uniform (it is conceptually similar to a C pointer function)
//subroutine uniform shadow_map Shadow_Calculation;

////////////////////////////////////////////////////////////////////


//////////////////////////////////////////
// Schlick-GGX method for geometry obstruction (used by GGX model)
float G1(float angle, float alpha)
{
    // in case of Image Based Lighting, the k factor is different:
    // usually it is set as k=(alpha*alpha)/2
    float r = (alpha + 1.0);
    float k = (r*r) / 8.0;

    float num   = angle;
    float denom = angle * (1.0 - k) + k;

    return num / denom;
}

///////////// MAIN ////////////////////////////////////////////////
void main()
{
    // we repeat the UVs and we sample the texture
    vec2 repeated_Uv = mod(interp_UV*repeat, 1.0);
    vec4 surfaceColor = texture(tex, repeated_Uv);

    // normalization of the per-fragment normal
    vec3 N = normalize(vNormal);
    // normalization of the per-fragment light incidence direction
    vec3 L = normalize(lightDir.xyz);

    // cosine angle between direction of light and normal
    float NdotL = max(dot(N, L), 0.0);

    // diffusive (Lambert) reflection component
    vec3 lambert = (Kd*surfaceColor.rgb)/PI;

    // we initialize the specular component
    vec3 specular = vec3(0.0);

    // if the cosine of the angle between direction of light and normal is positive, then I can calculate the specular component
    if(NdotL > 0.0)
    {
        // the view vector has been calculated in the vertex shader, already negated to have direction from the mesh to the camera
        vec3 V = normalize( vViewPosition );

        // half vector
        vec3 H = normalize(L + V);

        // we implement the components seen in the slides for a PBR BRDF
        // we calculate the cosines and parameters to be used in the different components
        float NdotH = max(dot(N, H), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float VdotH = max(dot(V, H), 0.0);
        float alpha_Squared = alpha * alpha;
        float NdotH_Squared = NdotH * NdotH;

        // Geometric factor G2
        float G2 = G1(NdotV, alpha)*G1(NdotL, alpha);

        // Rugosity D
        // GGX Distribution
        float D = alpha_Squared;
        float denom = (NdotH_Squared*(alpha_Squared-1.0)+1.0);
        D /= PI*denom*denom;

        // Fresnel reflectance F (approx Schlick)
        vec3 F = vec3(pow(1.0 - VdotH, 5.0));
        F *= (1.0 - F0);
        F += F0;

        // we put everything together for the specular component
        specular = (F * G2 * D) / (4.0 * NdotV * NdotL);

    }

    // the rendering equation is:
    //integral of: BRDF * Li * (cosine angle between N and L)
    // BRDF in our case is: the sum of Lambert and GGX
    // Li is considered as equal to 1: light is white, and we have not applied attenuation. With colored lights, and with attenuation, the code must be modified and the Li factor must be multiplied to finalColor
    vec3 finalColor = (lambert + specular)*NdotL;


    colorFrag = vec4(finalColor, 1.0);
}
