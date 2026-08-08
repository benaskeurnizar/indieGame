#ifndef UNICODE
#define UNICODE
#endif 

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include <glad/glad.h>
#include <Gl/gl.h>
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
//#include "Game.h"

#define IDT_TIMER1 1
#define IDT_TIMER2 2

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

//#include "win32_sound.cpp"
#include "game_code.cpp"

LARGE_INTEGER PerformanceFrequency;
float TargetSecondsElapsed;
double elapsed_time = 0.0;

static int Gwidth;
static int Gheight;

Engine* GEngine = NULL;


int running = 1;
int first = 1;
int first_water = 1;

void opnGlRender(HWND window){
    RECT ClientRect;
    GetClientRect(window,&ClientRect);
    int width = ClientRect.right - ClientRect.left;
    int height = ClientRect.bottom - ClientRect.top;
    glViewport(0,0,width,height);
    HDC windowDC = GetDC(window);
    SwapBuffers(windowDC);
    ReleaseDC(window,windowDC);
}
void win32_initOpenGl(HWND window){
    HDC windowDC = GetDC(window);
    PIXELFORMATDESCRIPTOR pfd = 
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    // Flags
        PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
        32,                   // Colordepth of the framebuffer.
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,                   // Number of bits for the depthbuffer
        8,                    // Number of bits for the stencilbuffer
        0,                    // Number of Aux buffers in the framebuffer.
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };
    int PixelFormat = ChoosePixelFormat(windowDC,&pfd);
    if (PixelFormat == 0){
        return;
    }
    SetPixelFormat(windowDC,PixelFormat,&pfd);
    HGLRC GlHandle = wglCreateContext(windowDC);
    if(wglMakeCurrent(windowDC,GlHandle)){
        printf("successfully make gl current! \n");
    }else{
        printf("failed to make gl current! \n");
    }
    printf("Loaded OpenGL %d.%d\n", GLVersion.major, GLVersion.minor);
    ReleaseDC(window,windowDC);
}

void UpdateBuffer(BitMap* buffer){
    uint32_t *bufferP = (uint32_t*) buffer->Memory;
    for(int y=0;y<buffer->height;y++){
        for(int x=0;x<buffer->width;x++){
            bufferP[x] = 0x00000000;
        }
        bufferP += buffer->width;
    }
}

void ResizeDib(BitMap* buffer,int width,int height){
    int BytesPerPixel = 4;
    if(buffer->Memory){
        VirtualFree(buffer->Memory,0,MEM_RELEASE);
    }
    buffer->width = width;
    buffer->height = height;
    buffer->BitMapInfo.bmiHeader.biSize = sizeof(buffer->BitMapInfo.bmiHeader) ;
    buffer->BitMapInfo.bmiHeader.biWidth = width ;
    buffer->BitMapInfo.bmiHeader.biHeight = -height ;
    buffer->BitMapInfo.bmiHeader.biPlanes = 1 ;
    buffer->BitMapInfo.bmiHeader.biBitCount =BytesPerPixel * 8 ;
    buffer->BitMapInfo.bmiHeader.biCompression = BI_RGB ;
    buffer->BitMapInfo.bmiHeader.biSizeImage = 0 ;
    buffer->Memory = VirtualAlloc(0,BytesPerPixel*width*height,MEM_COMMIT,PAGE_READWRITE);
    memset(buffer->Memory,0x00,BytesPerPixel*width*height);
    
}

void UpdateWindow(BitMap* buffer,HDC hdc,int W,int H){
    int Draw = StretchDIBits(
                             hdc,
                             0,
                             0,
                             buffer->width,
                             buffer->height,
                             0,
                             0,
                             W,
                             H,
                             buffer->Memory,
                             &buffer->BitMapInfo,
                             DIB_RGB_COLORS,
                             SRCCOPY
                             );
    
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
  switch(uMsg){
  case WM_CREATE:
    {
      HDC hdc = GetDC(hwnd);
      RECT ClientRect;
      GetClientRect(hwnd,&ClientRect);
      int width = ClientRect.right - ClientRect.left;
      int height = ClientRect.bottom - ClientRect.top;
      Gwidth = width;
      Gheight = height;
      win32_initOpenGl(hwnd);
      if (!gladLoadGL()) {
	printf("Failed to initialize GLAD\n");
	return -1;
      }
      printf("Loaded OpenGL %d.%d\n", GLVersion.major, GLVersion.minor);
      printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
      glEnable(GL_DEPTH_TEST);
      glEnable(GL_TEXTURE_2D);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
      SetTimer(hwnd, IDT_TIMER1, 33, NULL);
      ReleaseDC(hwnd,hdc);
    }break;
  case WM_SIZE:
    {
      HDC hdc = GetDC(hwnd);
      RECT ClientRect;
      GetClientRect(hwnd,&ClientRect);
      int width = ClientRect.right - ClientRect.left;
      int height = ClientRect.bottom - ClientRect.top;
      if(GEngine){
	GEngine->renderer.screenWidth = width;
	GEngine->renderer.screenHeight = height;
      }
      Gwidth = width;
      Gheight = height;
      ReleaseDC(hwnd,hdc);
    }break;
  case WM_PAINT:
    {
      HDC hdc = GetDC(hwnd);
      RECT ClientRect;
      GetClientRect(hwnd,&ClientRect);
      int width = ClientRect.right - ClientRect.left;
      int height = ClientRect.bottom - ClientRect.top;
      ReleaseDC(hwnd,hdc);
    }break;
  case WM_LBUTTONDOWN:
    {
      GEngine->input.mouseLeftHeld = 1;
    }break;
  case WM_LBUTTONUP:
    {
      GEngine->input.mouseLeftHeld = 0;
      GEngine->input.firstMousePress = 1;
    }break;
  case WM_RBUTTONDOWN:
    {
      GEngine->input.mouseRightHeld = 1;
    }break;
  case WM_RBUTTONUP:
    {
      GEngine->input.mouseRightHeld = 0;
    }break;
  case WM_MOUSEMOVE:
    {
      if(GEngine){
	float sensitivity = 0.05;
	if(GEngine->input.mouseLeftHeld){
	  if(GEngine->input.firstMousePress){
	    GEngine->input.mouseX = GET_X_LPARAM(lParam);
	    GEngine->input.mouseY = GET_Y_LPARAM(lParam);
	    GEngine->input.firstMousePress = 0;
	  }else{
	    int lastX = GET_X_LPARAM(lParam);
	    int lastY = GET_Y_LPARAM(lParam);
	    int DX = GEngine->input.mouseX - lastX;
	    int DY = -(GEngine->input.mouseY - lastY);
	    GEngine->input.mouseX = lastX ;
	    GEngine->input.mouseY = lastY ;
	    GEngine->input.DYaw = DX * sensitivity;
	    GEngine->input.DPitch = DY * sensitivity;
	  }
	}
      }
    }break;
  case WM_KEYDOWN:
    {
      if(wParam==0x57){ // W
        GEngine->input.keyW = 1;
      }
      if(wParam==0x53){ //s
	GEngine->input.keyS = 1;
      }
      if(wParam==0x41){//A
	GEngine->input.keyA = 1;
      }
      if(wParam==0x44){//D
	GEngine->input.keyD = 1;
      }
      if(wParam==VK_SPACE){
	if(!GEngine){
	  break;
	}
	/*if(GEngine->debug_mode){
	  GEngine->debug_mode = 0;
	}else{
	  GEngine->debug_mode = 1;
	  initDebugCamera(&(GEngine->world));
	  }*/
	GEngine->input.SpacePressed = 1;
      }
    }break;
  case WM_KEYUP:
    {
      if(wParam==0x57){ // W
        GEngine->input.keyW = 0;
      }
      if(wParam==0x53){ //s
	GEngine->input.keyS = 0;
      }
      if(wParam==0x41){//A
	GEngine->input.keyA = 0;
      }
      if(wParam==0x44){//D
	GEngine->input.keyD = 0;
      }
    }break;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
        
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LARGE_INTEGER win32_getWallClock(){
  LARGE_INTEGER result;
  QueryPerformanceCounter(&result);
  return result;
}

float win32_getSecondsElapsed(LARGE_INTEGER start,LARGE_INTEGER end){
  float result =  ((float)(end.QuadPart -start.QuadPart)) / (float)PerformanceFrequency.QuadPart;
  return result;
}

int WINAPI wWinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,PWSTR pCmdLine,int nCmdShow){
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  
  const wchar_t CLASS_NAME[] = L"Game";
  WNDCLASSW wc = {0};
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = CLASS_NAME;
  int width,height;
  //MMRESULT timeBeginPriod(1);
  RegisterClassW(&wc);
  HWND hwnd = CreateWindowExW(
			      0,                              // Optional window styles.
			      CLASS_NAME,                     // Window class
			      L"quake",    // Window text
			      WS_OVERLAPPEDWINDOW,            // Window style
                                
			      // Size and position
			      CW_USEDEFAULT, CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,
                                
			      NULL,       // Parent window    
			      NULL,       // Menu
			      hInstance,  // Instance handle
			      NULL        // Additional application data
			      );
    
  if (hwnd == NULL)
    {
      return 0;
    }
  QueryPerformanceFrequency(&PerformanceFrequency);
  ShowWindow(hwnd, nCmdShow);
  LARGE_INTEGER lastCounter;
  QueryPerformanceCounter(&lastCounter);
  while(running){
    MSG message;
    while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
      if (message.message == WM_QUIT) {
	running = 0;
      } else {
	TranslateMessage(&message);
	DispatchMessage(&message);
      }
    }
    if(first){
      GameInit(Gwidth,Gheight,&GEngine);
      TargetSecondsElapsed = 1.0f / GEngine->refresh_rate_hz;
      //NOTE: Now we have the engine structure (hopefully!)
      first = 0;
	
    }else{
      GameUpdate(GEngine,elapsed_time);
      GameRender(GEngine);
      opnGlRender(hwnd);
    }
    LARGE_INTEGER WorkCounter = win32_getWallClock();
    float workSecondsElapsed = win32_getSecondsElapsed(lastCounter, WorkCounter);
    if(workSecondsElapsed < TargetSecondsElapsed){
      while(workSecondsElapsed < TargetSecondsElapsed){
	DWORD sleepMs = (DWORD)(1000.0 * (TargetSecondsElapsed - workSecondsElapsed));
	//Sleep(sleepMs);
	WorkCounter = win32_getWallClock();
	workSecondsElapsed = win32_getSecondsElapsed(lastCounter,WorkCounter);
      }
    }else{
      //TODO: missed frame!
    }
    LARGE_INTEGER endCounter;
    QueryPerformanceCounter(&endCounter);
    int counterElapsed = (int)(endCounter.QuadPart - lastCounter.QuadPart);
    int MsPerFrame = (int)(win32_getSecondsElapsed(lastCounter,endCounter) * 1000.0);
    int FPS = PerformanceFrequency.QuadPart/counterElapsed;
    char Buffer[256];
    wsprintfA(Buffer," %dms/f, %dfps \n",MsPerFrame,FPS);
    if(GEngine){
      snprintf(GEngine->renderer.fps_text, 64,"FPS : %d\n",FPS);
    }
    OutputDebugStringA(Buffer);
    lastCounter = endCounter;
    elapsed_time += 0.0016;
  }
  return 0;
}
