#include "ModelManager.h"

ModelManager* ModelManager::GetInstance() {
	static ModelManager instance;
	return &instance;
}

void ModelManager::Initialize(DirectXCommon* dxCommon) {
	this->dxCommon = dxCommon;
}

void ModelManager::Finalize() {
	models.clear();
}

Model* ModelManager::LoadModel(const std::string& directoryPath, const std::string& filename) {
	// すでにロード済みかチェック
	std::string filePath = directoryPath + "/" + filename;
	if (models.contains(filePath)) {
		return models.at(filePath).get();
	}

	// 新しく生成して初期化
	std::unique_ptr<Model> model = std::make_unique<Model>();
	model->Initialize(dxCommon, directoryPath, filename);

	// キャッシュに保存して返す
	models[filePath] = std::move(model);
	return models.at(filePath).get();
}
