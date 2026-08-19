# Module B — dynamic-shape module（steady-state が直し方）

同一の attention ブロックが、warmup で kv 長 1→K と伸び、その後 K で安定する。**同じモジュールが
多数の shape で呼ばれる**ので、全フレームで素直に変換しようとすると dynamic-shape パスで詰まる。
直し方は **steady-state（step ≥ K）フレームだけで変換する**こと。SparseDrive の instance-queue
が埋まる挙動の縮小版。

**動かすスキル:** `tensorrt-config`（shape 検証）+ `tensorrt-troubleshoot`（dynamic-shape 分類）

## まず素の動作確認

```bash
python model.py
# frame  0 kv_len= 1 [warmup] -> (1, 1, 64)
# ...
# frame  7 kv_len= 8 [steady] -> (1, 1, 64)   ← step >= K=8 から steady
# ...
```

## プラグインのインストール

[セットアップとプラグインのインストール](../README.md#installation) を参照。
**共通 venv とスキルは root で一度だけ** install 済みの前提（`../.venv` を共有）。

## TensorRT 変換（`/tensorrt` スキルを使う）

```
/tensorrt このフォルダの model.py の attention を TensorRT 化したい。
環境は ../.venv を使うこと。instruction として ./ フォルダの外の docs は読まないこと。
```

### オーケストレータが聞いてくる質問と、その答え方

| # | 聞かれること | いつ | この例での答え |
|---|------------|------|--------------|
| 1 | **何を変換対象にしますか？** | 最初 | **Attn モジュール（`self.attn`）** |
| 2 | **どの精度で変換しますか？** | 対象確定後 | **fp32 から** |
| 3 | **バックエンドは TensorRT で良いですか？**（setup） | install 前※ | **TensorRT** |
| 4 | **AcuiRT をどう導入しますか？**（setup） | install 時※ | **clone して editable 導入** |
| 5 | **accuracy_key は何にしますか？**（setup） | 配線時※ | **allclose-rate**（steady-state フレームで torch eager 一致率） |
| 6 | **dynamic shape をどう扱いますか？**（troubleshoot） | shape 不一致を検出時 | **steady-state（kv_len==K）フレームだけで変換**（真の dynamic-shape profile は使わない） |

> ※ 質問 3〜5（setup）は**そのフォルダで初めて install する時だけ**聞かれる。既に同じ env に
> AcuiRT/TensorRT が入っていれば skip される。質問 6 が B 固有の核心。

### 進行のイメージ（ログ）

```
[tensorrt] config: 対象 = attn（form (b) の children ネスト）
  → 入力 shape を確認 … kv の L が 1,2,…,8 と変化している
  → これは static shape ではない → troubleshoot に回す

[tensorrt] troubleshoot: dynamic-shape として分類
  → 全フレームで変換すると AcuiRT が
    "Dynamic shape loading is not implemented" で停止
  → 提案: 真の TRT dynamic-shape profile は使わない（別の難しい話）。
          代わりに変換のキャリブレーションを steady-state フレーム（kv_len == K）に絞る

[tensorrt] 再変換
  → make_inputs() のうち is_steady() == True のフレームだけ流す
  → shape が一定 (1,8,64) になり、engine build 成功

[tensorrt] verify
  → steady-state での accuracy 維持 / 速度記録
```

### 期待される結果
- 全フレーム変換は **dynamic-shape で失敗**（これがこのモジュールが教える失敗）。
- **steady-state フレームだけに絞ると変換が通る**。
- `SPARSEDRIVE_TRT_CONV_SKIP` 系の「キャリブレーションのフレーム選定」の教訓を、SparseDrive の
  複雑さ抜きで再現する。

> 注意（未確認）: AcuiRT が shape 変化に対して**最後の shape だけトレースする**のか、明示的に
> dynamic-shape エラーを出すのかは GPU 上で実行して確認が要る。前者なら B は失敗を再現しないので、
> 実機で最初に確かめる。
