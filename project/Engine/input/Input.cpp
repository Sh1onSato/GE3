#include "Input.h"
#include <cassert>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")


void Input::Initialize(WinApp* winApp){
	this->winApp = winApp;
	// DirectInputの初期化
	hr = DirectInput8Create(
		winApp->GetHinstance(),
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		reinterpret_cast<void**>(directInput_.GetAddressOf()),
		nullptr
	);
	assert(SUCCEEDED(hr));
	// 入力データ形式乗セット
	hr = directInput_->CreateDevice(
		GUID_SysKeyboard,
		keyboard.GetAddressOf(),
		NULL
	);
	assert(SUCCEEDED(hr));
	// 入力データ形式の設定
	hr = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(hr));
	// 協調レベルの設定
	hr = keyboard->SetCooperativeLevel(winApp->GetHwnd(),
		DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY
	);
	assert(SUCCEEDED(hr));
}


void Input::Update(){
	// キーボードの状態を取得する
	memcpy(keyPre, key, sizeof(key));

	keyboard->Acquire();

	keyboard->GetDeviceState(sizeof(key), key);
}

bool Input::PushKey(BYTE keyNumber){
	if(key[keyNumber]){
		return true;
	}

	return false;
}

bool Input::TriggerKey(BYTE keyNumber){
	if(!keyPre[keyNumber] && key[keyNumber]){
		return true;
	}
	return false;
}
