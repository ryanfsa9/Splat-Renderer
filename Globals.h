#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Math.h"

//In Main.cpp

//Helper Functions
void Error(const char* file, int line, const char* msg);
void HRError(long hr, const char* file, int line);
void TextBox(const char* msg);
bool keyDown(char key);
#define ASSERT(b) if(!(b)) { Error(__FILE__, __LINE__, ""); }
#define ASSERTMSG(b, m) if(!(b)) { Error(__FILE__, __LINE__, m); }
#define HR(b) if(b!=S_OK) HRError(b, __FILE__, __LINE__);

struct Window {
	HWND hWnd;
	Int2 size;
} extern window;

struct Camera {
	Float3 pos;
	float yaw, pitch;

	constexpr static float nearZ = 0.1f;
	constexpr static float farZ = 1000.0f;
	constexpr static float fov_y = 60.0f * PI / 180.0f;

	Matrix4 View();
	Matrix4 Proj();
} extern camera;

//In Graphics.cpp

namespace Graphics {
	void InitGlobals();
	void Resize();
	void CleanGlobals();
}

namespace GS {
	void Init(const char* ply);
	void Render();
	void Clean();
}

namespace Mesh {
	void Init(const char* obj);
	void Render();
	void Clean();
}