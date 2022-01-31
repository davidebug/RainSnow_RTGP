/*
11_illumination_models_ML.vert: as 09_illumination_models.vert, but with multiple lights

N.B. 1) In this example, we consider point lights only. For different kind of lights, the computation must be changed (for example, a directional light is defined by the direction of incident light, so the lightDir is passed as uniform and not calculated in the shader like in this case with a point light).

N.B. 2) 
There are other methods (more efficient) to pass multiple data to the shaders, using for example Uniform Buffer Objects.
With last versions of OpenGL, using structures like the one cited above, it is possible to pass a "dynamic" number of lights
https://www.geeks3d.com/20140704/gpu-buffers-introduction-to-opengl-3-1-uniform-buffers-objects/
https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL (scroll down a bit)
https://hub.packtpub.com/opengl-40-using-uniform-blocks-and-uniform-buffer-objects/

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2020/2021
Master degree in Computer Science
Universita' degli Studi di Milano

*/

#version 410 core

// number of lights in the scene
#define NR_LIGHTS 3

// vertex position in world coordinates
layout (location = 0) in vec3 position;
// vertex normal in world coordinate
layout (location = 1) in vec3 normal;
// the numbers used for the location in the layout qualifier are the positions of the vertex attribute
// as defined in the Mesh class

// vectors of lights positions (passed from the application)
uniform vec3 lights[NR_LIGHTS];

// model matrix
uniform mat4 modelMatrix;
// view matrix
uniform mat4 viewMatrix;
// Projection matrix
uniform mat4 projectionMatrix;

// normals transformation matrix (= transpose of the inverse of the model-view matrix)
uniform mat3 normalMatrix;

// array of light incidence directions (in view coordinate)
out vec3 lightDirs[NR_LIGHTS];

// the transformed normal (in view coordinate) is set as an output variable, to be "passed" to the fragment shader
// this means that the normal values in each vertex will be interpolated on each fragment created during rasterization between two vertices
out vec3 vNormal;

// in the fragment shader, we need to calculate also the reflection vector for each fragment
// to do this, we need to calculate in the vertex shader the view direction (in view coordinates) for each vertex, and to have it interpolated for each fragment by the rasterization stage
out vec3 vViewPosition;


void main(){

  // vertex position in ModelView coordinate (see the last line for the application of projection)
  // when I need to use coordinates in camera coordinates, I need to split the application of model and view transformations from the projection transformations
  vec4 mvPosition = viewMatrix * modelMatrix * vec4( position, 1.0 );
  
  // view direction, negated to have vector from the vertex to the camera
  vViewPosition = -mvPosition.xyz;

  // transformations are applied to the normal
  vNormal = normalize( normalMatrix * normal );

  // light incidence directions for all the lights (in view coordinate)
  for (int i=0;i<NR_LIGHTS;i++)
  {
    vec4 lightPos = viewMatrix  * vec4(lights[i], 1.0);;
    lightDirs[i] = lightPos.xyz - mvPosition.xyz;
  }

  // we apply the projection transformation
  gl_Position = projectionMatrix * mvPosition;

}
