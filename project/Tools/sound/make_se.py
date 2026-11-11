# 効果音（SE）を合成して .wav に書き出す
#
# 実行（Blender に付いている Python を使う。標準ライブラリだけで動く）:
#   "C:\Program Files\Blender Foundation\Blender 5.1\5.1\python\bin\python.exe" Tools/sound/make_se.py [出力フォルダ]
#   出力フォルダを省略すると Resources/sounds/ に書き出す。
#
# 形式: 44.1kHz・16bit・モノラルの PCM（エンジンの Audio::SoundLoadWave が読める、拡張なしの WAVEFORMATEX）。
# 音作りの方針: 世界観（電子世界の仮想アリーナ）に合わせて、実在の銃の録音ではなく
#   「低い衝撃＋ノイズ＋少しデジタルな電子音」を重ねて作る。乱数は固定（SEED）なので、何度実行しても同じ音になる。

import math
import os
import random
import struct
import sys
import wave

SAMPLE_RATE = 44100
SEED = 1234


# ---------------------------------------------------------------------------
# 基本の部品
# ---------------------------------------------------------------------------
def silence(seconds):
    return [0.0] * int(SAMPLE_RATE * seconds)


def mix_into(dst, src, start_seconds=0.0, gain=1.0):
    """src を dst の start_seconds の位置から足し込む（はみ出したら dst を伸ばす）"""
    offset = int(start_seconds * SAMPLE_RATE)
    need = offset + len(src)
    if need > len(dst):
        dst.extend([0.0] * (need - len(dst)))
    for i, v in enumerate(src):
        dst[offset + i] += v * gain
    return dst


def exp_env(n, tau, attack=0.002):
    """立ち上がり attack 秒、そのあと時定数 tau 秒で指数的に減衰するエンベロープ"""
    a = max(1, int(attack * SAMPLE_RATE))
    out = []
    for i in range(n):
        t = i / SAMPLE_RATE
        rise = min(1.0, i / a)
        out.append(rise * math.exp(-t / tau))
    return out


def noise(seconds, rng):
    return [rng.uniform(-1.0, 1.0) for _ in range(int(SAMPLE_RATE * seconds))]


def lowpass(samples, cutoff_start, cutoff_end=None):
    """1次のローパス。カットオフを cutoff_start→cutoff_end へ直線的に動かせる"""
    if cutoff_end is None:
        cutoff_end = cutoff_start
    out = []
    y = 0.0
    n = len(samples)
    for i, x in enumerate(samples):
        fc = cutoff_start + (cutoff_end - cutoff_start) * (i / max(1, n - 1))
        alpha = 1.0 - math.exp(-2.0 * math.pi * fc / SAMPLE_RATE)
        y += alpha * (x - y)
        out.append(y)
    return out


def highpass(samples, cutoff):
    low = lowpass(samples, cutoff)
    return [x - l for x, l in zip(samples, low)]


def sweep_sine(seconds, f_start, f_end, curve=1.0):
    """周波数を f_start→f_end へ動かすサイン波（curve>1 で最初に速く下がる）"""
    n = int(SAMPLE_RATE * seconds)
    out = []
    phase = 0.0
    for i in range(n):
        p = (i / max(1, n - 1)) ** (1.0 / curve)
        f = f_start + (f_end - f_start) * p
        phase += 2.0 * math.pi * f / SAMPLE_RATE
        out.append(math.sin(phase))
    return out


def square(seconds, freq_start, freq_end=None):
    if freq_end is None:
        freq_end = freq_start
    n = int(SAMPLE_RATE * seconds)
    out = []
    phase = 0.0
    for i in range(n):
        f = freq_start + (freq_end - freq_start) * (i / max(1, n - 1))
        phase += f / SAMPLE_RATE
        out.append(1.0 if (phase % 1.0) < 0.5 else -1.0)
    return out


def bitcrush(samples, steps=16, hold=4):
    """量子化（段数を落とす）＋サンプルの間引きで、デジタルっぽいザラつきを足す"""
    out = []
    held = 0.0
    for i, x in enumerate(samples):
        if i % hold == 0:
            held = round(x * steps) / steps
        out.append(held)
    return out


def apply_env(samples, env):
    return [s * e for s, e in zip(samples, env)]


def saturate(samples, drive=1.5):
    return [math.tanh(s * drive) for s in samples]


def bandpass(samples, low_cut, high_cut):
    return lowpass(highpass(samples, low_cut), high_cut)


def reverb(samples, decay_seconds=0.9, wet=0.25, tail_seconds=None):
    """簡単なリバーブ（並列のコムフィルタ4本＋直列のオールパス2本、Schroeder 型）。
    広いアリーナに響く余韻を足して、音が「ブツッ」と切れるおもちゃっぽさを消す"""
    if tail_seconds is None:
        tail_seconds = decay_seconds
    src = list(samples) + [0.0] * int(tail_seconds * SAMPLE_RATE)
    n = len(src)
    wet_sum = [0.0] * n
    for delay_ms in (29.7, 37.1, 41.1, 43.7):
        d = int(delay_ms * 0.001 * SAMPLE_RATE)
        g = 10.0 ** (-3.0 * (delay_ms * 0.001) / decay_seconds)  # decay_seconds で -60dB になる帰還量
        buf = [0.0] * n
        lp = 0.0
        for i in range(n):
            fb = buf[i - d] if i >= d else 0.0
            lp += 0.4 * (fb - lp)  # 帰還を少しこもらせて、高域が残りすぎないようにする
            buf[i] = src[i] + g * lp
            wet_sum[i] += buf[i] * 0.25
    for delay_ms, g in ((5.0, 0.7), (1.7, 0.7)):
        d = int(delay_ms * 0.001 * SAMPLE_RATE)
        out = [0.0] * n
        for i in range(n):
            x_d = wet_sum[i - d] if i >= d else 0.0
            y_d = out[i - d] if i >= d else 0.0
            out[i] = -g * wet_sum[i] + x_d + g * y_d
        wet_sum = out
    return [s + w * wet for s, w in zip(src, wet_sum)]


def write_wav(path, samples, peak=0.89):
    """ピークを peak にそろえて 16bit で書き出す（音量の調整はゲーム側の volume で行う）"""
    m = max(1e-9, max(abs(s) for s in samples))
    scale = peak / m
    # 終わりのプツッというノイズを防ぐため、最後の 5ms をフェードアウトする
    fade = int(0.005 * SAMPLE_RATE)
    data = bytearray()
    n = len(samples)
    for i, s in enumerate(samples):
        g = 1.0 if i < n - fade else (n - i) / fade
        v = int(max(-1.0, min(1.0, s * scale * g)) * 32767)
        data += struct.pack("<h", v)
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SAMPLE_RATE)
        w.writeframes(bytes(data))
    print(f"[se] {path}  ({n / SAMPLE_RATE:.2f}s)")


# ---------------------------------------------------------------------------
# 個々の効果音
# ---------------------------------------------------------------------------
def make_shotgun_fire(rng):
    """ショットガンの発射音（重め）：
    サブベース（下がる低いサイン）＋中低域の「ドン」＋明るい→こもるノイズ＋破裂の瞬間を重ね、強く歪ませてから
    アリーナに響く余韻（リバーブ）を付ける。音程の目立つ電子音は入れない（おもちゃっぽくなるため）"""
    # 「まだ軽い」とのことで低域を大幅に足し、高域を抑えた（2026/11/11）
    length = 1.2
    out = silence(length)
    n = len(out)
    # サブベース（体に来る低い唸り）：より低く・長く・大きく
    sub = apply_env(sweep_sine(length, 80.0, 26.0, curve=2.0), exp_env(n, 0.28, attack=0.001))
    mix_into(out, sub, 0.0, 1.6)
    # 低い胴鳴り（ドーン）：60〜250Hz のノイズを長めに
    boom = apply_env(bandpass(noise(length, rng), 60.0, 250.0), exp_env(n, 0.22, attack=0.002))
    mix_into(out, boom, 0.0, 2.2)
    # 中低域の打撃（120〜700Hz に絞って短く）
    punch = apply_env(bandpass(noise(length, rng), 120.0, 700.0), exp_env(n, 0.06, attack=0.0005))
    mix_into(out, punch, 0.0, 1.4)
    # ノイズの本体（明るさを抑え、早めにこもらせる）
    for cut_start, cut_end, tau, gain in ((3500.0, 250.0, 0.14, 0.6), (1800.0, 150.0, 0.3, 0.6)):
        body = apply_env(lowpass(noise(length, rng), cut_start, cut_end), exp_env(n, tau, attack=0.001))
        mix_into(out, body, 0.0, gain)
    # 破裂の瞬間（控えめに。高すぎる成分は軽く聞こえるので 1.5〜5kHz）
    crack = apply_env(bandpass(noise(0.03, rng), 1500.0, 5000.0), exp_env(int(0.03 * SAMPLE_RATE), 0.007, attack=0.0003))
    mix_into(out, crack, 0.0, 0.5)
    # 少し遅れて2発目の低い押し（音の「重さ」を長く感じさせる）
    push = apply_env(sweep_sine(0.5, 55.0, 30.0, curve=1.5), exp_env(int(0.5 * SAMPLE_RATE), 0.15, attack=0.01))
    mix_into(out, push, 0.03, 0.8)
    dry = saturate(out, 3.5)
    # 余韻は暗く長く（高域の多い響きは軽く聞こえるため、最後に全体を少しこもらせる）
    wet = reverb(dry, decay_seconds=1.6, wet=0.4, tail_seconds=0.8)
    return saturate(lowpass(wet, 9000.0), 1.3)


def click(rng, tone_hz, seconds=0.05):
    """金属的なカチッ：短いノイズを帯域で絞ったもの（音程の目立つサインは使わない）"""
    n = int(seconds * SAMPLE_RATE)
    hit = apply_env(bandpass(noise(seconds, rng), tone_hz * 0.6, tone_hz * 3.0), exp_env(n, 0.007, attack=0.0003))
    body = apply_env(bandpass(noise(seconds, rng), 300.0, 1200.0), exp_env(n, 0.012, attack=0.0005))
    return [h * 0.9 + b * 0.6 for h, b in zip(hit, body)]


def make_shotgun_pump(rng):
    """ポンプの音。ゲームの先台の動き（0.1秒後に引き始め0.25秒で引き切り、0.4秒で戻る）に合わせて、
    発射と同時に鳴らすだけで合うように、無音の間を含めて作る"""
    out = silence(0.5)
    mix_into(out, click(rng, 1800.0), 0.10, 0.9)                                   # 引き始めのカチ
    slide = apply_env(lowpass(noise(0.14, rng), 2500.0), exp_env(int(0.14 * SAMPLE_RATE), 0.08))
    mix_into(out, slide, 0.11, 0.25)                                               # スライドのシャッ
    mix_into(out, click(rng, 1500.0), 0.24, 0.7)                                   # 引き切ったところ
    mix_into(out, click(rng, 2400.0, 0.06), 0.38, 1.0)                             # 戻して閉じるガチャ
    return reverb(out, decay_seconds=0.35, wet=0.15, tail_seconds=0.15)


def make_hit_wall(rng):
    """壁・床への着弾：硬い面に当たる「ガッ」（低めの打撃＋砕けるノイズ）に短い響き"""
    length = 0.2
    n = int(length * SAMPLE_RATE)
    thud = apply_env(bandpass(noise(length, rng), 150.0, 1000.0), exp_env(n, 0.02, attack=0.0003))
    debris = apply_env(bandpass(noise(length, rng), 1500.0, 8000.0), exp_env(n, 0.035, attack=0.0005))
    dry = saturate([t * 1.2 + d * 0.6 for t, d in zip(thud, debris)], 1.8)
    return reverb(dry, decay_seconds=0.6, wet=0.25, tail_seconds=0.3)


def heavy_impact(rng, length, low_hz):
    """重い物に当たる打撃：低い唸り（短いサブ）＋中低域のノイズ＋金属っぽい不協和の倍音（低め）"""
    n = int(length * SAMPLE_RATE)
    out = [0.0] * n
    sub = apply_env(sweep_sine(length, low_hz * 1.6, low_hz, curve=2.0), exp_env(n, 0.06, attack=0.0005))
    mid = apply_env(bandpass(noise(length, rng), 200.0, 2500.0), exp_env(n, 0.03, attack=0.0003))
    for i in range(n):
        out[i] = sub[i] * 0.9 + mid[i] * 1.2
    # 鈍い金属の響き（倍音を整数倍にしないと、楽器っぽい「音程」にならず物っぽくなる）
    for freq, amp, tau in ((310.0, 0.35, 0.08), (527.0, 0.25, 0.06), (893.0, 0.15, 0.04)):
        tone = apply_env(sweep_sine(length, freq, freq * 0.97), exp_env(n, tau, attack=0.0008))
        for i in range(n):
            out[i] += tone[i] * amp
    return saturate(out, 2.2)


def make_hit_boss(rng):
    """ボスへの命中：重い金属の塊に当たる「ゴッ」に、アリーナの響き"""
    dry = heavy_impact(rng, 0.3, 70.0)
    return reverb(dry, decay_seconds=0.8, wet=0.3, tail_seconds=0.4)


def make_hit_core(rng):
    """コアへの命中：ボスへの命中より鋭く重い打撃に、割れるような高い成分を重ねる（「効いた」と分かる音）。
    澄んだチャイムはおもちゃっぽいので使わない"""
    length = 0.35
    n = int(length * SAMPLE_RATE)
    out = silence(length)
    mix_into(out, heavy_impact(rng, length, 85.0), 0.0, 1.0)
    shatter = apply_env(bandpass(noise(length, rng), 3000.0, 11000.0), exp_env(n, 0.05, attack=0.0005))
    mix_into(out, shatter, 0.0, 0.7)
    # 少し遅れて2発目の打撃（「ガ、ン」と重なる感じ）
    mix_into(out, heavy_impact(rng, 0.2, 110.0), 0.025, 0.6)
    return reverb(saturate(out, 1.5), decay_seconds=0.9, wet=0.35, tail_seconds=0.45)


def make_shield_block(rng):
    """シールドで弾かれた音：金属的なキン（倍音が整数倍でない高いサイン）＋少し下がる"""
    length = 0.35
    n = int(length * SAMPLE_RATE)
    partials = [(2100.0, 1.0, 0.08), (3420.0, 0.6, 0.05), (5230.0, 0.35, 0.03)]
    out = [0.0] * n
    for freq, amp, tau in partials:
        tone = apply_env(sweep_sine(length, freq, freq * 0.96), exp_env(n, tau, attack=0.0005))
        for i in range(n):
            out[i] += tone[i] * amp
    tick = apply_env(highpass(noise(0.02, rng), 4000.0), exp_env(int(0.02 * SAMPLE_RATE), 0.004, attack=0.0003))
    mix_into(out, tick, 0.0, 0.5)
    return reverb(out, decay_seconds=0.7, wet=0.3, tail_seconds=0.35)


SOUNDS = {
    "shotgun_fire": make_shotgun_fire,
    "shotgun_pump": make_shotgun_pump,
    "hit_wall": make_hit_wall,
    "hit_boss": make_hit_boss,
    "hit_core": make_hit_core,
    "shield_block": make_shield_block,
}


def main():
    project_root = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
    out_dir = os.path.abspath(sys.argv[1]) if len(sys.argv) > 1 else os.path.join(project_root, "Resources", "sounds")
    os.makedirs(out_dir, exist_ok=True)
    for name, func in SOUNDS.items():
        rng = random.Random(f"{SEED}-{name}")  # 音ごとに固定の乱数（ほかの音を足しても変わらない）
        write_wav(os.path.join(out_dir, name + ".wav"), func(rng))


main()
