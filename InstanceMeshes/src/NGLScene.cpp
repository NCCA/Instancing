#include <QMouseEvent>
#include <QGuiApplication>

#include "NGLScene.h"
#include <ngl/Camera.h>
#include <ngl/Light.h>
#include <ngl/Transformation.h>
#include <ngl/Material.h>
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>
#include <ngl/Random.h>

constexpr size_t c_numTrees=15000;

NGLScene::NGLScene()
{

  setTitle("Morph Mesh Demo");
}


void NGLScene::createTransformTBO()
{

  GLuint morphTarget;
  glGenBuffers(1,&morphTarget);
  std::vector<ngl::Mat4> transforms(c_numTrees);
  ngl::Random *rng=ngl::Random::instance();
  ngl::Vec3 tx;
  ngl::Mat4 pos;
  ngl::Mat4 scale;
  for(auto &t : transforms)
  {
    tx=rng->getRandomVec3()*140;
    auto yScale=rng->randomPositiveNumber(2.0f)+0.5f;
    pos.translate(tx.m_x,0.0,tx.m_z);
    scale.scale(1.0f,yScale,1.0f);
    t=scale*pos;
  }
  glBindBuffer(GL_TEXTURE_BUFFER, morphTarget);
  //ngl::NGLCheckGLError("bind texture",__LINE__);
  glBufferData(GL_TEXTURE_BUFFER, transforms.size()*sizeof(ngl::Mat4), &transforms[0].m_00, GL_STATIC_DRAW);

  glGenTextures(1, &m_tboID);
  glActiveTexture( GL_TEXTURE0 );
  glBindTexture(GL_TEXTURE_BUFFER,m_tboID);

  glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, morphTarget);

}


NGLScene::~NGLScene()
{
  std::cout<<"Shutting down NGL, removing VAO's and Shaders\n";
}


void NGLScene::resizeGL( int _w, int _h )
{
  m_cam.setShape( 45.0f, static_cast<float>( _w ) / _h, 0.05f, 350.0f );
  m_win.width  = static_cast<int>( _w * devicePixelRatio() );
  m_win.height = static_cast<int>( _h * devicePixelRatio() );
}

void NGLScene::initializeGL()
{
  // we must call this first before any other GL commands to load and link the
  // gl commands from the lib, if this is not done program will crash
  ngl::NGLInit::instance();

  glClearColor(0.4f, 0.4f, 0.4f, 1.0f);			   // Grey Background
  // enable depth testing for drawing
  glEnable(GL_DEPTH_TEST);
  // enable multisampling for smoother drawing
  glEnable(GL_MULTISAMPLE);
  // Now we will create a basic Camera from the graphics library
  // This is a static camera so it only needs to be set once
  // First create Values for the camera position
  ngl::Vec3 from(0,100,180);
  ngl::Vec3 to(0,10,0);
  ngl::Vec3 up(0,1,0);


  // first we create a mesh from an obj passing in the obj file and texture
  m_mesh.reset( new ngl::Obj("models/tree.obj"));
  m_mesh->createVAO();

  m_cam.set(from,to,up);
  // set the shape using FOV 45 Aspect Ratio based on Width and Height
  // The final two are near and far clipping planes of 0.5 and 10
  m_cam.setShape(45,(float)720.0/576.0,0.05,350);
  // now to load the shader and set the values
  // grab an instance of shader manager
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  // we are creating a shader called PerFragADS
  shader->createShaderProgram("PerFragADS");
  // now we are going to create empty shaders for Frag and Vert
  shader->attachShader("PerFragADSVertex",ngl::ShaderType::VERTEX);
  shader->attachShader("PerFragADSFragment",ngl::ShaderType::FRAGMENT);
  // attach the source
  shader->loadShaderSource("PerFragADSVertex","shaders/PerFragASDVert.glsl");
  shader->loadShaderSource("PerFragADSFragment","shaders/PerFragASDFrag.glsl");
  // compile the shaders
  shader->compileShader("PerFragADSVertex");
  shader->compileShader("PerFragADSFragment");
  // add them to the program
  shader->attachShaderToProgram("PerFragADS","PerFragADSVertex");
  shader->attachShaderToProgram("PerFragADS","PerFragADSFragment");

  // now we have associated this data we can link the shader
  shader->linkProgramObject("PerFragADS");
  // and make it active ready to load values
  (*shader)["PerFragADS"]->use();
  // now we need to set the material and light values
  /*
   *struct MaterialInfo
   {
        // Ambient reflectivity
        vec3 Ka;
        // Diffuse reflectivity
        vec3 Kd;
        // Specular reflectivity
        vec3 Ks;
        // Specular shininess factor
        float shininess;
  };*/
  shader->setShaderParam3f("material.Ka",0.1,0.1,0.1);
  // red diffuse
  shader->setShaderParam3f("material.Kd",0.8,0.8,0.8);
  // white spec
  shader->setShaderParam3f("material.Ks",1.0,1.0,1.0);
  shader->setShaderParam1f("material.shininess",1000);
  // now for  the lights values (all set to white)
  /*struct LightInfo
  {
  // Light position in eye coords.
  vec4 position;
  // Ambient light intensity
  vec3 La;
  // Diffuse light intensity
  vec3 Ld;
  // Specular light intensity
  vec3 Ls;
  };*/
  shader->setUniform("light.position",ngl::Vec3(2,20,2));
  shader->setShaderParam3f("light.La",0.1,0.1,0.1);
  shader->setShaderParam3f("light.Ld",1.0,1.0,1.0);
  shader->setShaderParam3f("light.Ls",0.9,0.9,0.9);

  glEnable(GL_DEPTH_TEST); // for removal of hidden surfaces

  // as re-size is not explicitly called we need to do this.
  glViewport(0,0,width(),height());
  m_text.reset( new ngl::Text(QFont("Arial",16)));
  m_text->setScreenSize(width(),height());
  createTransformTBO();

}


void NGLScene::loadMatricesToShader()
{
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  (*shader)["PerFragADS"]->use();
  ngl::Mat4 MV;
  ngl::Mat4 MVP;
  ngl::Mat3 normalMatrix;
  MV= m_mouseGlobalTX*m_cam.getViewMatrix();
  MVP=MV*m_cam.getProjectionMatrix() ;
  normalMatrix=MV;
  normalMatrix.inverse();
  shader->setUniform("mouseTX",m_mouseGlobalTX);
  shader->setUniform("VP",m_cam.getVPMatrix());
}

void NGLScene::paintGL()
{
  // clear the screen and depth buffer
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glViewport(0,0,m_win.width,m_win.height);
   // Rotation based on the mouse position for our global transform
   ngl::Mat4 rotX;
   ngl::Mat4 rotY;
   // create the rotation matrices
   rotX.rotateX(m_win.spinXFace);
   rotY.rotateY(m_win.spinYFace);
   // multiply the rotations
   m_mouseGlobalTX=rotY*rotX;
   // add the translations
   m_mouseGlobalTX.m_m[3][0] = m_modelPos.m_x;
   m_mouseGlobalTX.m_m[3][1] = m_modelPos.m_y;
   m_mouseGlobalTX.m_m[3][2] = m_modelPos.m_z;

  loadMatricesToShader();
  // draw the mesh
  m_mesh->bindVAO();
  glBindTexture(GL_TEXTURE_BUFFER, m_tboID);
  //m_meshes[0]->draw();
 glDrawArraysInstanced(GL_TRIANGLES,0,m_mesh->getMeshSize(),c_numTrees);
//  glDrawArrays(GL_TRIANGLES,0,m_meshes[0]->getMeshSize());

  m_mesh->unbindVAO();
}

//----------------------------------------------------------------------------------------------------------------------

void NGLScene::keyPressEvent(QKeyEvent *_event)
{
  // this method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  switch (_event->key())
  {
  // escape key to quite
  case Qt::Key_Escape : QGuiApplication::exit(EXIT_SUCCESS); break;
  // show full screen
  case Qt::Key_F : showFullScreen(); break;
  // show windowed
  case Qt::Key_N : showNormal(); break;

  case Qt::Key_W : glPolygonMode(GL_FRONT_AND_BACK,GL_LINE); break;
  case Qt::Key_S : glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); break;

  default : break;
  }
  // finally update the GLWindow and re-draw
    update();
}



