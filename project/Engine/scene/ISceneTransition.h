#pragma once

/// <summary>
/// シーン遷移演出の差し込み口。ゲームがこれを継承した演出（タイル・グリッチなど）を作り、
/// SceneManager::SetTransition で渡すと、SceneManager が「暗転 → シーン差し替え → 明転」の流れの中で呼んでくれる。
/// SceneManager は演出を所有しない（ゲーム側が生きている間は有効なまま持っておくこと）。
/// 演出の設定（色・時間など）はエンジンが知る必要がないので、継承した側の関数で渡す（Start には引数を持たせない）。
///
/// 流れ:
///   Start()  → 暗転（IsFadingOut の間はシーンの更新が止まる）→ 真っ暗で IsReadyToSwitch が true
///   → SceneManager がシーンを差し替え → BeginFadeIn() → 明転 → IsActive が false で終わり
///   StartFromBlack() は起動直後の最初のシーン用: 真っ暗な状態から明転だけ行う
/// </summary>
class ISceneTransition {
public:
    virtual ~ISceneTransition() = default;

    // 暗転開始
    virtual void Start() = 0;
    // 真っ暗な状態から明転開始（起動直後の最初のシーン用）
    virtual void StartFromBlack() = 0;
    // 暗転完了後、シーンの差し替えを済ませてから SceneManager が呼ぶ。明転へ進める
    virtual void BeginFadeIn() = 0;

    // SceneManager::Update が毎フレーム呼ぶ（時間は Time::GetUnscaledDeltaTime で進める。ポーズ中でも進めるため）
    virtual void Update() = 0;
    // SceneManager::Draw が、シーンと UI を描いた後に呼ぶ（スプライトを積むだけ。描画は SceneManager が Flush する）
    virtual void Draw() = 0;

    // 遷移演出中か（暗転・停止・明転のどれか）
    virtual bool IsActive() const = 0;
    // 暗転中か（暗転・停止）。この間はシーンの更新と入力を止める
    virtual bool IsFadingOut() const = 0;
    // 真っ暗になり、シーンを差し替えるべきフレームか
    virtual bool IsReadyToSwitch() const = 0;
};
