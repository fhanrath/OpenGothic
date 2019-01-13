#include "mainwindow.h"

#include <Tempest/Except>
#include <Tempest/Painter>

#include <Tempest/Brush>
#include <Tempest/Pen>
#include <Tempest/Layout>

#include "ui/gamemenu.h"
#include "ui/menuroot.h"

#include "gothic.h"

using namespace Tempest;

MainWindow::MainWindow(Gothic &gothic, Tempest::VulkanApi& api)
  :Window(Maximized),device(api,hwnd()),atlas(device),resources(gothic,device),draw(device),gothic(gothic) {
  for(uint8_t i=0;i<device.maxFramesInFlight();++i){
    fLocal.emplace_back(device);
    commandBuffersSemaphores.emplace_back(device);
    }

  initSwapchain();
  setupUi();

  background = Resources::loadTexture("STARTSCREEN.TGA");
  }

MainWindow::~MainWindow() {
  removeAllWidgets();
  // unload
  gothic.setWorld(World());
  }

void MainWindow::setupUi() {
  setLayout(Horizontal);
  rootMenu = &addWidget(new MenuRoot(gothic));
  rootMenu->setMenu(new GameMenu(*rootMenu,gothic,"MENU_MAIN"));
  }

void MainWindow::paintEvent(PaintEvent& event) { 
  if(!gothic.world().isEmpty())
    return;

  Painter p(event);

  p.setBrush(Color(0.0));
  p.drawRect(0,0,w(),h());

  p.setBrush(background);
  p.drawRect((w()-background.w())/2,(h()-background.h())/2,
             background.w(),background.h(),
             0,0,background.w(),background.h());
  }

void MainWindow::resizeEvent(SizeEvent&) {
  device.reset();
  initSwapchain();
  }

void MainWindow::mouseMoveEvent(MouseEvent &event) {
  event.accept();
  }

void MainWindow::initSwapchain(){
  const size_t imgC=device.swapchainImageCount();
  commandDynamic.clear();
  fbo.clear();

  mainPass=device.pass(Color(0.f),FboMode::Discard);

  for(size_t i=0;i<imgC;++i) {
    Tempest::Frame frame=device.frame(i);
    fbo.emplace_back(device.frameBuffer(frame,mainPass));
    commandDynamic.emplace_back(device.commandBuffer());
    }
  }

void MainWindow::render(){
  try {
    auto& context=fLocal[device.frameId()];
    Semaphore&     renderDone=commandBuffersSemaphores[device.frameId()];
    CommandBuffer& cmd       =commandDynamic[device.frameId()];

    context.gpuLock.wait();

    const  uint32_t imgId=device.nextImage(context.imageAvailable);

    if(needToUpdate())
      dispatchPaintEvent(surface,atlas);

    cmd.begin();
    cmd.beginRenderPass(fbo[imgId],mainPass,w(),h());

    draw.draw(cmd,*this,gothic);
    surface.draw(device,cmd,mainPass);

    cmd.endRenderPass();
    cmd.end();

    device.draw(cmd,context.imageAvailable,renderDone,context.gpuLock);

    device.present(imgId,renderDone);
    }
  catch(const Tempest::DeviceLostException&) {
    device.reset();
    initSwapchain();
    }
  }
