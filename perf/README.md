# `/perf` — サンプルワークロード

意図的にパフォーマンスのバグを仕込んだ小さな C プログラム集です。`/perf` プラグインを既知のターゲットに対してテストするために使用します。各プログラムには実際のパスロジック（悪い分岐、キャッシュ非効率なアクセスパターン、I/O ストールなど）があり、以下のウォークスルーでスキルの出力と解釈方法を確認できます。

## Installation

 `apm` CLI を使って `/perf` スキルをインストールします

```bash
apm marketplace add __APEX_URL__/apex/marketplace.git
```

**opencode を起動するディレクトリから**実行してください:

```bash
apm install --target opencode perf@apex-marketplace#perf-v0.1.0
```

そのディレクトリで `opencode` を起動すると `/perf` スキルが使えます。

## Usage

2 つの使い方があります:

- **個別スキル** — 一度に 1 つの分析を実行します。例: `/perf-hotspot`、`/perf-branch-miss`
- **オーケストレーター** — `/perf-orchestrator` が調査全体を自動で進めます。対象を起動し、まず hotspot を実行してから、結果に基づいて次に何を計測すべきかを提案します。

## スキル一覧

| スキル | 内容 |
|--------|------|
| `perf-runner` | 対象を起動し、生存確認後にセッションを書き出す（オーケストレーターが自動で行う） |
| `perf-orchestrator` | 調査全体をガイド: どこで → なぜ → 未計測の軸は何か |
| `perf-hotspot` | CPU 時間の行き先（cycles フレームグラフ） |
| `perf-cache` | キャッシュミスの集中箇所（cache-miss フレームグラフ、粗め） |
| `perf-counters` | IPC + 分岐ミス率 |
| `perf-tma` | ストールの原因（トップダウンマイクロアーキテクチャ分析） |
| `perf-cache-miss` | 行レベルの精密なキャッシュミス帰属（PEBS） |
| `perf-branch-miss` | 精密な分岐ミス帰属（PEBS） |
| `perf-ports` | 実行ポートの飽和・ストール構成 |
| `perf-simd` | FP ベクトル化幅（scalar/AVX2/AVX-512）とベクトル化率 |
| `perf-c2c` | スレッド間のフォルスシェアリング |
| `perf-bandwidth` | DRAM 帯域の飽和 |
| `perf-offcpu` | off-CPU 時間（I/O / ロック / 待機） |
| `perf-syscall-time` / `perf-offcpu-sched` | off-CPU のドリルダウン |
| `perf-annotate` | 既存の `.data` ファイルに対する行/命令レベルのドリル |
| `perf-issue` | finding とソースコードを読み、修正案を Markdown で生成 |

## サンプルプログラム

`imgpipe` は異なる軸に 3 つの独立したボトルネックを持つ多軸プログラムで、オーケストレーターのテストに最適です。

> ウォークスルーはプログラムディレクトリの外に置いています（[`docs/`](docs/)）。プログラムディレクトリ内で動作するエージェントが答えを読めないようにするためです。

## ウォークスルー

詳細なウォークスルーは [`docs/`](docs/) にあります:

- [キャッシュミス (Cache Misses)](docs/cache-misses.md) — `/perf-hotspot` から `/perf-issue` まで、ステップ 1〜5 の調査手順と出力例
- [オーケストレーターモードのボトルネック](docs/orchestrator-bottlenecks.md) — `/perf-orchestrator` で試す 2 つの追加ボトルネック（pipeline stall / branch misprediction）
