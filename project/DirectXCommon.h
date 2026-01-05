#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h> // ComPtr を使用するために必要	
#include"WinApp.h"

class DirectXCommon{
public:
	void Initialize();
private:
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
	// WindowsAPI
	WinApp* winApp = nullptr;
};

