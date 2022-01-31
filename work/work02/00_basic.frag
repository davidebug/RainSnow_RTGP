/*

00_basic.frag : (VERY) basic Fragment shader

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2020/2021
Master degree in Computer Science
Universita' degli Studi di Milano

*/

#version 410 core

// output variable for the fragment shader. Usually, it is the final color of the fragment
out vec4 color;

void main()
{
    color = vec4(1.0,0.0,0.0,1.0);
}
