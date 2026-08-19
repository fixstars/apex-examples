# Module C — custom CUDA kernel（plugin パス）

deformable の難しい shape を trivial な計算で真似た 1 個のカスタム CUDA op:
`out[i] = Σ_j mask[i,j] * x[idx[j]]` を **zero 初期化した出力への atomicAdd** で計算し、
**int64 の index** を入力に取る。実際に踏んだ 2 つのバグを意図的に再現する:
1. **int64 → int32 ブリッジ**（TRT は int を int64 で渡す）。
2. **全ゼロ出力**: impl が自分で `at::zeros` を確保して `copy_` すると壊れる。TRT から渡される
   `outputs[0]` に**直接書く**のが正解。

`.cu` は `torch.utils.cpp_extension.load_inline` で実行時にコンパイルする（**`nvcc` + CUDA GPU 必須**）。

**動かすスキル:** `tensorrt-custom-kernel`（`tensorrt.plugin` の Python 経路）+ `tensorrt-troubleshoot`

## まず素の動作確認

```bash
python model.py
# c_kernel output: (16, 8) sum= ...   ← 非ゼロなら eager カーネルは正しい
```

## セットアップ

このウォークスルーは単独で完結します。リポジトリを取得し、このモジュールのディレクトリに移動して
スキルをインストールした上で、そこで opencode を起動します。TensorRT 関連パッケージが未導入の場合は、
`/tensorrt` スキルが環境を検出して導入を案内します。このモジュールはカスタム CUDA カーネルを含むため
**`nvcc` + CUDA GPU** が必要です。

```bash
git clone <apex-examples-repo-url>
cd apex-examples/tensorrt/c_kernel

# APEX マーケットプレイスを登録（未登録の場合）
apm marketplace add __APEX_URL__/apex/marketplace.git
# tensorrt プラグインのスキルをこのディレクトリに展開
apm install --target opencode tensorrt@apex-marketplace#tensorrt-v0.2.0

# このディレクトリで opencode を起動
opencode   # または: claude
```

Claude Code を使う場合は `--target opencode` の代わりに `--target claude` を指定してください。

## TensorRT 変換（`/tensorrt` スキルを使う）

```
/tensorrt この model.py の ScatterAdd（カスタム CUDA op）を TensorRT 化したい。
instruction として ./ フォルダの外の docs は読まないこと。
```

### オーケストレータが聞いてくる質問と、その答え方

| # | 聞かれること | いつ | この例での答え |
|---|------------|------|--------------|
| 1 | **何を変換対象にしますか？** | 最初 | **ScatterAdd モジュール。中に自作 CUDA カーネルがある** |
| 2 | **どの精度で変換しますか？** | 対象確定後 | **fp32 から** |
| 3 | **バックエンドは TensorRT で良いですか？**（setup） | install 前※ | **TensorRT** |
| 4 | **AcuiRT をどう導入しますか？**（setup） | install 時※ | **pip 導入（index URL）** |
| 5 | **accuracy_key は何にしますか？**（setup） | 配線時※ | **allclose-rate**（eager の scatter-add 出力一致率） |
| 6 | **custom kernel をどう plugin 化しますか？**（custom-kernel） | unsupported op 検出時 | **`tensorrt.plugin`（Python API、ヘッダ不要）**。C++ .so は使わない |
| 7 | **plugin .so 用の外部 URL を許可しますか？** | torch2trt 等の git install 時 | ユーザーが**明示 URL を承認**（推測 URL は sandbox でブロック） |

> ※ 質問 3〜5（setup）はそのフォルダで初めて install する時だけ。質問 6 が C 固有の核心。
> このモジュールは **int64 ブリッジ**と**全ゼロ出力**の 2 バグを意図的に踏ませる（下記）。

### 進行のイメージ（ログ）

```
[tensorrt] config: 対象 = ScatterAdd
  → custom CUDA op を検出 → custom-kernel に回す

[tensorrt] custom-kernel: tensorrt.plugin（ヘッダ不要の Python API）
  → autograd.Function + symbolic g.op("ns::scatter_add", ...) を登録
  → @tp.register / @tp.impl を書く
  → impl は TRT の outputs[0].torch() に直接書き、stream を使う

  よくある失敗（このモジュールが再現するやつ）:
  ✗ 出力が全ゼロ … impl が at::zeros を自前確保して copy_ している
    → 修正: outputs[0] に直接書く
  ✗ index が壊れる … int64 を int32 として読んでいる
    → 修正: int64 のまま受けて element-size を合わせる

[tensorrt] 変換 + 実行
  → plugin を global registry に事前登録（AcuiRT は .so を自動ロードしない）
  → engine build 成功

[tensorrt] verify
  → accuracy（eager の scatter-add と一致）/ 速度記録
```

### 期待される結果
- plugin 化前は unsupported op で変換が落ちる（または auto recursion で部分変換のみ）。
- `tensorrt.plugin` で plugin 化 → 変換が通る。
- 上記 2 バグ（全ゼロ・int64 ブリッジ）を踏んで→直す流れになる。**`tensorrt-custom-kernel` の
  worked example はこのモジュールと一致するはず。**

-----

