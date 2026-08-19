# nsys-profiler

NVIDIA Nsight Systems (nsys) を用いて GPU プログラムをプロファイリングする
**nsys-profiler** APEX プラグインの例題集。

ユーザーとの対話を通じて対象プログラム・定常状態の区間・トレース深度・GPU・分散構成を聞き取り、
必要に応じて NVTX マーカー挿入や cudaProfilerApi ベースの区間指定を補助した上で、`nsys profile` を
実行しプロファイリング結果として `.sqlite` ファイルを得ます。得られた `.sqlite` は
nsys-analyzer で解析できます。

## Installation

```bash
apm marketplace add __APEX_URL__/apex/marketplace.git
```

各 fixture ディレクトリ（例: `py-pipeline/`）で、そこから opencode / Claude Code を起動する
前提でスキルをインストールします:

```bash
# 1. プラグインモジュールをローカルに取得
apm install --target opencode nsys-profiler@apex-marketplace#nsys-profiler-v0.1.0

# 2. 取得したモジュールを .agents/ (または .claude/) に展開
apm install --target opencode
```

各 fixture ディレクトリで `opencode` を起動すると `nsys-profiler` スキルが使えます。
Claude Code を使う場合は `--target opencode` の代わりに `--target claude` を指定してください。

## Usage

以下のいずれかで起動できます:

- スラッシュコマンドで直接指定する:

  ```
  /nsys-profiler
  ```

- 自然言語で依頼する:

  > このプログラムを nsys でプロファイルしてください

いずれの場合も、エージェントが対象・定常状態の切り出し方・トレース深度などをインタビューし、
必要な NVTX 追加や実行コマンドを提示した上で `nsys profile` を実行します。


### スキル一覧

| スキル | 内容 |
|--------|------|
| `nsys-profiler` | オーケストレーター。インタビュー → 補助スキル呼び出し → `nsys profile` 実行 |
| `nsys-profiler-context-detect` | `CLAUDE.md` / `README.md` などから対象プログラムの種別を推測しインタビューの起点にする |
| `nsys-profiler-capture-window` | 定常状態の区間を決める（cudaProfilerApi マーカー挿入 or `--delay/--duration` 指定） |
| `nsys-profiler-nvtx-inject` | NVTX レンジの追加。analyzer が `--root <range>` で論理領域に絞れるようにする |

### `/nsys-profiler`

オーケストレーター。インタビューの順序は
**context-detect → Q-target → Q-nvtx → Q-trace → Q-gpu → Q-distributed → Q-output**。
対象に応じて補助スキル（`context-detect`, `nvtx-inject`, `capture-window`）を呼び分け、
最後に `nsys profile` を実行して `.sqlite` を生成します。

### `/nsys-profiler-context-detect`

fixture の `README.md` / `CLAUDE.md` を読み、対象プログラムの種別（Python functional / PyTorch
`nn.Module` / CUDA C++）を推測し、後続のインタビューの起点にします。過大申告しない（無いものを
PyTorch モデルと呼ばない等）のが正解の基準。

### `/nsys-profiler-nvtx-inject`

ユーザーが NVTX を持たない場合、対象に応じてレンジを追加します:

- **関数アノテーション** — `preprocess`/`infer`/`postprocess` のような plain 関数に `nvtx_range`
  ラッパーを挿入（`py-pipeline/` 向け）。
- **module hooks (Layer 1)** — `nn.Module` ツリーに `auto_instrument` で named ranges
  （`stem`/`blocks.0..2`/`head`）を付与（`py-module/` 向け）。
- **manual snippet** — CUDA C++ は自動注入不可。`nvtxRangePush/Pop` と `#include` のスニペットを
  提示（`cpp-saxpy/` 向け）。

### `/nsys-profiler-capture-window`

定常状態の区間を決めます:

- **cudaProfilerApi マーカー** — `cudaProfilerStart()` を warmup 後、
  `cudaDeviceSynchronize()` してから `cudaProfilerStop()`（同期し忘れを防ぐ）。
- **delay / duration** — コードを編集しない汎用パス。`--delay 1s --duration 3s` のように指定。

---

詳細な例は [`docs/`](docs/) を参照してください:

- [Fixture 1 — py-pipeline (Python, functional)](docs/fixture-1-py-pipeline.md)
- [Fixture 2 — py-module (PyTorch, user nn.Module tree)](docs/fixture-2-py-module.md)
- [Fixture 3 — cpp-saxpy (CUDA C++)](docs/fixture-3-cpp-saxpy.md)
