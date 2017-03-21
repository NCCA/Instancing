#version 410 core
layout (location =0) in vec3 inVert;
layout (location = 2) in vec3 inNormal;
layout (location = 1) in vec2 inUV;

uniform samplerBuffer TBO;
uniform mat4 mouseTX;
uniform mat4 VP;
out vec4 colour;
void main()
{


  vec4 col1=texelFetch(TBO,gl_InstanceID*4+0);
  vec4 col2=texelFetch(TBO,gl_InstanceID*4+1);
  vec4 col3=texelFetch(TBO,gl_InstanceID*4+2);
  vec4 col4=texelFetch(TBO,gl_InstanceID*4+3);
  colour=col2;
  mat4 tx=mat4(col1,col2,col3,col4);
  // P*V*mouse*tx;
  gl_Position=VP*mouseTX*tx*vec4(inVert,1.0);
}









