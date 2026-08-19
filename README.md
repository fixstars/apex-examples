# APEX Examples

APEX エージェントプラグイン向けのデモ用ワークロード集。各ディレクトリは小さく自己完結した
プロジェクトで、対応する APEX スキルが見つけて直すことを想定した意図的な問題を仕込んで
あります。スキルを既知のターゲットで試すのに使います。

## Prerequisites

APEX —— `apm` CLI とスキルマーケットプレイス —— は**別途配布されます**
（このリポジトリには含まれません）。APEX 配布物で案内されたマーケットプレイス URL を、最初に
一度だけ登録してください:

```bash
apm marketplace add __APEX_URL__/apex/marketplace.git
```

次に、試したい例の README に従ってプラグインをインストールします:

```bash
apm install --target opencode <plugin>@apex-marketplace   # または: --target claude
```

## Examples

| 例 | APEX プラグイン | 確認できること | スタック |
|---|---|---|---|
| [`perf/`](perf/) | `perf` | CPU プロファイリングスキル: 仕込み済みの cache-stride / branch-miss / I/O ストールボトルネック (`imgpipe`) | C / OpenMP |
| [`vectorize/`](vectorize/) | `vectorize` | 3D ポーズアライメントベンチマークでの SIMD ベクトル化ワークフロー (x86_64 + aarch64 クロスビルド) | C++ / CMake |
| [`tensorrt/`](tensorrt/) | `tensorrt` | TensorRT 変換、レッスンごとに 1 モジュール: ハッピーパス / ダイナミックシェイプ / カスタム CUDA カーネル / stateful リファクタ | PyTorch |
| [`nsys-profiler/`](nsys-profiler/) | `nsys-profiler` | GPU プロファイリング fixture (3): 関数レベル NVTX / Layer-1 module hooks / C++ capture markers + manual NVTX | CUDA C++ / PyTorch |
| [`nsys-analyzer/`](nsys-analyzer/) | `nsys-analyzer` | nsys SQLite エクスポートに対する GPU ボトルネック解析（事前キャプチャのプロファイルは後日公開予定） | nsys `.sqlite` |

注: 各例のウォークスルーは日本語で書かれています。

## License

Apache-2.0 —— [LICENSE](LICENSE) と [NOTICE](NOTICE) を参照してください。Copyright 2026 Fixstars Corporation.
