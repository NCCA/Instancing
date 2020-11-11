#include <QMouseEvent>
#include <QGuiApplication>

#include "NGLScene.h"
#include <ngl/Transformation.h>
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>
#include <ngl/Random.h>

constexpr size_t c_numTrees=50000;

NGLScene::NGLScene()
{

  setTitle("Instancing Meshes");
  m_fpsTimer =startTimer(0);
  m_fps=0;
  m_frames=0;
  m_timer.start();

}


void NGLScene::createTransformTBO()
{
  // create a texture buffer to store the position and scale as a mat4 for each tree
  GLuint tbo;
  glGenBuffers(1,&tbo);
  // resize vector
  std::vector<ngl::Mat4> transforms(c_numTrees);
  ngl::Vec3 tx;
  ngl::Mat4 pos;
  ngl::Mat4 scale;
  // set random position and scale for each matrix
  for(auto &t : transforms)
  {
    tx=ngl::Random::getRandomVec3()*540;
    auto yScale=ngl::Random::randomPositiveNumber(2.0f)+0.5f;
    pos.translate(tx.m_x,0.0,tx.m_z);
    scale.scale(yScale,yScale,yScale);
    t=pos*scale;
  }
  // bind and fill TBO
  glBindBuffer(GL_TEXTURE_BUFFER, tbo);
  glBufferData(GL_TEXTURE_BUFFER, transforms.size()*sizeof(ngl::Mat4), &transforms[0].m_00, GL_STATIC_DRAW);
  // attatch to texture ( Texture unit 0 in this case as using not others)
  glGenTextures(1, &m_tboID);
  glActiveTexture( GL_TEXTURE0 );
  glBindTexture(GL_TEXTURE_BUFFER,m_tboID);
  // Note GL_RGBA32F as using Mat4 -> 4* vec4 in size
  glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, tbo);

}


NGLScene::~NGLScene()
{
  std::cout<<"Shutting down NGL, removing VAO's and Shaders\n";
}


void NGLScene::resizeGL( int _w, int _h )
{
  m_project=ngl::perspective( 45.0f, static_cast<float>( _w ) / _h, 0.05f,1350.0f );
  m_win.width  = static_cast<int>( _w * devicePixelRatio() );
  m_win.height = static_cast<int>( _h * devicePixelRatio() );
}

void NGLScene::initializeGL()
{
  // we must call this first before any other GL commands to load and link the
  // gl commands from the lib, if this is not done program will crash
  ngl::NGLInit::initialize();

  glClearColor(0.4f, 0.4f, 0.4f, 1.0f);			   // Grey Background
  // enable depth testing for drawing
  glEnable(GL_DEPTH_TEST);
  // enable multisampling for smoother drawing
  glEnable(GL_MULTISAMPLE);
  // Now we will create a basic Camera from the graphics library
  // This is a static camera so it only needs to be set once
  // First create Values for the camera position
  ngl::Vec3 from(0,100,280);
  ngl::Vec3 to(0,10,0);
  ngl::Vec3 up(0,1,0);


  // first we create a mesh from an obj passing in the obj file and texture
  m_mesh=std::make_unique<ngl::Obj>("models/tree.obj");
  m_mesh->createVAO();

  m_view=ngl::lookAt(from,to,up);
  // set the shape using FOV 45 Aspect Ratio based on Width and Height
  // The final two are near and far clipping planes of 0.5 and 10
  m_project=ngl::perspective(45.0f,720.0f/576.0f,0.05f,1350.0f);
  // we are creating a shader called PerFragADS
  ngl::ShaderLib::createShaderProgram("PerFragADS");
  // now we are going to create empty shaders for Frag and Vert
  ngl::ShaderLib::attachShader("PerFragADSVertex",ngl::ShaderType::VERTEX);
  ngl::ShaderLib::attachShader("PerFragADSFragment",ngl::ShaderType::FRAGMENT);
  // attach the source
  ngl::ShaderLib::loadShaderSource("PerFragADSVertex","shaders/PerFragASDVert.glsl");
  ngl::ShaderLib::loadShaderSource("PerFragADSFragment","shaders/PerFragASDFrag.glsl");
  // compile the shaders
  ngl::ShaderLib::compileShader("PerFragADSVertex");
  ngl::ShaderLib::compileShader("PerFragADSFragment");
  // add them to the program
  ngl::ShaderLib::attachShaderToProgram("PerFragADS","PerFragADSVertex");
  ngl::ShaderLib::attachShaderToProgram("PerFragADS","PerFragADSFragment");

  // now we have associated this data we can link the shader
  ngl::ShaderLib::linkProgramObject("PerFragADS");
  // and make it active ready to load values
  ngl::ShaderLib::use("PerFragADS");

  glEnable(GL_DEPTH_TEST); // for removal of hidden surfaces

  createTransformTBO();
  // load a texture into texture Unit 1
  ngl::Texture t("models/ratGrid.png");
  t.setMultiTexture(1);
  m_textureID=t.setTextureGL();
  ngl::ShaderLib::setUniform("tex",1);
  ngl::ShaderLib::setUniform("TBO",0);

  m_text=std::make_unique<ngl::Text>("fonts/Arial.ttf",16);
  m_text->setScreenSize(width(),height());

}


void NGLScene::loadMatricesToShader()
{
  ngl::ShaderLib::use("PerFragADS");
  // loading this to shader each frame as it is the mouse that changes
  ngl::ShaderLib::setUniform("mouseTX",m_mouseGlobalTX);
  ngl::ShaderLib::setUniform("VP",m_project*m_view);
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
   ++m_frames;

  // draw the mesh
  m_mesh->bindVAO();
  loadMatricesToShader();

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_BUFFER, m_tboID);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D,m_textureID);

  glDrawArraysInstanced(GL_TRIANGLES,0,m_mesh->getMeshSize(),c_numTrees);
  m_mesh->unbindVAO();

  m_text->setColour(1,1,0);
  m_text->renderText(10,700,fmt::format("{} instances {} fps",c_numTrees,m_fps));

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

void NGLScene::timerEvent( QTimerEvent *_event )
{
  if(_event->timerId() == m_fpsTimer)
    {
      if( m_timer.elapsed() > 1000.0)
      {
        m_fps=m_frames;
        m_frames=0;
        m_timer.restart();
      }
     }
      // re-draw GL
  update();
}



