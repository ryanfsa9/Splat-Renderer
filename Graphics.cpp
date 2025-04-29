#include "Globals.h"

#include <d3d11.h>
#pragma comment(lib,"d3d11.lib")
#include <d3dcompiler.h>
#pragma comment(lib,"D3DCompiler.lib")

#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
using namespace std;

Matrix4 Camera::Proj() { //Coordinate space: Z forward, X right, Y down
	float h = 1.0f / tan(fov_y / 2.0f);
	float w = h * window.size.y / window.size.x;
	float r = farZ / (farZ - nearZ);
	return Matrix4(
		{ w, 0, 0, 0 },
		{ 0,-h, 0, 0 },
		{ 0, 0, r, -r * nearZ },
		{ 0, 0, 1, 0 }
	);
}
Matrix4 Camera::View() {
	return Matrix4::RotationX(pitch) * Matrix4::RotationY(yaw) * Matrix4::Translation(-pos);
}

//Globals
ID3D11Device* pDevice = nullptr;
IDXGISwapChain* pSwapChain = nullptr;
ID3D11DeviceContext* pContext = nullptr;
ID3D11RenderTargetView* pTarget = nullptr;

void Graphics::InitGlobals() {
	//Swap Chain Description
	DXGI_SWAP_CHAIN_DESC swap_d = {};
	swap_d.BufferDesc.Width = window.size.x;
	swap_d.BufferDesc.Height = window.size.y;
	swap_d.BufferDesc.RefreshRate.Numerator = 0;
	swap_d.BufferDesc.RefreshRate.Denominator = 0;
	swap_d.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swap_d.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swap_d.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swap_d.SampleDesc.Count = 1;
	swap_d.SampleDesc.Quality = 0;
	swap_d.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_d.BufferCount = 2;
	swap_d.OutputWindow = window.hWnd;
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
	HR(pDXGIFactory->MakeWindowAssociation(window.hWnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER));
	pDXGIFactory->AddRef();

	//Create RenderTarget, Viewport
	Resize();
}

void Graphics::Resize() {
	if (!pContext) return; //do nothing if called before Init

	//Unbind Render Target From OutputMerger
	pContext->OMSetRenderTargets(0, 0, 0);

	//If Render Target Exists, Release It (or ResizeBuffers will fail)
	if (pTarget) pTarget->Release();

	//Resize Swapchain
	HR(pSwapChain->ResizeBuffers(0, window.size.x, window.size.y, DXGI_FORMAT_B8G8R8A8_UNORM, 0));

	//Get Render Target
	ID3D11Resource* pBackBuffer;
	HR(pSwapChain->GetBuffer(0u, __uuidof(ID3D11Resource), (void**)&pBackBuffer));
	HR(pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pTarget));
	pBackBuffer->Release();

	//Set Viewport
	D3D11_VIEWPORT vp;
	vp.Width = (float)window.size.x;
	vp.Height = (float)window.size.y;
	vp.MinDepth = 0;
	vp.MaxDepth = 1;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	pContext->RSSetViewports(1, &vp);
}

void Graphics::CleanGlobals() {
	pTarget->Release();
	pContext->Release();
	pSwapChain->Release();
	pDevice->Release();
}

//GS
namespace GS {
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

	namespace GSLoader {
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

			fclose(f);
		}
	}

	namespace SortThread {
		HANDLE thread = nullptr;
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
					Matrix4 View = camera.View();
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
			delete[] temp;
			return 0;
		}
	}

	//GS buffers and shaders
	ID3D11BlendState* pBlend = nullptr;
	//Shaders
	ID3D11InputLayout* il = nullptr;
	ID3D11VertexShader* vs = nullptr;
	ID3D11GeometryShader* gs = nullptr;
	ID3D11PixelShader* ps = nullptr;
	//Buffers
	ID3D11Buffer* vBuffer = nullptr;
	ID3D11Buffer* cBuffer = nullptr;
}	

void GS::Init(const char* ply) {
	//load gaussians
	GSLoader::load(ply);
	
	//create sorter and wait for it to sort
	SortThread::thread = CreateThread(NULL, 0, SortThread::entry, NULL, NULL, NULL);
	ASSERT(SortThread::thread);
	while (!SortThread::flagSorted) {}

	//Compile Vertex Shader
	ID3DBlob* pBlob = nullptr;
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

	pBlob->Release();

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
	HR(pDevice->CreateBuffer(&cdesc, nullptr, &cBuffer));

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

	//Bind stuff
	pContext->OMSetBlendState(pBlend, NULL, 0xFFFFFF);
	pContext->IASetInputLayout(il);
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	UINT stride = sizeof(Gaussian);
	UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, &vBuffer, &stride, &offset);

	pContext->VSSetShader(vs, nullptr, 0u);
	pContext->VSSetConstantBuffers(0, 1, &cBuffer);
	pContext->GSSetShader(gs, nullptr, 0u);
	pContext->PSSetShader(ps, nullptr, 0u);
}

void GS::Render() {
	//Clear render target
	const float color[] = { 0.0f , 0.0f , 0.0f, 0.0f };
	pContext->ClearRenderTargetView(pTarget, color);

	//Update constBuffer
	struct cbuffer {
		Matrix4 Proj; //no padding neccessary, already 16b alligned
		Matrix4 View;
	};

	D3D11_MAPPED_SUBRESOURCE sub;
	HR(pContext->Map(cBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub));

	cbuffer* cbuf = (cbuffer*)sub.pData;
	cbuf->Proj = camera.Proj();
	cbuf->View = camera.View();

	pContext->Unmap(cBuffer, 0);

	//Update vBuffer
	if (SortThread::flagSorted) {
		HR(pContext->Map(vBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub));

		Gaussian* vbuf = (Gaussian*)sub.pData;
		memcpy(vbuf, &gaussians[0], gaussians.size() * sizeof(Gaussian));

		pContext->Unmap(vBuffer, 0);

		SortThread::flagSorted = false;
	}


	//Set Render Target
	pContext->OMSetRenderTargets(1u, &pTarget, nullptr);

	//Draw
	pContext->Draw(gaussians.size(), 0u);

	//Present
	pSwapChain->Present(0u, 0u);
}

void GS::Clean() {
	pBlend->Release();
	il->Release();
	vs->Release();
	gs->Release();
	ps->Release();
	vBuffer->Release();
	cBuffer->Release();

	SortThread::flagExit = true;
	WaitForSingleObject(SortThread::thread, INFINITE);

	SortThread::flagExit = false;
	SortThread::flagSorted = false;
}