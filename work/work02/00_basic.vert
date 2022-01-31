/*

00_basic.vert : basic Vertex shader

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2020/2021
Master degree in Computer Science
Universita' degli Studi di Milano

*/


#version 410 core

// vertex position in world coordinates
// the number used for the location in the layout qualifier is the position of the vertex attribute
// as defined in the Mesh class
layout (location = 0) in vec3 position;

void main()
{
    // we assign the original vertex position in output
    gl_Position = vec4(position, 1.0f);
}
