#include "Math.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <d3d11.h>
#pragma comment(lib,"d3d11.lib")
#include <d3dcompiler.h>
#pragma comment(lib,"D3DCompiler.lib")

#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
using namespace std;

struct Gaussian { //matches vertexShader input
	Float3 pos;
	Float4 col;
	Float3 cov0;
	Float3 cov1;
	//cov matrix symetric, so only need to store 6 values as below. stored in cov0,cov1
	/*
	0 1 2
	- 3 4
	- - 5
	*/
};
vector<Gaussian> gaussians;

namespace Window {
	HINSTANCE hInst;
	HWND hWnd;
	Int2 size; //client area

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
		std::wstringstream text;
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
	}
	void TextBox(const char* msg) {
		MessageBoxA(NULL, msg, "Hello:", MB_ICONINFORMATION);
	}
	bool keyDown(char key) {
		return GetAsyncKeyState(key) >> 15;
	}
}
#define ASSERT(b) if(!(b)) { Window::Error(__FILE__, __LINE__, ""); }
#define ASSERTMSG(b, m) if(!(b)) { Window::Error(__FILE__, __LINE__, m); }
#define HR(b) if(b!=S_OK) Window::HRError(b, __FILE__, __LINE__);

namespace PLYLoader {
	struct Gaussian_Ply { //must match the data order and sizes of the ply file
		Float3 pos;
		Float3 normal;
		Float3 col;
		Float3 sh[15];
		float opacity;
		Float3 scale;
		Quaternion rot;
	};
	void load(const char* file) {
		gaussians.clear();

		//read gaussians
		FILE* f;
		fopen_s(&f, file, "r+b");
		ASSERT(f);

		char buf[256];
		int i = 80;
		while (--i > 0) {
			fgets(buf, 256, f);
			if (string(buf) == string("end_header\n")) break;
		}

		Gaussian_Ply gbuffer[200];
		size_t read_total = 0;
		size_t read = 0;

		do {
			read = fread(gbuffer, sizeof(Gaussian_Ply), 200, f);
			read_total += read;
			for (size_t i = 0; i < read; i++) {
				//parse raw file data to desired format
				Gaussian_Ply& raw = gbuffer[i];
				Gaussian g;

				//credit to https://github.com/antimatter15/splat/blob/main/main.js for how to interpret the raw data in the ply files, the data seems to be stored in very unintuitive ways.

				//position
				g.pos = raw.pos;

				//exponentiate scales
				Float3 scale = Float3(exp(raw.scale.x), exp(raw.scale.y), exp(raw.scale.z));

				//normalize rotation
				Quaternion q = raw.rot;
				float qnorm = sqrt(q.w * q.w + q.u.dot(q.u));
				q.w /= qnorm;
				q.u /= qnorm;

				//cov matrix
				Matrix4 RS = Matrix4::RotationQuat(q) * Matrix4::Scaling(scale);
				RS = RS * RS.T();
				g.cov0 = Float3(RS[0][0], RS[0][1], RS[0][2]);
				g.cov1 = Float3(RS[1][1], RS[1][2], RS[2][2]);

				//color
				const float SH_C0 = 0.28209479177387814; //why? idk.
				Float3 col = gbuffer[i].col * SH_C0 + Float3(0.5f, 0.5f, 0.5f);
				float alpha = 1.0 / (1.0 + exp(-raw.opacity));
				g.col = Float4(col.x, col.y, col.z, alpha);

				gaussians.push_back(g);
			}
		} while (read == 200);
	}
}

namespace Camera {
	Float3 pos;
	float yaw, pitch;

	constexpr static float nearZ = 0.1f;
	constexpr static float farZ = 1000.0f;
	constexpr static float fov_y = 60.0f * PI / 180.0f;

	Matrix4 View() {
		return Matrix4::RotationX(pitch) * Matrix4::RotationY(yaw) * Matrix4::Translation(-pos);
	}
	Matrix4 Proj() { //Coordinate space: Z forward, X right, Y down
		float h = 1.0f / tan(fov_y / 2.0f);
		float w = h * Window::size.y / Window::size.x;
		float r = farZ / (farZ - nearZ);
		return Matrix4(
			{ w, 0, 0, 0 },
			{ 0,-h, 0, 0 },
			{ 0, 0, r, -r * nearZ },
			{ 0, 0, 1, 0 }
		);
	}
}

namespace SortThread {
	bool flagSorted = false;
	bool flagExit = false;

	struct ref {
		int index;
		float depth;
	};

	bool compareRef(const ref& a, const ref& b) {
		return a.depth < b.depth;
	}

	DWORD entry(void*) {
		vector<ref> refs(gaussians.size());
		Gaussian* temp = new Gaussian[gaussians.size()];
		while (!flagExit) {
			if (!flagSorted) {
				//compute depths and copy gaussians into temp
				Matrix4 View = Camera::View();
				for (int i = 0; i < gaussians.size(); i++) {
					Float3 cam_pos = View * gaussians[i].pos;
					refs[i].index = i;
					refs[i].depth = cam_pos.z;
					temp[i] = gaussians[i];
				}
				//sort refs
				sort(refs.begin(), refs.end(), compareRef); //std::sort probably uses quickSort. a radix/counting sort would be faster but whatever

				//move
				for (int i = 0; i < gaussians.size(); i++) {
					gaussians[i] = temp[refs[i].index];
				}

				flagSorted = true;
			}
		}
		return 0;
	}
}

//Graphics
namespace Graphics {
	ID3D11Device* pDevice = nullptr;
	IDXGISwapChain* pSwapChain = nullptr;
	ID3D11DeviceContext* pContext = nullptr;
	ID3D11RenderTargetView* pTarget = nullptr;
	ID3D11BlendState* pBlend = nullptr;
	ID3DBlob* pBlob = nullptr;

	//Shaders
	ID3D11InputLayout* il = nullptr;
	ID3D11VertexShader* vs = nullptr;
	ID3D11GeometryShader* gs = nullptr;
	ID3D11PixelShader* ps = nullptr;

	//Buffers
	ID3D11Buffer* vBuffer = nullptr;
	ID3D11Buffer* constBuffer = nullptr;

	void Init();
	void Resize();
	void RenderGS();

	void Init() {
		//Swap Chain Description
		DXGI_SWAP_CHAIN_DESC swap_d = {};
		swap_d.BufferDesc.Width = Window::size.x;
		swap_d.BufferDesc.Height = Window::size.y;
		swap_d.BufferDesc.RefreshRate.Numerator = 0;
		swap_d.BufferDesc.RefreshRate.Denominator = 0;
		swap_d.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swap_d.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swap_d.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swap_d.SampleDesc.Count = 1;
		swap_d.SampleDesc.Quality = 0;
		swap_d.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swap_d.BufferCount = 2;
		swap_d.OutputWindow = Window::hWnd;
		swap_d.Windowed = TRUE;
		swap_d.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swap_d.Flags = 0;
		
		D3D_FEATURE_LEVEL featureLevels[1]{ D3D_FEATURE_LEVEL_11_1 };
		D3D_FEATURE_LEVEL* returnedFeatureLevel = new D3D_FEATURE_LEVEL;
		//Create Device, SwapChain, Context
		HR(D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			0,
			featureLevels,
			1,
			D3D11_SDK_VERSION,
			&swap_d,
			&pSwapChain,
			&pDevice,
			returnedFeatureLevel,
			&pContext
		));
		ASSERTMSG(*returnedFeatureLevel == D3D_FEATURE_LEVEL_11_1, "Failed to create feature level 11_1");

		//Get Parent Factory and tell DX not to handle Alt+Enter
		IDXGIFactory* pDXGIFactory;
		HR(pSwapChain->GetParent(__uuidof(IDXGIFactory), (void**)&pDXGIFactory));
		HR(pDXGIFactory->MakeWindowAssociation(Window::hWnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER));
		pDXGIFactory->AddRef();

		//Create RenderTarget, Viewport
		Graphics::Resize();

		//Compile Vertex Shader
		ID3DBlob* errorBlob = nullptr;
		HR(D3DCompileFromFile(L"GS_Shaders.hlsl", nullptr, nullptr, "vertexShader", "vs_5_0", 0, 0, &pBlob, &errorBlob));
		//Window::TextBox((char*)errorBlob->GetBufferPointer());
		HR(pDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &vs));

		//Create IED
		D3D11_INPUT_ELEMENT_DESC ied_desc[4] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT   , 0, 0                           , D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT   , 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT   , 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};
		
		HR(pDevice->CreateInputLayout(ied_desc, 4, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), &il));

		//Compile Geometery Shader
		HR(D3DCompileFromFile(L"GS_Shaders.hlsl", nullptr, nullptr, "geometryShader", "gs_5_0", 0, 0, &pBlob, &errorBlob));
		HR(pDevice->CreateGeometryShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &gs));

		//Compile Pixel Shader
		HR(D3DCompileFromFile(L"GS_Shaders.hlsl", nullptr, nullptr, "pixelShader", "ps_5_0", 0, 0, &pBlob, &errorBlob));
		HR(pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &ps));

		//Create VBuffer
		D3D11_BUFFER_DESC vdesc = {};
		vdesc.ByteWidth = gaussians.size() * sizeof(Gaussian);
		vdesc.Usage = D3D11_USAGE_DYNAMIC;
		vdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		D3D11_SUBRESOURCE_DATA subdata = {};
		subdata.pSysMem = &gaussians[0];
		pDevice->CreateBuffer(&vdesc, &subdata, &vBuffer);

		//Create ConstBuffer
		D3D11_BUFFER_DESC cdesc = {};
		//ByteWidth must be a multiple of 16
		cdesc.ByteWidth = 32 * sizeof(float);
		cdesc.Usage = D3D11_USAGE_DYNAMIC;
		cdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		HR(pDevice->CreateBuffer(&cdesc, nullptr, &constBuffer));

		//Create Blend State
		D3D11_BLEND_DESC blenddesc = {};
		blenddesc.RenderTarget[0].BlendEnable = TRUE;
		blenddesc.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_DEST_ALPHA;
		blenddesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		blenddesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blenddesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;
		blenddesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		blenddesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blenddesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		HR(pDevice->CreateBlendState(&blenddesc, &pBlend));
	}
	void Resize() {
		if (!pContext) return; //do nothing if called before Init

		//Unbind Render Target From OutputMerger
		pContext->OMSetRenderTargets(0, 0, 0);

		//If Render Target Exists, Release It (or ResizeBuffers will fail)
		if (pTarget) pTarget->Release();

		//Resize Swapchain
		HR(pSwapChain->ResizeBuffers(0, Window::size.x, Window::size.y, DXGI_FORMAT_B8G8R8A8_UNORM, 0));

		//Get Render Target
		ID3D11Resource* pBackBuffer;
		HR(pSwapChain->GetBuffer(0u, __uuidof(ID3D11Resource), (void**)&pBackBuffer));
		HR(pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pTarget));
		pBackBuffer->Release();

		//Set Viewport
		D3D11_VIEWPORT vp;
		vp.Width = (float)Window::size.x;
		vp.Height = (float)Window::size.y;
		vp.MinDepth = 0;
		vp.MaxDepth = 1;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		pContext->RSSetViewports(1, &vp);
	}
	void RenderGS() {
		//Clear render target
		const float color[] = { 0.0f , 0.0f , 0.0f, 0.0f };
		Graphics::pContext->ClearRenderTargetView(Graphics::pTarget, color);

		//Update constBuffer
		struct cbuffer {
			Matrix4 Proj; //no padding neccessary, already 16b alligned
			Matrix4 View;
		};

		D3D11_MAPPED_SUBRESOURCE sub;
		HR(pContext->Map(constBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub));

		cbuffer* cbuf = (cbuffer*)sub.pData;
		cbuf->Proj = Camera::Proj();
		cbuf->View = Camera::View();

		pContext->Unmap(constBuffer, 0);

		//Update vBuffer
		if (SortThread::flagSorted) {
			HR(pContext->Map(vBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub));

			Gaussian* vbuf = (Gaussian*)sub.pData;
			memcpy(vbuf, &gaussians[0], gaussians.size() * sizeof(Gaussian));
			
			pContext->Unmap(constBuffer, 0);

			SortThread::flagSorted = false;
		}


		//Set Render Target
		pContext->OMSetRenderTargets(1u, &pTarget, nullptr);
		pContext->OMSetBlendState(pBlend, NULL, 0xFFFFFF);

		//Bind Stuff
		pContext->IASetInputLayout(il);
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		UINT stride = sizeof(Gaussian);
		UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, &vBuffer, &stride, &offset);

		pContext->VSSetShader(vs, nullptr, 0u);
		pContext->VSSetConstantBuffers(0, 1, &constBuffer);
		pContext->GSSetShader(gs, nullptr, 0u);
		pContext->PSSetShader(ps, nullptr, 0u);

		pContext->Draw(gaussians.size(), 0u);

		//Present
		Graphics::pSwapChain->Present(0u, 0u);
	}
	void Clean() {
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_WINDOWPOSCHANGED:
	{
		RECT r;
		GetClientRect(hWnd, &r);
		Int2 newSize(r.right - r.left, r.bottom - r.top);

		//resize
		if (Window::size != newSize) {
			Window::size = newSize;
			Graphics::Resize();
		}
		return 0;
	}
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
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PSTR pCmdLine, _In_ int nCmdShow) {
	Window::hInst = hInstance;

	//Register WndClass
	WNDCLASSEXW wc;
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = Window::hInst;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"MainWndClass";
	wc.hIconSm = NULL;

	ASSERT(RegisterClassExW(&wc));

	//Create Window
	Window::hWnd = CreateWindowExW(
		WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
		L"MainWndClass",
		L"GS Renderer :)",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL, NULL, Window::hInst, NULL);

	ASSERT(Window::hWnd != nullptr);
	ShowWindow(Window::hWnd, SW_NORMAL);
	ASSERT(UpdateWindow(Window::hWnd));

	PLYLoader::load("point_cloud_bonsai.ply");
	
	//create sorter and wait for it to sort
	HANDLE sorter = CreateThread(NULL, 0, SortThread::entry, NULL, NULL, NULL);
	ASSERT(sorter);
	while (!SortThread::flagSorted) {}

	Graphics::Init();

	POINT mousePos;
	ASSERT(GetCursorPos(&mousePos));

	//Main Loop.
	while (MessagePump()) {
		//Camera Movement
		Float3 move;
		if (Window::keyDown('W')) move.z += 0.05f;
		if (Window::keyDown('S')) move.z -= 0.05f;
		if (Window::keyDown('D')) move.x += 0.05f;
		if (Window::keyDown('A')) move.x -= 0.05f;
		if (Window::keyDown(VK_SHIFT)) move.y -= 0.05f;
		if (Window::keyDown(VK_CONTROL)) move.y += 0.05f;
		Camera::pos += Matrix4::RotationY(-Camera::yaw) * Matrix4::RotationX(-Camera::pitch)* move;
		//Camera Rotation
		POINT lastPos = mousePos;
		ASSERT(GetCursorPos(&mousePos));
		if (Window::keyDown(VK_LBUTTON)) {
			Camera::yaw += (float)(mousePos.x - lastPos.x) / 400.0f;
			Camera::pitch -= (float)(mousePos.y - lastPos.y) / 400.0f;

			if (Camera::pitch >  PI / 2.0f) Camera::pitch =  PI / 2.0f;
			if (Camera::pitch < -PI / 2.0f) Camera::pitch = -PI / 2.0f;
		}
		//Render Frame
		Graphics::RenderGS();
	}
	SortThread::flagExit = true;
	WaitForSingleObject(sorter, INFINITE);

	return 0;
}