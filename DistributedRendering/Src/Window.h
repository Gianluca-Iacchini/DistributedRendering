#pragma once

#include <string>

namespace DR
{
	class Window
	{
	public:
		Window();
		Window(int width, int height, const std::wstring name);
		~Window() { m_NameCounter -= 1; }
		
		bool InitWindow();
		HWND GetRawWindow();

		const int GetWidth() { return m_Width; }
		const int GetHeight() { return m_Height; }
		const std::wstring GetName();

	private:

		HWND m_Hwnd;
		int m_Width;
		int m_Height;
		std::wstring m_Name;
		static int m_NameCounter;
	};
}


