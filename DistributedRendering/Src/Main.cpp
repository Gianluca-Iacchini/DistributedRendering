#include "drpch.h"
#include <dx12Lib/Device.h>

#include <shellapi.h>
#include "Application.h"

using namespace dx12lib;
using namespace DR;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
#if defined( _DEBUG )
    // Always enable the Debug layer before doing anything with DX12.
    Device::EnableDebugLayer();
#endif

    Application* app = Application::GetInstance();
    Logger logger = app->GetLogger();

    // Set the working directory
    WCHAR path[MAX_PATH];
    int     argc = 0;
    LPWSTR* argv = ::CommandLineToArgvW(lpCmdLine, &argc);
    if (argv)
    {
        for (int i = 0; i < argc; ++i)
        {
            // -wd Specify the Working Directory.
            if (::wcscmp(argv[i], L"-wd") == 0)
            {


                ::wcscpy_s(path, argv[++i]);
                ::SetCurrentDirectoryW(path);

                std::wstring p(path);
                std::string pp(p.begin(), p.end());

                logger.Info(pp);
            }
        }
        ::LocalFree(argv);
    }

    int retCode = 0;

    //GameFramework::Create(hInstance);
    //{
    //    auto demo = std::make_unique<Tutorial5>(L"Models", 1920, 1080);
    //    retCode = demo->Run();
    //}
    //// Destroy game framework resource.
    //GameFramework::Destroy();

    //::atexit(&Device::ReportLiveObjects);



    if (!app->InitApplication())
    {
        logger.Error("Could not init application");

        retCode = -1;
    }

    if (!app->NewWindow(1920, 1080, L"Main Window"))
    {
        logger.Error("Could not init Main Window");
        retCode = -2;
    }

    logger.Info("App successfully initialized!");

    if (!app->LoadScene())
    {
        logger.Error("Could not load scene.");
    }

    app->Run();

    return retCode;
}