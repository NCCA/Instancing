#version 410 core
layout (location =0)out vec4 fragColour;
in vec4 colour;
in vec2 vertUV;
uniform sampler2D tex;

void main ()
{
  fragColour=texture(tex, vertUV.st);
}


