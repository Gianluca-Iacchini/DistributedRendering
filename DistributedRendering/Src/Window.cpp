#include "drpch.h"
#include "Window.h"

#include <dx12Lib/Helpers.h>

#include <Application.h>

using namespace DR;

int Window::m_NameCounter = 0;

Window::Window()
{
	this->m_Width = 1920;
	this->m_Height = 1080;
	this->m_Name = L"Window" + m_NameCounter;
	m_NameCounter += 1;
}

Window::Window(int width, int height, std::wstring name)
{
	this->m_Width = width;
	this->m_Height = height;
	this->m_Name = name;
}

bool Window::InitWindow()
{
	RECT windowRect = { 0, 0, m_Width, m_Height };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	int width = windowRect.right - windowRect.left;
	int height = windowRect.bottom - windowRect.top;

	Application* instance = Application::GetInstance();
	
	std::wstring hwndName = instance->GetWndName();
	HINSTANCE hInstance = instance->GetHInstance();

	m_Hwnd = CreateWindow(hwndName.c_str(), this->m_Name.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, hInstance, 0);

	if (!m_Hwnd)
	{
		Logger logger = instance->GetLogger();
		logger.Error("Failed to create Window {0}", ConvertString(m_Name));

		return false;
	}

	return true;
}

HWND Window::GetRawWindow()
{
	return m_Hwnd;
}

const std::wstring Window::GetName()
{
	return m_Name;
}