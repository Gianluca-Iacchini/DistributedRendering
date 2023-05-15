#define NOMINMAX
#include "drpch.h"
#include "Application.h"
#include <objbase.h>

#include <algorithm>

#include <dx12Lib/Texture.h>
#include <dx12Lib/CommandQueue.h>
#include <dx12Lib/CommandList.h>
#include <dx12Lib/SwapChain.h>
#include "SceneVisitor.h"
#include <dx12Lib/SceneNode.h>
#include <dx12Lib/Adapter.h>
#include <VideoStream.h>


using namespace DR;
using namespace dx12lib;
using namespace DirectX;

Application* Application::m_Instance = nullptr;
const std::wstring Application::SCENE_PATH = L"../Assets/Models/crytek-sponza/sponza_nobanner.obj";

Application::Application()
{
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	m_Logger = Logger();
	m_Logger.CreateConsole();

	m_Camera = Camera();
	m_Camera.SetPosition(XMFLOAT3(0, 1.7f, 0));
	m_Light.resize(1);
	m_Timer = Timer();
}

Application* Application::GetInstance()
{
	if (m_Instance == nullptr)
	{
		m_Instance = new Application();
	}
	return m_Instance;
}

bool Application::InitApplication()
{
	HRESULT hr = CoInitializeEx(NULL, COINIT::COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		Application* app = Application::GetInstance();
		Logger logger = app->GetLogger();

		logger.Error("Failed to initialize CoInitializeEx.");

		return false;
	}

	WNDCLASSEXW wndClass = { 0 };

	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = &WndProcFunc;
	wndClass.hInstance = m_hInstance;
	wndClass.hIcon = LoadIconW(m_hInstance, NULL);
	wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wndClass.lpszMenuName = nullptr;
	wndClass.lpszClassName = m_WndName.c_str();
	wndClass.hIconSm = LoadIcon(m_hInstance, NULL);

	if (!RegisterClassExW(&wndClass))
	{
		MessageBoxW(NULL, L"Failed register window class", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	return true;
}

LRESULT CALLBACK Application::WndProcFunc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_PAINT:
		PAINTSTRUCT ps;
		(void)BeginPaint(hwnd , &ps);
		EndPaint(hwnd, &ps);
		break;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		else if (wParam == VK_SPACE)
		{
			m_Instance->m_MoveLights = !m_Instance->m_MoveLights;
		}
		break;
	case WM_SIZE:
	{
		int width = ((int)(short)LOWORD(lParam));
		int height = ((int)(short)HIWORD(lParam));
		if (m_Instance != nullptr) {
			m_Instance->OnResize(width, height);
		}
		break;
	}
	case WM_MOUSEMOVE:
		m_Instance->UpdateCameraRot(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

const std::wstring Application::GetWndName()
{
	return m_WndName;
}

HINSTANCE Application::GetHInstance()
{
	return m_hInstance;
}

bool Application::NewWindow(int width, int height, std::wstring name)
{
	m_Window = Window(width, height, name);

	if (m_Window.InitWindow())
	{
		m_Viewport = CD3DX12_VIEWPORT(0.f, 0.f, static_cast<float>(m_Window.GetWidth()), static_cast<float>(m_Window.GetHeight()));
		m_ScissorRect = { 0, 0, LONG_MAX, LONG_MAX };
		return true;
	}

	return false;
}

void Application::Run()
{
	std::ofstream fpOut("TestUDP.h265", std::ios::out | std::ios::binary);

	MSG msg = { 0 };

	//VideoStream mClient = VideoStream();
	//mClient.InitializeAsClient(19998);
	//mClient.ClientListen(fpOut);

	m_Timer.Reset();

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			m_Timer.Tick();
			if (m_FinishedLoading)
			{
				ShowWindow(m_Window.GetRawWindow(), SW_SHOW);
				UpdateWindow(m_Window.GetRawWindow());

				m_FinishedLoading = false;
			}
			else if (m_SceneLoaded)
			{
				Update();
				Render(fpOut);
			}
		}
	}
	m_HEVCEncoder->EndEncode(fpOut);
	//mClient.StopListening();
	fpOut.close();
}

bool Application::LoadScene()
{
	m_Device = Device::Create();

	m_HEVCEncoder = std::make_shared<HEVCEncoder>(m_Device);



	if (!m_Device)
	{
		m_Logger.Error("Failed to create device");
		return false;
	}



	m_SwapChain = m_Device->CreateSwapChain(m_Window.GetRawWindow(), DXGI_FORMAT_B8G8R8A8_UNORM);

	m_LoadingTask = std::async(std::launch::async, std::bind(&Application::LoadData, this));

	auto& commandQueue = m_Device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto  commandList = commandQueue.GetCommandList();

	m_LightingPSO = std::make_shared<DirectLightingPSO>(m_Device, L"Shaders/Compiled/Basic_VS.cso", L"Shaders/Compiled/Lighting_PS.cso");
	//m_EffectPSO = std::make_shared<EffectPSO>(m_Device, true, false);

	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

	DXGI_SAMPLE_DESC sampleDesc = m_Device->GetMultisampleQualityLevels(backBufferFormat);

	auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(backBufferFormat, m_Width, m_Height, 1, 1, sampleDesc.Count, sampleDesc.Quality, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

	D3D12_CLEAR_VALUE colorClearValue;
	colorClearValue.Format = colorDesc.Format;
	colorClearValue.Color[0] = 0.8f;
	colorClearValue.Color[1] = 0.8f;
	colorClearValue.Color[2] = 0.8f;
	colorClearValue.Color[3] = 1.f;

	std::shared_ptr<Texture> colorTexture = m_Device->CreateTexture(colorDesc, &colorClearValue);
	colorTexture->SetName(L"Color Render Target");

	auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat, m_Width, m_Height, 1, 1, sampleDesc.Count,
		sampleDesc.Quality, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	D3D12_CLEAR_VALUE depthClearValue;
	depthClearValue.Format = depthDesc.Format;
	depthClearValue.DepthStencil = { 1.0f, 0 };

	auto depthTexture = m_Device->CreateTexture(depthDesc, &depthClearValue);
	depthTexture->SetName(L"Depth Render Target");

	m_RenderTarget.AttachTexture(AttachmentPoint::Color0, colorTexture);
	m_RenderTarget.AttachTexture(AttachmentPoint::DepthStencil, depthTexture);

	//commandQueue.Flush();

	return true;
}

bool Application::LoadData()
{
	auto& commandQueue = m_Device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue.GetCommandList();

	auto scene = commandList->LoadSceneFromFile(SCENE_PATH);

	commandQueue.ExecuteCommandList(commandList);

	commandQueue.Flush();

	if (scene)
	{
		m_Logger.Info("Successfully loaded scene!");
		
		scene->GetRootNode()->SetLocalTransform(XMMatrixScaling(0.01f, 0.01f, 0.01f));

		m_Scene = scene;
		m_SceneLoaded = true;
		m_FinishedLoading = true;
		return true;

	}
	else
	{
		m_Logger.Error("Could not load scene.");
		return false;
	}


}

void Application::Update()
{


	ComputeStats();
	
	m_SwapChain->WaitForSwapChain();

	UpdateCameraPos();

	XMMATRIX viewMatrix = m_Camera.GetView();
	
	static float lightMoveTime = 0.0f;

	const float radius = 1.0f;

	if (m_MoveLights)
	{
		lightMoveTime += static_cast<float>(m_Timer.DeltaTime()) * 0.5f * XM_PI;
	}

	XMVECTORF32 positionWS = { static_cast<float>(std::sin(lightMoveTime)) * radius, radius,
							   static_cast<float>(std::cos(lightMoveTime)) * radius, 1.0f };
	
	XMVECTOR directionWS = XMVector3Normalize(XMVectorNegate(positionWS));
	XMVECTOR directionVS = XMVector3TransformNormal(directionWS, viewMatrix);

	DirectionalLight& l = m_Light[0];

	XMStoreFloat4(&l.DirectionWS, directionWS);
	XMStoreFloat4(&l.DirectionVS, directionVS);

	l.Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	m_LightingPSO->SetDirectionalLight(m_Light);
	//m_EffectPSO->SetDirectionalLights(m_Light);
}

void Application::Render(std::ofstream& fpOut)
{
	auto& commandQueue = m_Device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto  commandList = commandQueue.GetCommandList();

	// Full Screen

	//SceneVisitor litPass(*commandList, m_Camera, *m_EffectPSO, false);
	SceneVisitor litPass(*commandList, m_Camera, *m_LightingPSO, false);

	auto renderTarget = m_RenderTarget;

	FLOAT clearColor[] = {0.8f, 0.8f, 0.8f, 1.0f};
	commandList->ClearTexture(renderTarget.GetTexture(AttachmentPoint::Color0), clearColor);
	commandList->ClearDepthStencilTexture(renderTarget.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);

	commandList->SetViewport(m_Viewport);
	commandList->SetScissorRect(m_ScissorRect);
	commandList->SetRenderTarget(m_RenderTarget);

	m_Scene->Accept(litPass);

	auto swapChainBackBuffer = m_SwapChain->GetRenderTarget().GetTexture(AttachmentPoint::Color0);
	auto msaaRenderTarget = m_RenderTarget.GetTexture(AttachmentPoint::Color0);

	commandList->ResolveSubresource(swapChainBackBuffer, msaaRenderTarget);

	commandQueue.ExecuteCommandList(commandList);



	m_SwapChain->Present();


	if (m_Timer.TotalTime() * 1000.f - lastRenderTime > fps30Lock)
	{
		m_HEVCEncoder->EncodeFrame(swapChainBackBuffer->GetD3D12Resource().Get(), fpOut);
		//m_HEVCEncoder->EndEncode(fpOut);
		lastRenderTime = m_Timer.TotalTime() * 1000.f;
	}
}

void Application::ComputeStats()
{
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	timeElapsed += m_Timer.DeltaTime();
	frameCnt++;

	if (timeElapsed > 1.0f)
	{
		float fps = (float)frameCnt;
		float ms = 1000.0f / fps;

		std::wstring fpsString = std::to_wstring(fps);
		std::wstring msString = std::to_wstring(ms);

		std::wstring windowTitle = m_Window.GetName() + L" [FPS]: " + fpsString + L" [MS]: " + msString;

		SetWindowText(m_Window.GetRawWindow(), windowTitle.c_str());

		frameCnt = 0;
		timeElapsed = 0;
	}

	totalFrames++;
}

void Application::UpdateCameraPos()
{
	float movSpeed = m_MoveSpeed;

	if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
	{
		movSpeed *= 2;
	}
	
	if (GetAsyncKeyState('W') & 0x8000)
	{
		m_Camera.Walk(movSpeed * m_Timer.DeltaTime());
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		m_Camera.Walk(-movSpeed* m_Timer.DeltaTime());
	}

	if (GetAsyncKeyState('D') & 0x8000)
	{
		m_Camera.Strafe(movSpeed * m_Timer.DeltaTime());
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		m_Camera.Strafe(-movSpeed * m_Timer.DeltaTime());
	}
	if (GetAsyncKeyState('E') & 0x8000)
	{
		m_Camera.Fly(movSpeed * m_Timer.DeltaTime());
	}
	if (GetAsyncKeyState('Q') & 0x8000)
	{
		m_Camera.Fly(-movSpeed * m_Timer.DeltaTime());
	}

	m_Camera.UpdateViewMatrix();
}

void Application::UpdateCameraRot(WPARAM wParam, int x, int y)
{
	if (m_FirstMove)
	{
		m_FirstMove = false;
		m_LastXPos = x;
		m_LastYPos = y;
	}

	if ((wParam & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - m_LastXPos));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - m_LastYPos));

		m_Camera.Pitch(dy * m_Timer.DeltaTime() * m_RotSpeed);
		m_Camera.Yaw(dx * m_Timer.DeltaTime() * m_RotSpeed);
	}

	m_LastXPos = x;
	m_LastYPos = y;
}

void Application::OnResize(int width, int height)
{
	m_Width = width > 1 ? width : 1;
	m_Height = height > 1 ? height : 1;

	m_Camera.SetFrustum(0.25f * XM_PI, m_Width / (float)m_Height, 0.1f, 1000.f);
	m_Viewport = CD3DX12_VIEWPORT(0.f, 0.f, static_cast<float>(m_Width), static_cast<float>(m_Height));

	m_RenderTarget.Resize(m_Width, m_Height);

	m_SwapChain->Resize(m_Width, m_Height);
}
