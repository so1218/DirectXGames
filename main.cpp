#include <Windows.h>
#include <cstdint>
#include <string>
#include <format>

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // メッセージに応じてゲーム固有の処理を行う
    switch (msg)
    {
        // ウィンドウが破壊された
    case WM_DESTROY:
        // OSに対して、アプリの終了を伝える
        PostQuitMessage(0);
        return 0;
    }

    // 標準のメッセージ処理を行う
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void Log(const std::string& message)
{
    OutputDebugStringA(message.c_str());
}


// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) 
{
    // クライアント領域のサイズ
    const int32_t kClientWidth = 1280;
    const int32_t kClientHeight = 720;

    // ウィンドウサイズを表す構造体にクライアント領域を入れる
    RECT wrc = { 0,0,kClientWidth ,kClientHeight };

    // クライアント領域を元に実際のサイズにwrcを変更してもらう
    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

    WNDCLASS wc = {};
    // ウィンドウプロシージャ
    wc.lpfnWndProc = WindowProc;
    // ウィンドウクラス名
    wc.lpszClassName = L"CG2WindowClass";
    // インスタンスハンドル
    wc.hInstance = GetModuleHandle(nullptr);
    // カーソル
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    //ウィンドウクラスを登録する
    RegisterClass(&wc);

    // ウィンドウの生成
    HWND hwnd = CreateWindow(
        wc.lpszClassName,       // 利用するクラス名
        L"CG2",                 // タイトルバーの文字
        WS_OVERLAPPEDWINDOW,    // ウィンドウスタイル
        CW_USEDEFAULT,          // 表示X座標
        CW_USEDEFAULT,
        wrc.right - wrc.left,
        wrc.bottom - wrc.top,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr
    );

    // ウィンドウを表示する
    ShowWindow(hwnd, SW_SHOW);

	// 出力ウィンドウへの文字出力
	OutputDebugStringA("Hello,DirectX!\n");

    // 文字列を格納する
    std::string str0{ "STRING!!" };

    // 整数を文字列にする
    std::string str1{ std::to_string(10) };

    // string->wstring
    std::wstring ConvertString(const std::string & str);
    // wstring->string
    std::string ConvertString(const std::wstring & str);

    

    MSG msg{};
    // ウィンドウのxボタンが押されるまでのループ
    while (msg.message != WM_QUIT)
    {
        // Windowにメッセージが来てたら最優先で処理させる
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // ゲームの処理
        }
    }

	return 0;
}

