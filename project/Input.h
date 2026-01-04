#pragma once
#include <Windows.h>
#include <wrl.h> // ComPtr を使用するために必要
#include"WinApp.h"

#define  DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

// ComPtr を簡単に使うために名前空間をusing
using namespace Microsoft::WRL;

class Input {
public:
    // 初期化処理
    void Initialize(WinApp* winApp);
    // 更新処理
    void Update();

   /// <summary>
   /// キーの押下をチェック
   /// </summary>
   /// <param name="keyNumber">キー番号 ( DIK_0 等 )</param>
   /// <returns>押されていればtrue</returns>
    bool PushKey(BYTE keyNumber);

    /// <summary>
	/// キーのトリガーをチェック
	/// </summary>
	/// <param name="keyNumber">キー番号 ( DIK_0 等 )</param>
	/// <returns>押された瞬間ならtrue</returns>
	bool TriggerKey(BYTE keyNumber);


private:
    HRESULT hr;

    ComPtr<IDirectInputDevice8> keyboard; // キーボードデバイス
    BYTE key[256] = {};
	BYTE keyPre[256] = {};
    
    ComPtr<IDirectInput8> directInput_;
    WinApp* winApp = nullptr;

};

