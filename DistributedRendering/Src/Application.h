#pragma once
#include "Logger.h"
#include "Window.h"
#include <dx12Lib/Device.h>
#include "DirectLightningPSO.h"
#include <dx12Lib/RenderTarget.h>
#include <dx12Lib/Scene.h>
#include "Camera.h"
#include "Timer.h"
#include "LightTypes.h"
#include <future>
#include "HEVCEncoder.h"


#include <fstream>

namespace DR{
	

	class Application
	{


	public:
		Application(Application& other) = delete;
		void operator=(const Application&) = delete;

		static Application* GetInstance();
		const std::wstring GetWndName();
		HINSTANCE GetHInstance();
		bool InitApplication();
		bool NewWindow(int width, int height, const std::wstring name);
		Logger GetLogger() { return m_Logger; }

		bool LoadScene();
		
		void Run();

		int GetWidth() { return m_Width; };
		int GetHeight() { return m_Height; }

	protected:
		Application();
		bool LoadData();
		void Update();
		void Render(std::ofstream& fpOut);
		void ComputeStats();
		void UpdateCameraPos();
		void UpdateCameraRot(WPARAM btnState, int x, int y);
		void OnResize(int width, int height);

		D3D12_RECT m_ScissorRect;
		D3D12_VIEWPORT m_Viewport;

		static LRESULT CALLBACK WndProcFunc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

		static Application* m_Instance;
		

		const std::wstring m_WndName = L"Distributed Rendering";
		const float m_RotSpeed = 75.0f;
		float m_MoveSpeed = 5.0f;

		int m_Width = 1920;
		int m_Height = 1080;

		Logger m_Logger;
		HINSTANCE m_hInstance;
		Window m_Window;


		std::shared_ptr<dx12lib::Device> m_Device;
		std::shared_ptr<dx12lib::SwapChain> m_SwapChain;

		std::shared_ptr<DirectLightingPSO> m_LightingPSO;

		bool m_shouldStart = false;

		dx12lib::RenderTarget m_RenderTarget;
		std::shared_ptr<dx12lib::Scene> m_Scene;

		std::shared_ptr<HEVCEncoder> m_HEVCEncoder;

		static const std::wstring SCENE_PATH;
		
		Camera m_Camera;

		Timer m_Timer;



		bool m_FinishedLoading = false;
		bool m_SceneLoaded = false;
		bool m_MoveLights = true;

		std::future<bool> m_LoadingTask;

		std::vector<DirectionalLight> m_Light;

		int m_LastXPos = 0;
		int m_LastYPos = 0;
		bool m_FirstMove = true;

		uint64_t totalFrames = 0;

		float fps30Lock = 1000.f / 60.f;
		float lastRenderTime = 0.0f;
	};
}

