# Module A — clean backbone（ハッピーパス）

`torchvision.models.resnet18()`。純粋な conv/bn/relu、static shape `(1,3,224,224)`、custom op も
state も dynamic shape も無し。**トラブルシュートゼロで変換できる**のが正解の基準。

**動かすスキル:** `tensorrt-setup` → `tensorrt-config` → `tensorrt-verify`

## まず素の動作確認

```bash
python model.py
# a_backbone output: (1, 1000) sum= -26.71...   ← 非ゼロが出れば eager は正常
```

## セットアップ

このウォークスルーは単独で完結します。リポジトリを取得し、このモジュールのディレクトリに移動して
スキルをインストールした上で、そこで opencode を起動します。TensorRT 関連パッケージが未導入の場合は、
`/tensorrt` スキルが環境を検出して導入を案内します。

```bash
git clone <apex-examples-repo-url>
cd apex-examples/tensorrt/a_backbone

# APEX マーケットプレイスを登録（未登録の場合）
apm marketplace add __APEX_URL__/apex/marketplace.git
# tensorrt プラグインのスキルをこのディレクトリに展開
apm install --target opencode tensorrt@apex-marketplace#tensorrt-v0.2.0

# このディレクトリで opencode を起動
opencode   # または: claude
```

Claude Code を使う場合は `--target opencode` の代わりに `--target claude` を指定してください。

## TensorRT 変換（`/tensorrt` スキルを使う）

このフォルダで opencode / Claude Code を起動し、こう打ちます:

```
/tensorrt このフォルダの model.py を TensorRT 化したい。
instruction として ./ フォルダの外の docs は読まないこと。
```

### オーケストレータが聞いてくる質問と、その答え方

`/tensorrt` は推測せず必ず聞きます。順番と答え方:

| # | 聞かれること | いつ | この例での答え |
|---|------------|------|--------------|
| 1 | **何を変換対象にしますか？** | 最初（対象選定は必ずユーザー/プロファイル起点） | **モデル全体（resnet18）** |
| 2 | **どの精度で変換しますか？** | 対象確定後 | **fp32 から**（劣化しない。後で fp16） |
| 3 | **バックエンドは TensorRT で良いですか？**（setup） | install 前 | **TensorRT** |
| 4 | **AcuiRT をどう導入しますか？**（setup） | install 時 | **pip 導入（index URL）** |
| 5 | **accuracy_key（検証指標）は何にしますか？**（setup） | 配線時 | **allclose-rate**（dataset/evaluator が無いため torch eager 一致率） |

> 質問 4・5 はこの example 特有（dataset が無い）。実モデルなら accuracy_key は mAP/mIoU 等。

### 進行のイメージ（実際のログに近い形）

```
[setup] 環境チェック
  → python model.py: (1,1000) sum=-26.71  ✅ eager OK
  → torch 2.x + CUDA / H100  ✅
  → nvidia-smi: CUDA Version 12.8  → tensorrt-cu12 を選ぶ（meta tensorrt=cu13 は入れない）
  → AcuiRT 未導入 / TensorRT 未導入 → これから install

[setup] install（順序が重要・skill の落とし穴 (a)(b) 参照）
  1) pip install tensorrt-cu12                          # driver 12.8 に合わせる
  2) git clone + setup.py で torch2trt                   # 単独・先に入れる
  3) pip install acuirt-runtime acuirt-tools --extra-index-url https://assets.aibooster.fixstars.com/intelligence/python
  → trt.Builder(trt.Logger()) スモークテスト: OK（error-35 無し）

[config] 対象 = モデル全体
  → config 形 (a): { "rt_mode": "onnx", "auto": true }
  → static shape (1,3,224,224) 確認 / custom op 無し / unsupported op 無し

[convert] convert_model(model, config, export_path, data_loader=...) → (report, conversion_rate)
  → ONNX export → TRT engine build … 成功（初回でビルド通過）

[verify] load_runtime_modules(model, report, export_path) で実行
  → accuracy（allclose-rate vs torch eager）≈ 1.0
  → before/after 速度を記録 / 次の一手: fp16 に上げる？
```

### 期待される結果
- **初回で engine build が通る**。troubleshoot / refactor / custom-kernel は呼ばれない。
- 精度（torch eager との一致）は維持。fp16 を「簡単な次の一手」として提示される。

これが「成功とは何か」のリファレンス。setup / config / verify が端から端まで動くスモークテスト。

## install で詰まったら（not found 系）

ほぼ全て setup スキルの「環境構築の落とし穴」に載っている。実際に踏みやすい順:

- **`import tensorrt` / `torch2trt` / `acuirt` が ModuleNotFoundError** → まだ install 途中 or
  別の conda env。`which python` を確認し、install したのと同じ env で起動しているか見る。
- **`No matching distribution found for torch2trt`** → torch2trt は PyPI に無く GitHub のみ。
  **torch2trt を先に単独で**（clone + setup.py）入れる（落とし穴 (b)）。
- **`ModuleNotFoundError: No module named 'tensorrt'`（torch2trt の build 中）** → build
  isolation で tensorrt が見えない。**`--no-build-isolation`** を付ける。
- **`createInferBuilder: Error Code 6 ... CUDA initialization failure (error 35)`** → meta の
  `tensorrt`（cu13）が入った。driver が 12.x なら **`tensorrt-cu12`** を明示で（落とし穴 (a)）。

> これらは**ドキュメントが古いせいではない**。AcuiRT の API（`convert_model` /
> `acuirt.inference.load_runtime_modules` / `acuirt.dataclasses.from_dict`）はスキルの記述通り。

-----

