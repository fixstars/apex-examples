# Module D — refactor が要るモジュール（参照透過性）

純粋な DAG ではない stateful モジュール: `self.state` を持ち、初回呼び出しを `state is None` で
分岐し、呼び出しをまたいで `self.state` を破壊的に更新する。`tensorrt-refactor` の
`referential-transparency.md` の `NoTransparencyClass` そのもの。直接 ONNX export は内部 state と
分岐のせいで失敗 / 非決定的になる。直し方は **state を明示的な I/O に持ち上げる**
（`forward(x, state) -> (y, new_state)`、初回は 0 初期化）→ 純 DAG にして export 可能にする。

**動かすスキル:** `tensorrt-refactor` + `tensorrt-troubleshoot`（export 失敗）

## まず素の動作確認

```bash
python model.py
# d_stateful equivalence (first frame): True
#   ← NoTransparencyClass の初回呼び出し == TransparencyClass(x, zeros) を確認
```

## プラグインのインストール

[セットアップとプラグインのインストール](../README.md#installation) を参照。
**共通 venv とスキルは root で一度だけ** install 済みの前提（`../.venv` を共有）。

## TensorRT 変換（`/tensorrt` スキルを使う）

```
/tensorrt この model.py のモジュールを TensorRT 化したい。
環境は ../.venv を使うこと。instruction として ./ フォルダの外の docs は読まないこと。
```

### オーケストレータが聞いてくる質問と、その答え方

| # | 聞かれること | いつ | この例での答え |
|---|------------|------|--------------|
| 1 | **何を変換対象にしますか？** | 最初 | **NoTransparencyClass。内部 state を持っている** |
| 2 | **どの精度で変換しますか？** | 対象確定後 | **fp32 から** |
| 3 | **バックエンドは TensorRT で良いですか？**（setup） | install 前※ | **TensorRT** |
| 4 | **AcuiRT をどう導入しますか？**（setup） | install 時※ | **clone して editable 導入** |
| 5 | **accuracy_key は何にしますか？**（setup） | 配線時※ | **allclose-rate**（0 初期化フレームで eager 一致率） |
| 6 | **state をどう扱いますか？**（refactor） | 内部 state + 分岐を検出時 | **state を明示 I/O に持ち上げる**（`forward(x,state)->(y,new_state)`、初回 0 初期化） |
| 7 | **初期値は 0 で良いですか？**（refactor） | 持ち上げ提案時 | **0 で良い**（`torch.zeros` を初回入力にする） |

> ※ 質問 3〜5（setup）はそのフォルダで初めて install する時だけ。質問 6・7 が D 固有の核心。
> `model.py` のクラス定義は**勝手に書き換えない** — 配線は評価スクリプト側で行う。

### 進行のイメージ（ログ）

```
[tensorrt] config: 対象 = NoTransparencyClass
  → 内部 state（self.state）と state is None 分岐を検出
  → 純 DAG ではない → refactor に回す（変換不能のまま config は書かない）

[tensorrt] refactor: 参照透過性
  → 提案: state を明示 I/O に持ち上げる
      forward(x) で self.state を読み書き
    → forward(x, state) -> (fc(state+x), state+x)
  → 初期値は 0 で良いか確認（OK なら 0 初期化を入力する）
  → host 側の state 管理は eager のまま、純粋な compute コアだけ export
  → 承認 → 評価スクリプト側で配線（model.py のクラス定義は勝手に書き換えない）

[tensorrt] 変換 + 実行
  → TransparencyClass を export → engine build 成功

[tensorrt] verify
  → 0 初期化フレームで eager と一致 / 速度記録
```

### 期待される結果
- リファクタ前は **export 失敗 / 非決定的**（内部 state + 分岐）。
- state を明示 I/O にして 0 初期化 → 純 DAG → クリーンに export。
- 「state は明示 I/O のまま / host 側マネージャは eager のまま / 純粋 compute コアだけ export」
  の教訓（skill feedback F1）を教える。`NoTransparencyClass` / `TransparencyClass` のペアは
  スキルからそのまま持ってきているので、refactor の worked example をこのモジュールで検証できる。
