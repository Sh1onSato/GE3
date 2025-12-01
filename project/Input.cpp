#include "Input.h"
#include <cassert>
#include<wrl.h> // ComPtr を使用するために必要
// ComPtr を簡単に使うために名前空間をusing
using namespace Microsoft::WRL;

void Input::Initialize(){
	ComPtr<IDirectInput8> directInput_;
	hr = DirectInput8Create(
		GetModuleHandle(nullptr),
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		reinterpret_cast<void**>(directInput_.GetAddressOf()),
		nullptr
	);
	assert(SUCCEEDED(hr));
	// 入力データ形式乗セット
	hr = directInput_->CreateDevice(
		GUID_SysKeyboard,
		keyboard_.GetAddressOf(),
		NULL
	);


}


void Input::Update(){

}
