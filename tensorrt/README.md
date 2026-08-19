# tensorrt

`tensorrt` プラグインのスキルを 1 つずつ通すための、**独立した**小さいモジュール集。SparseDrive の
代わりの最初の例: SparseDrive は custom-op + state + dynamic shape を一度に積んでいて失敗の切り分けが
大変だった。ここは **1 モジュール ⇄ 1 スキル ⇄ 1 つの失敗と直し方**。

各モジュールは**自分のフォルダ**にあり、そのフォルダから Claude Code / opencode を起動して
`/tensorrt` スキルを実行します。各 `model.py` は**素の・未変換の `nn.Module`** で、CLI も rt-config も
変換も**プラグイン側のスキルが足します**（例の方では一切先回りしない）。それがこの例の目的です。

| フォルダ | 通すスキル | 教訓 | ウォークスルー |
|---------|-----------|------|--------------|
| [`a_backbone/`](a_backbone/) | setup, config, verify | ハッピーパス: トラブルシュートゼロで変換（resnet18） | [Module A](docs/module-a.md) |
| [`b_dynamic/`](b_dynamic/) | config, troubleshoot | 同一モジュールが多数の shape → steady-state フレームだけで変換 | [Module B](docs/module-b.md) |
| [`c_kernel/`](c_kernel/) | custom-kernel, troubleshoot | 実カスタム CUDA op → `tensorrt.plugin`；int64 ブリッジ + 全ゼロバグを再現 | [Module C](docs/module-c.md) |
| [`d_stateful/`](d_stateful/) | refactor, troubleshoot | 内部 state + 分岐 → state を明示 I/O に持ち上げる | [Module D](docs/module-d.md) |

`c_kernel/` は `nvcc` + CUDA GPU が要ります（実 `.cu` を `load_inline` でコンパイル）。他は torch
（A は torchvision も）だけで動きます。

Module E（backbone + head を合体、CUDA-graphs ⨯ TRT 共存）は A–D が通ってからの後回し。

---

## Installation

```bash
git clone <apex-examples-repo-url>
cd apex-examples/tensorrt
```

全モジュールで**共通の環境を 1 つだけ**作って使い回します（install は一度きり）。venv は
**リポジトリ root** に作り、各モジュールから `../.venv` として参照します。

### 1. 共通の venv を作る（リポジトリ root で一度だけ）

```bash
python -m venv .venv
source .venv/bin/activate
uv pip install torch torchvision --index-url https://download.pytorch.org/whl/cu126
```

> - `.venv` は **root に 1 つ**。各モジュール（`a_backbone/` 等）はこの `../.venv` を共有する。
> - ここで入れるのは torch / torchvision（共通ベース）だけ。**AcuiRT / TensorRT / torch2trt は
>   `/tensorrt` の `setup` スキルがこの同じ venv に入れて案内する**（順序・pin・落とし穴も含めて
>   skill が面倒を見る）。**install は最初の 1 回だけ**で、以降のモジュールは同じ環境を使うので
>   setup の質問（backend / AcuiRT 導入）は skip される。

### 2. APM でスキルをインストール（root で一度だけ）

APEX 配布物に含まれる `apm` CLI を使います。未登録の場合は、配布物で案内された
マーケットプレイス URL を先に登録してください:

```bash
apm marketplace add https://__APEX_HOST__:4430/apex/marketplace.git
apm install --target opencode tensorrt@apex-marketplace#tensorrt-v0.1.0
```

> - `tensorrt` オーケストレータが setup / config / refactor / custom-kernel / troubleshoot /
>   verify のヘルパースキルを必要に応じて呼びます（個別にインストールする必要はありません）。
> - Claude Code を使う場合は `--target opencode` の代わりに `--target claude`（`.claude/skills/`
>   に展開）。

### 3. 各モジュールで `/tensorrt` を起動する

変換したいモジュールのフォルダ（例: `a_backbone/`）に入り、そこで opencode / Claude Code を
起動します。スキルには**共通 venv を使うこと**と**このフォルダの外の docs は読まないこと**を
明示します（各モジュール doc の起動テキストをそのまま使う）:

```
/tensorrt このフォルダの model.py を TensorRT 化したい。
環境は ../.venv を使うこと。instruction として ./ フォルダの外の docs は読まないこと。
```

> 「外の docs を読まない」のは、ルート README および [`docs/`](docs/) のモジュール walkthrough を
> 見てなぞる（=ズルする）のを防ぐため。エージェントは目の前の `model.py` から本当に作業する。
> ウォークスルーは人間用（GitHub / serve ドキュメント）であって、エージェントは読まない。

> `c_kernel`（カスタム CUDA カーネル）だけは `nvcc` + CUDA GPU が要ります。他の 3 つは torch
> （A は torchvision も）だけで動きます。

---

## Usage

各モジュールは**自分のフォルダ**にあり、そのフォルダから Claude Code / opencode を起動して
`/tensorrt` スキルを実行します。以下の walkthrough は各モジュールでスキルがどう進むか、何を聞かれ
どんな結果になるかを記載したものです。

- [Module A — clean backbone（ハッピーパス）](docs/module-a.md)
- [Module B — dynamic-shape module（steady-state が直し方）](docs/module-b.md)
- [Module C — custom CUDA kernel（plugin パス）](docs/module-c.md)
- [Module D — refactor が要るモジュール（参照透過性）](docs/module-d.md)
