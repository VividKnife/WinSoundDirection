#include "App/ApplicationHost.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    App::ApplicationHost app{hInstance, nCmdShow};
    return app.Run();
}
