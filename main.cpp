#include <Windows.h>
#include <cstdint>
#include <string>
#include <format>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <DbgHelp.h>
#include <strsafe.h>
#include <dxgidebug.h>
#include <dxcapi.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

struct Vector3
{
    float x, y, z;
};

struct Vector4
{
    float x, y, z, w;
};

struct Matrix4x4
{
    float m[4][4];
};

struct Transform
{
    Vector3 scale;
    Vector3 rotate;
    Vector3 translate;
};

Matrix4x4 MakeIdentity4x4()
{
    Matrix4x4 matrix{};
    matrix.m[0][0] = 1.0f;
    matrix.m[1][1] = 1.0f;
    matrix.m[2][2] = 1.0f;
    matrix.m[3][3] = 1.0f;
    return matrix;
}

Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
    Matrix4x4 result = {};

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            result.m[row][col] = 0.0f;
            for (int k = 0; k < 4; ++k) {
                result.m[row][col] += m1.m[row][k] * m2.m[k][col];
            }
        }
    }

    return result;
}

// 平行移動行列
Matrix4x4 MakeTranslateMatrix(const Vector3& translate)
{
    Matrix4x4 matrix = {};

    // 単位行列
    for (int i = 0; i < 4; ++i)
    {
        matrix.m[i][i] = 1.0f;
    }

    // 平行移動成分を設定
    matrix.m[3][0] = translate.x;
    matrix.m[3][1] = translate.y;
    matrix.m[3][2] = translate.z;

    return matrix;
}

// 拡大縮小行列
Matrix4x4 MakeScaleMatrix(const Vector3& scale)
{
    Matrix4x4 matrix = {};

    // 拡大縮小成分を設定
    matrix.m[0][0] = scale.x;
    matrix.m[1][1] = scale.y;
    matrix.m[2][2] = scale.z;
    matrix.m[3][3] = 1.0f;

    return matrix;
}

// X軸回転行列
Matrix4x4 MakeRotateXMatrix(float radian)
{
    Matrix4x4 matrix = {};

    matrix.m[0][0] = 1.0f;
    matrix.m[1][1] = std::cosf(radian);
    matrix.m[1][2] = std::sinf(radian);
    matrix.m[2][1] = -std::sinf(radian);
    matrix.m[2][2] = std::cosf(radian);
    matrix.m[3][3] = 1.0f;

    return matrix;
}

// Y軸回転行列
Matrix4x4 MakeRotateYMatrix(float radian)
{
    Matrix4x4 matrix = {};

    matrix.m[0][0] = std::cosf(radian);
    matrix.m[0][2] = -std::sinf(radian);
    matrix.m[1][1] = 1.0f;
    matrix.m[2][0] = std::sinf(radian);
    matrix.m[2][2] = std::cosf(radian);
    matrix.m[3][3] = 1.0f;

    return matrix;
}

// Z軸回転行列
Matrix4x4 MakeRotateZMatrix(float radian)
{
    Matrix4x4 matrix = {};

    matrix.m[0][0] = std::cosf(radian);
    matrix.m[0][1] = std::sinf(radian);
    matrix.m[1][0] = -std::sinf(radian);
    matrix.m[1][1] = std::cosf(radian);
    matrix.m[2][2] = 1.0f;
    matrix.m[3][3] = 1.0f;

    return matrix;
}

// アフィン変換行列
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate)
{
    Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);
    Matrix4x4 rotateXMatrix = MakeRotateXMatrix(rotate.x);
    Matrix4x4 rotateYMatrix = MakeRotateYMatrix(rotate.y);
    Matrix4x4 rotateZMatrix = MakeRotateZMatrix(rotate.z);
    Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

    // 回転行列を合成
    Matrix4x4 rotateMatrix = Multiply(Multiply(rotateXMatrix, rotateYMatrix), rotateZMatrix);

    // 最終的なアフィン行列
    Matrix4x4 affineMatrix = Multiply(Multiply(scaleMatrix, rotateMatrix), translateMatrix);

    return affineMatrix;
}

// 逆行列
Matrix4x4 Inverse(const Matrix4x4& m)
{
    Matrix4x4 result{};
    float inv[16], det;
    const float* src = &m.m[0][0];

    inv[0] = src[5] * src[10] * src[15] -
        src[5] * src[11] * src[14] -
        src[9] * src[6] * src[15] +
        src[9] * src[7] * src[14] +
        src[13] * src[6] * src[11] -
        src[13] * src[7] * src[10];

    inv[4] = -src[4] * src[10] * src[15] +
        src[4] * src[11] * src[14] +
        src[8] * src[6] * src[15] -
        src[8] * src[7] * src[14] -
        src[12] * src[6] * src[11] +
        src[12] * src[7] * src[10];

    inv[8] = src[4] * src[9] * src[15] -
        src[4] * src[11] * src[13] -
        src[8] * src[5] * src[15] +
        src[8] * src[7] * src[13] +
        src[12] * src[5] * src[11] -
        src[12] * src[7] * src[9];

    inv[12] = -src[4] * src[9] * src[14] +
        src[4] * src[10] * src[13] +
        src[8] * src[5] * src[14] -
        src[8] * src[6] * src[13] -
        src[12] * src[5] * src[10] +
        src[12] * src[6] * src[9];

    inv[1] = -src[1] * src[10] * src[15] +
        src[1] * src[11] * src[14] +
        src[9] * src[2] * src[15] -
        src[9] * src[3] * src[14] -
        src[13] * src[2] * src[11] +
        src[13] * src[3] * src[10];

    inv[5] = src[0] * src[10] * src[15] -
        src[0] * src[11] * src[14] -
        src[8] * src[2] * src[15] +
        src[8] * src[3] * src[14] +
        src[12] * src[2] * src[11] -
        src[12] * src[3] * src[10];

    inv[9] = -src[0] * src[9] * src[15] +
        src[0] * src[11] * src[13] +
        src[8] * src[1] * src[15] -
        src[8] * src[3] * src[13] -
        src[12] * src[1] * src[11] +
        src[12] * src[3] * src[9];

    inv[13] = src[0] * src[9] * src[14] -
        src[0] * src[10] * src[13] -
        src[8] * src[1] * src[14] +
        src[8] * src[2] * src[13] +
        src[12] * src[1] * src[10] -
        src[12] * src[2] * src[9];

    inv[2] = src[1] * src[6] * src[15] -
        src[1] * src[7] * src[14] -
        src[5] * src[2] * src[15] +
        src[5] * src[3] * src[14] +
        src[13] * src[2] * src[7] -
        src[13] * src[3] * src[6];

    inv[6] = -src[0] * src[6] * src[15] +
        src[0] * src[7] * src[14] +
        src[4] * src[2] * src[15] -
        src[4] * src[3] * src[14] -
        src[12] * src[2] * src[7] +
        src[12] * src[3] * src[6];

    inv[10] = src[0] * src[5] * src[15] -
        src[0] * src[7] * src[13] -
        src[4] * src[1] * src[15] +
        src[4] * src[3] * src[13] +
        src[12] * src[1] * src[7] -
        src[12] * src[3] * src[5];

    inv[14] = -src[0] * src[5] * src[14] +
        src[0] * src[6] * src[13] +
        src[4] * src[1] * src[14] -
        src[4] * src[2] * src[13] -
        src[12] * src[1] * src[6] +
        src[12] * src[2] * src[5];

    inv[3] = -src[1] * src[6] * src[11] +
        src[1] * src[7] * src[10] +
        src[5] * src[2] * src[11] -
        src[5] * src[3] * src[10] -
        src[9] * src[2] * src[7] +
        src[9] * src[3] * src[6];

    inv[7] = src[0] * src[6] * src[11] -
        src[0] * src[7] * src[10] -
        src[4] * src[2] * src[11] +
        src[4] * src[3] * src[10] +
        src[8] * src[2] * src[7] -
        src[8] * src[3] * src[6];

    inv[11] = -src[0] * src[5] * src[11] +
        src[0] * src[7] * src[9] +
        src[4] * src[1] * src[11] -
        src[4] * src[3] * src[9] -
        src[8] * src[1] * src[7] +
        src[8] * src[3] * src[5];

    inv[15] = src[0] * src[5] * src[10] -
        src[0] * src[6] * src[9] -
        src[4] * src[1] * src[10] +
        src[4] * src[2] * src[9] +
        src[8] * src[1] * src[6] -
        src[8] * src[2] * src[5];

    det = src[0] * inv[0] + src[1] * inv[4] + src[2] * inv[8] + src[3] * inv[12];

    det = 1.0f / det;

    Matrix4x4 resultMatrix;
    for (int i = 0; i < 16; ++i)
    {
        ((float*)resultMatrix.m)[i] = inv[i] * det;
    }

    return resultMatrix;
}

//透視投影行列
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip)
{
    Matrix4x4 matrix = {};
    float f = 1.0f / std::tanf(fovY * 0.5f);
    matrix.m[0][0] = f / aspectRatio;
    matrix.m[1][1] = f;
    matrix.m[2][2] = farClip / (farClip - nearClip);
    matrix.m[2][3] = 1.0f;
    matrix.m[3][2] = (-farClip * nearClip) / (farClip - nearClip);
    return matrix;
}

// string->wstring
std::wstring ConvertString(const std::string& str) {
    if (str.empty()) {
        return std::wstring();
    }

    auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
    if (sizeNeeded == 0) {
        return std::wstring();
    }
    std::wstring result(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
    return result;
}

// wstring->string
std::string ConvertString(const std::wstring& str) {
    if (str.empty()) {
        return std::string();
    }

    auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
    if (sizeNeeded == 0) {
        return std::string();
    }
    std::string result(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
    return result;  
}

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

void Log(const std::string& message, std::ostream& os)
{
    os << message << std::endl;
    OutputDebugStringA(message.c_str());
}

void Log(const std::string& message)
{
    OutputDebugStringA(message.c_str());
}

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception)
{
    // 時刻を取得して、時刻を名前に入れたファイルを作成。Dumpsディレクトリ以下に出力
    SYSTEMTIME time;
    GetLocalTime(&time);
    wchar_t filePath[MAX_PATH] = { 0 };
    CreateDirectory(L"./Dumps", nullptr);
    StringCchPrintfW(filePath, MAX_PATH,
        L"./Dumps/%04-d%02d%02d-%02d%02d.dmp",
        time.wYear, time.wMonth, time.wDay,
        time.wHour, time.wMinute);
    HANDLE dumpFileHandle = CreateFile(
        filePath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        0,
        CREATE_ALWAYS,
        0,
        0
    );
    // processId(このexeのId)とクラッシュ(例外)の発生したthreadIdを取得
    DWORD processId = GetCurrentProcessId();
    DWORD threadId = GetCurrentThreadId();
    // 設定情報を入力
    MINIDUMP_EXCEPTION_INFORMATION minidumpInfomation{ 0 };
	minidumpInfomation.ThreadId = threadId;
	minidumpInfomation.ExceptionPointers = exception;
	minidumpInfomation.ClientPointers = TRUE;
    // Dumpを出力。MiniDumpNormalは最低限の情報を出力するフラグ
	MiniDumpWriteDump(
		GetCurrentProcess(),
		processId,
		dumpFileHandle,
		MiniDumpNormal,
		&minidumpInfomation,
		nullptr,
		nullptr
	);
    //他に関連付けられているSEH例外ハンドラがあれば実行。通常はプロセスを終了する
    return EXCEPTION_EXECUTE_HANDLER;
}

// CompileShader関数
IDxcBlob* ConpileShader(
    // CompileするShaderファイルへのパス
    const std::wstring& filePath,
    // Compilerに使用するProfile
    const wchar_t* profile,
    // 初期化で生成したものを3つ
    IDxcUtils* dxcUtils,
    IDxcCompiler3* dxcCompiler,
    IDxcIncludeHandler* includeHandler)
{
    // 1.hlslファイルを読む

    // これからシェーダーをコンパイルする旨をログに出す
    Log(ConvertString(std::format(L"Begin CompileShader, path:{}, profile:{}\n", filePath, profile)));
    // hlslファイルを読む
    IDxcBlobEncoding* shaderSource = nullptr;
    HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
    // 読めなかったら止める
    assert(SUCCEEDED(hr));
    // 読み込んだファイルの内容を設定する
    DxcBuffer shaderSourceBuffer;
    shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
    shaderSourceBuffer.Size = shaderSource->GetBufferSize();
    shaderSourceBuffer.Encoding = DXC_CP_UTF8;// UTF8の文字コードであることを通知

    // 2.Compileする

    LPCWSTR arguments[] =
    {
        filePath.c_str(),
        L"-E", L"main",  // main 関数をエントリーポイントとして指定
        L"-T", profile,  // シェーダープロファイルの指定
        L"-Zi", L"-Qembed_debug",  // デバッグ情報を埋め込むオプション
        L"-Od",  // 最適化を外す
        L"-Zpr"   // メモリレイアウトの指定
    };
    // 実際にShaderをコンパイルする
    IDxcResult* shaderResult = nullptr;
    hr = dxcCompiler->Compile(
        &shaderSourceBuffer,// 読み込んだファイル
        arguments,// コンパイルオプション
        _countof(arguments),// コンパイルオプションの数
        includeHandler,// includeが含まれた諸々
        IID_PPV_ARGS(&shaderResult)// コンパイル結果
    );
    // コンパイルエラーではなくdxcが起動できないなど致命的な状況
    assert(SUCCEEDED(hr));

    // 3.警告・エラーが出ていない

    //警告・エラーが出てたらログを出して止める
    IDxcBlobUtf8* shaderError = nullptr;
    shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
    if (shaderError != nullptr && shaderError->GetStringLength() != 0)
    {
        Log(shaderError->GetStringPointer());
        // 警告・エラーダメゼッタイ
        assert(false);
    }

    // 4.Compile結果を受け取って返す

    // コンパイル結果から実行用のバイナリ部分を取得
    IDxcBlob* shaderBlob = nullptr;
    hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
    assert(SUCCEEDED(hr));
    // 成功したログを出す
    Log(ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}\n", filePath, profile)));
    // もう使わないリソースを開放
    shaderSource->Release();
    shaderResult->Release();
    // 実行用のバイナリを返却
    return shaderBlob;
}

ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes)
{
    // ヒーププロパティの設定 (UPLOADバッファを使用)
    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // バッファ用にUPLOADヒープタイプを使用

    // バッファリソースの設定
    D3D12_RESOURCE_DESC vertexResourceDesc = {};
    vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // バッファリソース
    vertexResourceDesc.Width = sizeInBytes; // サイズを指定
    vertexResourceDesc.Height = 1; // バッファの場合は高さは1
    vertexResourceDesc.DepthOrArraySize = 1; // バッファの場合は1
    vertexResourceDesc.MipLevels = 1; // バッファにはミップマップレベルは不要
    vertexResourceDesc.SampleDesc.Count = 1; // サンプル数は1
    vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // バッファのレイアウト

    // 実際にバッファリソースを作成
    ID3D12Resource* vertexResource = nullptr;
    HRESULT hr = device->CreateCommittedResource(
        &heapProperties, // ヒーププロパティ
        D3D12_HEAP_FLAG_NONE, // ヒープフラグ（特になし）
        &vertexResourceDesc, // リソースの説明
        D3D12_RESOURCE_STATE_GENERIC_READ, // リソースの初期状態
        nullptr, // 詳細設定なし
        IID_PPV_ARGS(&vertexResource) // リソースのポインタを受け取る
    );

    assert(SUCCEEDED(hr));

    // 成功したかどうかを確認
    if (FAILED(hr))
    {
        // エラーハンドリング
        return nullptr;
    }

    return vertexResource;
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) 
{
    // 誰も捕捉しなかった場合に、補足する関数を登録
    SetUnhandledExceptionFilter(ExportDump);

    // ログのディレクトリを用意
    std::filesystem::create_directory("logs");

    // 現在時刻を取得
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> nowSeconds =
    std::chrono::time_point_cast<std::chrono::seconds>(now);
    std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSeconds };

    // formatを使って年月日_時分秒の文字列に変換
    std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
    std::string logFilePath = std::string("logs/") + dateString + ".log";

    // ファイルを作って書き込み準備
    std::ofstream logStream(logFilePath);

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

    // ウィンドウクラスを登録する
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

    // デバッグレイヤー
#ifdef _DEBUG
    ID3D12Debug1* debugController = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        // デバッグレイヤーを有効にする
        debugController->EnableDebugLayer();
        // さらにGPU側でもチェックを行うようにする
        debugController->SetEnableGPUBasedValidation(TRUE);
    }
#endif

    // ウィンドウを表示する
    ShowWindow(hwnd, SW_SHOW);

    // DXGIファクトリーの生成
    IDXGIFactory7* dxgiFactory = nullptr;
    HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
    assert(SUCCEEDED(hr));
    // 使用するアダプタ用の変数。最初にnullptrを入れておく
    IDXGIAdapter4* useAdapter = nullptr;
    // いい順にアダプタを頼む
    for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; i++)
    {
        // アダプタの情報を取得する
        DXGI_ADAPTER_DESC3 adapterDesc{};
        hr = useAdapter->GetDesc3(&adapterDesc);
        assert(SUCCEEDED(hr));
        // ソフトウェアアダプタでなければ採用
        if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
        {
            // 採用したアダプタの情報をログに出力。
            Log(ConvertString(std::format(L"Use Adapter: {}\n", adapterDesc.Description)), logStream);
            break;
        }
        // ソフトウェアアダプタの場合は見なかったことにする
        useAdapter = nullptr;
    }

    ID3D12Device* device = nullptr;
    // 機能レベルとログ出力用の文字列
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
    };
    const char* featureLevelStrings[] =
    {
        "12_2","12_1","12_0"
    };
    // 高い順に生成できるか試していく
    for (size_t i = 0; i < _countof(featureLevels); i++)
    {
        // 採用したアダプターでデバイスを生成
        hr = D3D12CreateDevice(
            useAdapter,
            featureLevels[i],
            IID_PPV_ARGS(&device)
        );
        // 指定した機能レベルでデバイスが生成できたかを確認
        if (SUCCEEDED(hr))
        {
            // 生成できたのでログ出力を行ってループを抜ける
            Log(std::format("Feature Level : {}", featureLevelStrings[i]), logStream);
            break;
        }
    }
    // デバイスの生成がうまくいかなかったので起動できない
    assert(device != nullptr);
    Log("Complete create D3D12Device!!!\n", logStream);// 初期化完了のログを出す

#ifdef _DEBUG
	ID3D12InfoQueue* infoQueue = nullptr;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
	{
		// やばいエラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        // エラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		// 警告時に止まる
	/*	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);*/
        // 抑制するメッセージのID
        D3D12_MESSAGE_ID denyIds[] =
        {
            // Windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用バグによるエラーメッセージ
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
        };
        // 抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
        // 指定したメッセージの表示を抑制する
		infoQueue->PushStorageFilter(&filter);
        // 解放
		infoQueue->Release();
	}
#endif

    // 適切なアダプタが見つからなかったので起動できない
    assert(useAdapter != nullptr);

    // コマンドキューを生成する
    ID3D12CommandQueue* commandQueue = nullptr;
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(
		&commandQueueDesc,
		IID_PPV_ARGS(&commandQueue)
	);
    // コマンドキューの生成がうまくいかなかったら起動できない
	assert(SUCCEEDED(hr));

    // コマンドアロケータを生成する
    ID3D12CommandAllocator* commandAllocator = nullptr;
    hr = device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&commandAllocator)
    );
    // コマンドアロケータの生成がうまくいかなかったので起動できない
    assert(SUCCEEDED(hr));

    // コマンドリストを生成する
    ID3D12GraphicsCommandList* commandList = nullptr;
    hr = device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator,
        nullptr,
        IID_PPV_ARGS(&commandList)
    );
    // コマンドリストの生成がうまくいかなかったので起動できない
    assert(SUCCEEDED(hr));

    // スワップチェーンを生成する
    IDXGISwapChain4* swapChain = nullptr;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.Width = kClientWidth;// 画面の幅。ウィンドウのクライアント領域を同じものにしておく
    swapChainDesc.Height = kClientHeight;// 画面の高さ。ウィンドウのクライアント領域を同じものにしておく
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;// 色の形式
    swapChainDesc.SampleDesc.Count = 1;// マルチサンプルしない
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;// 描画のターゲットとして利用する
    swapChainDesc.BufferCount = 2;// ダブルバッファ
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;// モニタに移したら、中身を破棄
    // ウィンドウハンドル、設定を渡して生成する
    hr = dxgiFactory->CreateSwapChainForHwnd(
        commandQueue,
        hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
		reinterpret_cast<IDXGISwapChain1**>(&swapChain)
    );
    assert(SUCCEEDED(hr));

    // ディスクリプタヒープの生成
    ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc{};
    // レンダーターゲットビュー用
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    // ダブルバッファ用に二つ。
    rtvDescriptorHeapDesc.NumDescriptors = 2;
    hr = device->CreateDescriptorHeap(
        &rtvDescriptorHeapDesc,
        IID_PPV_ARGS(&rtvDescriptorHeap)
    );

    // ディスクリプタヒープが作れなかったので起動できない
    assert(SUCCEEDED(hr));

    // SwapChainからResourceを引っ張ってくる
    ID3D12Resource* swapChainResources[2] = { nullptr };
    hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
    // うまく取得できなけば起動できない
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	// うまく取得できなけば起動できない
    assert(SUCCEEDED(hr));

    // RTVの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    // 出力結果をSRGBに変換して書き込む
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    // 2dテクスチャとして読み込む
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    // ディスクリプタの先頭を取得する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    // RTVを2つ作るのでディスクリプタを2つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
    // まずは一つ目を作る。一つ目は最初のところに作る。作る場所をこちらで指定してあげる必要がある
	rtvHandles[0] = rtvStartHandle;
	device->CreateRenderTargetView(
		swapChainResources[0],
		&rtvDesc,
		rtvHandles[0]
	);
    // 二つ目のディスクリプタハンドルを得る
	rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    // 二つ目を作る
	device->CreateRenderTargetView(swapChainResources[1], &rtvDesc, rtvHandles[1]);

    // TransitionBarrierの設定
	D3D12_RESOURCE_BARRIER barrier{};
    // 今回のバリアはTransition
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    // Noneにしておく
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

    // 初期値0でFenceを作る
    ID3D12Fence* fence = nullptr;
    uint64_t fenceValue = 0;
    hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    assert(SUCCEEDED(hr));

    // FenceのSignalを待つためのイベントを作成する
    HANDLE fenceEvent = CreateEvent(nullptr, false, false, nullptr);
    assert(fenceEvent != nullptr);

    // dxcCompilerを初期化
	IDxcUtils* dxcUtils = nullptr;
	IDxcCompiler3* dxcCompiler = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

    // 現時点でincludeしないが、includeに対応するための設定を行っておく
	IDxcIncludeHandler* includeHandler = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));

    // RootSignature作成
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // RootParameter作成。複数設定できるので配列。今回は結果が1つだけなので長さ1の配列
    D3D12_ROOT_PARAMETER rootParameters[2] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;// CBVを使う
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;// PixelShaderで使う
    rootParameters[0].Descriptor.ShaderRegister = 0;// レジスタ番号0とバインド
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;// CBVを使う
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;// Vertex
    rootParameters[1].Descriptor.ShaderRegister = 0;// レジスタ番号0を使う
    descriptionRootSignature.pParameters = rootParameters;// ルートパラメータ配列へのポインタ
    descriptionRootSignature.NumParameters = _countof(rootParameters);// 配列の長さ

    // シリアライズしてバイナリにする
    ID3DBlob* signatureBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    hr = D3D12SerializeRootSignature(&descriptionRootSignature,
        D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr))
    {
        Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
        assert(false);
    }
    // バイナリを元に生成
    ID3D12RootSignature* rootSignature = nullptr;
    hr = device->CreateRootSignature(0,
        signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature));
    assert(SUCCEEDED(hr));

    // InputLayout
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[1] = {};
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    // BlendStateの設定
    D3D12_BLEND_DESC blendDesc{};
    // 全ての色要素を書き込む
    blendDesc.RenderTarget[0].RenderTargetWriteMask =
        D3D12_COLOR_WRITE_ENABLE_ALL;

    // RasterizerStateの設定
    D3D12_RASTERIZER_DESC rasterizerDesc{};
    // 裏面(時計回り)を表示しない
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    // 三角形の中を塗りつぶす
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

    // Shaderをコンパイルする
    IDxcBlob* vertexShaderBlob = ConpileShader(L"Object3D.VS.hlsl",
        L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
    assert(vertexShaderBlob != nullptr);

    IDxcBlob* pixelShaderBlob = ConpileShader(L"Object3D.PS.hlsl",
        L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
    assert(pixelShaderBlob != nullptr);

    // PSOを生成する
    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature;// RootSignature
    graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;// InputLayout
    graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),
    vertexShaderBlob->GetBufferSize() };// VertexShader
    graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),
    pixelShaderBlob->GetBufferSize() };// PixelShader
    graphicsPipelineStateDesc.BlendState = blendDesc;// BlendState
    graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;// Rasterizer
    // 書き込むRTVの情報
    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    // 利用するとトポロジ(形状)のタイプ。三角形
    graphicsPipelineStateDesc.PrimitiveTopologyType =
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    // どのように画面に打ち込むかの設定(気にしなくていい)
    graphicsPipelineStateDesc.SampleDesc.Count = 1;
    graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    // 実際に生成
    ID3D12PipelineState* graphicsPipelineState = nullptr;
    hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
        IID_PPV_ARGS(&graphicsPipelineState));
    assert(SUCCEEDED(hr));

    //// 頂点リソース用のヒープの設定
    //D3D12_HEAP_PROPERTIES uploadHeapProperties{};
    //uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;// UploadHeapを使う
    //// 頂点リソースの設定
    //D3D12_RESOURCE_DESC vertexResourceDesc{};
    //// バッファリソース。テキスチャの場合はまた別の設定をする
    //vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    //vertexResourceDesc.Width = sizeof(Vector4) * 3;// リソースのサイズ。今回はVector4を3頂点分
    //// バッファの場合はこれらは1にする決まり
    //vertexResourceDesc.Height = 1;
    //vertexResourceDesc.DepthOrArraySize = 1;
    //vertexResourceDesc.MipLevels = 1;
    //vertexResourceDesc.SampleDesc.Count = 1;
    //// バッファの場合はこれにする決まり
    //vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    //// 実際に頂点リソースを作る
    //ID3D12Resource* vertexResource = nullptr;
    //hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
    //    &vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
    //    IID_PPV_ARGS(&vertexResource));
    //assert(SUCCEEDED(hr));

    // vertexResourceの作成
    ID3D12Resource* vertexResource = CreateBufferResource(device, sizeof(Vector4) * 3);
    // マテリアル用のリソースを作る。今回はcolor1つ分のサイズを用意する
    ID3D12Resource* materialResource = CreateBufferResource(device, sizeof(Vector4));
    // マテリアルにデータを書き込む
    Vector4* materialData = nullptr;
    // 書き込むためのアドレスを取得
    materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
    // 今回は赤で書き込んでみる
    *materialData = Vector4(1.0f, 0.0f, 0.0f, 1.0f);

    // WVP用のリソースを作る。Matrix4x4 1つ分のサイズを用意する
    ID3D12Resource* wvpResource = CreateBufferResource(device, sizeof(Matrix4x4));
    // データを書き込む(もともとはwvpDataという変数名だった)
    Matrix4x4* transfomationMatrixData = nullptr;
    // 書き込むためのアドレスを取得
    wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&transfomationMatrixData));
    // 単位行列を書き込んでおく
    *transfomationMatrixData = MakeIdentity4x4();

    // 頂点バッファビューを作成する
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
    // リソースの先頭のアドレスから使う
    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
    // 使用するリソースのサイズは頂点3つ分のサイズ
    vertexBufferView.SizeInBytes = sizeof(Vector4) * 3;
    // 1頂点あたりのサイズ
    vertexBufferView.StrideInBytes = sizeof(Vector4);

    // 頂点リソースにデータを書き込む
    Vector4* vertexData = nullptr;
    // 書き込むためのアドレスを取得
    vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
    // 左下
    vertexData[0] = { -0.5f,-0.5f,0.0f,1.0f };
    // 上
    vertexData[1] = { 0.0f,0.5f,0.0f,1.0f };
    // 右下
    vertexData[2] = { 0.5f,-0.5f,0.0f,1.0f };

    // ビューポート
    D3D12_VIEWPORT viewport{};
    // クライアント領域のサイズと一緒にして画面全体に表示
    viewport.Width = kClientWidth;
    viewport.Height = kClientHeight;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    // シザー矩形
    D3D12_RECT scissorRect{};
    // 基本的にビューポートと同じ矩形が構成されるようにする
    scissorRect.left = 0;
    scissorRect.right = kClientWidth;
    scissorRect.top = 0;
    scissorRect.bottom = kClientHeight;

    // Transform変数を作る
    Transform transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
    Transform cameraTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f}, {0.0f,0.0f,-5.00f} };
	
    float fovY = 0.45f;
    float aspectRatio = 1280.0f / 720.0f;
    float nearClip = 0.1f;
    float farClip = 100.0f;

	// 出力ウィンドウへの文字出力
	OutputDebugStringA("Hello,DirectX!\n");

    // 文字列を格納する
    std::string str0{ "STRING!!" };

    // 整数を文字列にする
    std::string str1{ std::to_string(10) };

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
            
            // これから書き込むバックバッファのインデックスを取得
            UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
            // バリアを張る対象のリソース。現在のバックバッファに対して行う
            barrier.Transition.pResource = swapChainResources[backBufferIndex];
            // 遷移前(現在)のResourceState
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            // 遷移後のResourceState
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            // TransitionBarrierを張る
            commandList->ResourceBarrier(1, &barrier);

            // 描画先のRTVを設定する
            commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, nullptr);
            // 指定した色で画面全体をクリアする
            // 色。RGBAの順
            float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
            commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);

            commandList->RSSetViewports(1, &viewport);// Viewportを設定
            commandList->RSSetScissorRects(1, &scissorRect);// Scissorを設定
            // RootSignatureを設定。PSOに設定しているけど別途設定が必要
            commandList->SetGraphicsRootSignature(rootSignature);
            commandList->SetPipelineState(graphicsPipelineState);// PSOを設定
            commandList->IASetVertexBuffers(0, 1, &vertexBufferView);// VBVを設定
            // 形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけばいい
            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            
            // マテリアルCBufferの場所を設定
            commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

            transform.rotate.y += 0.03f;
            Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
            Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
            Matrix4x4 viewMatrix = Inverse(cameraMatrix);
            Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(fovY, aspectRatio, nearClip, farClip);
            // WVPMatrixを作る
            Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
            *transfomationMatrixData = worldViewProjectionMatrix;

            // wvp用のCBufferの場所を設定
            commandList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());
            
            // 描画。(DrawCall)。3頂点で1つのインスタンス。
            commandList->DrawInstanced(3, 1, 0, 0);


            // 画面に描く処理はすべて終わり、画面に映すので、状態を遷移
            // 今回はRenderTargetからPresentに遷移する
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            // TransitionBarrierを張る
            commandList->ResourceBarrier(1, &barrier);

            // コマンドリストの内容を確定させる。全てのコマンドを積んでからcloseすること
            hr = commandList->Close();
            assert(SUCCEEDED(hr));

            // GPUにコマンドリストの実行を行わせる
            ID3D12CommandList* commandLists[] = { commandList };
            commandQueue->ExecuteCommandLists(1, commandLists);

            // Fenceの値を更新
            fenceValue++;
            // GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようにSignalを送る
            commandQueue->Signal(fence, fenceValue);

            // GPUとOSに画面の交換を行うよう通知する
            swapChain->Present(1, 0);

            // Fenceの値が指定したSignal値にたどり着いているかを確認する
            // GetCompleteValueの初期値はFence作成時に渡した初期値
            if (fence->GetCompletedValue() < fenceValue)
            {
                // 指定したSignalにたどり着いていないので、たどり着くまで待つようにイベントを設定する
                fence->SetEventOnCompletion(fenceValue, fenceEvent);
                // イベントを待つ
                WaitForSingleObject(fenceEvent, INFINITE);
            }

            // 次のフレーム用のコマンドリストを準備
            hr = commandAllocator->Reset();
            assert(SUCCEEDED(hr));
            hr = commandList->Reset(commandAllocator, nullptr);
            assert(SUCCEEDED(hr));


        }
    }

    // Fenceの値を更新
    fenceValue++;
    // GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようにSignalを送る
    commandQueue->Signal(fence, fenceValue);
    // Fenceの値が指定したSignal値にたどり着いているかを確認する
    // GetCompleteValueの初期値はFence作成時に渡した初期値
    if (fence->GetCompletedValue() < fenceValue)
    {
        // 指定したSignalにたどり着いていないので、たどり着くまで待つようにイベントを設定する
        fence->SetEventOnCompletion(fenceValue, fenceEvent);
        // イベントを待つ
        WaitForSingleObject(fenceEvent, INFINITE);
    }

    // 解放処理
	CloseHandle(fenceEvent);
	fence->Release();
	rtvDescriptorHeap->Release();   
	swapChainResources[0]->Release();
	swapChainResources[1]->Release();
	swapChain->Release();
	commandList->Release();
	commandAllocator->Release();
	commandQueue->Release();
	device->Release();
	useAdapter->Release();
	dxgiFactory->Release();
    vertexResource->Release();
    graphicsPipelineState->Release();
    signatureBlob->Release();
    if (errorBlob)
    {
        errorBlob->Release();
    }
    rootSignature->Release();
    pixelShaderBlob->Release();
    vertexShaderBlob->Release();
    materialResource->Release();
   
#ifdef _DEBUG
	debugController->Release();
#endif
	CloseWindow(hwnd);

    // リソースリークチェック
	IDXGIDebug* debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
	{
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		debug->Release();
	}

	return 0;
}