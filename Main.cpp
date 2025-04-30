#include "Globals.h"
#include "resource.h"
#include "commdlg.h"

#include <string>
#include <sstream>
using namespace std;

Window window;
Camera camera;
HINSTANCE hInst;
void (*RenderCurrent)() = nullptr;
void (*CleanCurrent)() = nullptr;

//Helper Functions
void Error(const char* file, int line, const char* msg) {
		string body = "File: ";
		body.append(file);
		body.append("\nLine: ");
		body.append(std::to_string(line));
		body.append("\nMsg: ");
		body.append(msg);
		MessageBoxA(NULL, body.c_str(), "Error!", MB_ICONERROR);
		ExitProcess(-1); //bye bye
	}
void HRError(long hr, const char* file, int line) {
		wstringstream text;
		text << "WINDOWS/DIRECTX Error:";
		text << "\n[Location]\n" << file << " line " << line << "\n";

		TCHAR buffer[512];
		FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			hr,
			0,
			buffer,
			512,
			NULL
		);
		text << "\n[Description]\n" << buffer;

		text << "\n[HRESULT Code]\n" << hr;

		MessageBoxW(NULL, text.str().c_str(), L"ERROR:", MB_OK | MB_ICONERROR);
		ExitProcess(-1); //bye bye
	}
void TextBox(const char* msg) {
		MessageBoxA(NULL, msg, "Hello:", MB_ICONINFORMATION);
	}
bool KeyDown(char key) {
		return GetAsyncKeyState(key) >> 15;
	}
bool FileDialog(char* buf256, const char* filter) {
	OPENFILENAMEA ofn = {};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = window.hWnd;
	ofn.lpstrFile = buf256;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = 256;
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 0;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	return GetOpenFileNameA(&ofn);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_WINDOWPOSCHANGED:
	{
		RECT r;
		GetClientRect(hWnd, &r);
		Int2 newSize(r.right - r.left, r.bottom - r.top);

		//resize
		if (window.size != newSize) {
			window.size = newSize;
			Graphics::Resize();
		}
		return 0;
	}
	case WM_COMMAND:
	{	//menu buttons
		int wmId = LOWORD(wParam);
		switch (wmId)
		{
		case ID_FILE_OPENSPLAT:
			char buf[256];
			if (FileDialog(buf, ".PLY\0*.PLY\0")) {
				if (CleanCurrent) CleanCurrent();
				GS::Load(buf);
				RenderCurrent = GS::Render;
				CleanCurrent = GS::Clean;

				camera.pos = Float3(0, 0, 0);
				camera.yaw = 0;
				camera.pitch = 0;
			}
			break;
		case ID_FILE_MESH:
			if (CleanCurrent) CleanCurrent();
			Mesh::Extract();
			RenderCurrent = Mesh::Render;
			CleanCurrent = Mesh::Clean;
			break;
		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProcW(hWnd, msg, wParam, lParam);
	}
}

bool MessagePump() {
	MSG Msg;

	while (PeekMessageW(&Msg, NULL, 0, 0, PM_REMOVE)) {
		if (Msg.message == WM_QUIT) {
			return false;
		}
		TranslateMessage(&Msg);
		DispatchMessageW(&Msg);
	}
	return true;
}

//WinMain
int WINAPI WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE hPrevInstance, _In_ PSTR pCmdLine, _In_ int nCmdShow) {

	//Register WndClass
	WNDCLASSEXW wc;
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInst;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
	wc.lpszMenuName = MAKEINTRESOURCEW(IDR_MENU1);
	wc.lpszClassName = L"MainWndClass";
	wc.hIconSm = NULL;

	ASSERT(RegisterClassExW(&wc));

	//Create Window
	window.hWnd = CreateWindowExW(
		WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
		L"MainWndClass",
		L"Splat Renderer :)",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL, NULL, hInst, NULL);

	ASSERT(window.hWnd != nullptr);
	ShowWindow(window.hWnd, SW_NORMAL);
	ASSERT(UpdateWindow(window.hWnd));

	Graphics::InitGlobals();

	POINT mousePos;
	ASSERT(GetCursorPos(&mousePos));

	//Main Loop.
	while (MessagePump()) {
		if (RenderCurrent) {
			//Camera Movement
			Float3 move;
			if (KeyDown('W')) move.z += 0.05f;
			if (KeyDown('S')) move.z -= 0.05f;
			if (KeyDown('D')) move.x += 0.05f;
			if (KeyDown('A')) move.x -= 0.05f;
			if (KeyDown(VK_SPACE)) move.y -= 0.05f;
			if (KeyDown(VK_SHIFT)) move.y += 0.05f;
			camera.pos += Matrix4::RotationY(-camera.yaw) * Matrix4::RotationX(-camera.pitch) * move;
			//Camera Rotation
			POINT lastPos = mousePos;
			ASSERT(GetCursorPos(&mousePos));
			if (KeyDown(VK_LBUTTON)) {
				camera.yaw += (float)(mousePos.x - lastPos.x) / 400.0f;
				camera.pitch -= (float)(mousePos.y - lastPos.y) / 400.0f;

				if (camera.pitch > PI / 2.0f) camera.pitch = PI / 2.0f;
				if (camera.pitch < -PI / 2.0f) camera.pitch = -PI / 2.0f;
			}
			//Render Frame
			RenderCurrent();
		}
	}
	if(CleanCurrent) CleanCurrent();
	Graphics::CleanGlobals();

	return 0;
}