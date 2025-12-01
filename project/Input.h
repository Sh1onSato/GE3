#pragma once
#include <Windows.h>
#include <dinput.h>
#include <wrl.h> // ComPtr を使用するために必要

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

class Input {
public:
    // コンストラクタ
    Input();
    // デストラクタ
    ~Input();

    // 初期化処理
    void Initialize();
    // 更新処理
    void Update();

private:
    Microsoft::WRL::ComPtr<IDirectInput8> directInput_; // DirectInputオブジェクト
    Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard_; // キーボードデバイス
    HRESULT hr;
    BYTE keyState_[256];     // 現在のキー状態
    BYTE preKeyState_[256];  // 1フレーム前のキー状態
};

