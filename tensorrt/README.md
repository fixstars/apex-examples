# tensorrt

`/tensorrt` スキルは、環境セットアップからモデル変換、トラブルシューティング、カスタム CUDA
カーネル対応まで、状況に応じて複数のワークフローに対応しています。環境が手動でセットアップされていない場合は、
スキルが必要なセットアップも行います。

以下では、代表的なユースケースを例として紹介します。

* **基本的なモデル変換** — `resnet18` のような標準的な `nn.Module` を変換する — [Example](docs/module-a.md)
* **Dynamic shapes** — 複数の shape を持つモデルを steady-state のフレームに基づいて変換する — [Example](docs/module-b.md)
* **Custom CUDA kernel** — `tensorrt.plugin` を使ってカスタム CUDA op を TensorRT に対応させる — [Example](docs/module-c.md)
* **Stateful model** — 内部 state や分岐を持つモデルを、state を明示的な I/O として扱える形にリファクタリングする — [Example](docs/module-d.md)

---

## Installation

```bash
apm marketplace add __APEX_URL__/apex/marketplace.git
```

```bash
apm install --target opencode tensorrt@apex-marketplace#tensorrt-v0.2.0
```

> - `tensorrt` オーケストレータが setup / config / refactor / custom-kernel / troubleshoot /
>   verify のヘルパースキルを必要に応じて呼びます（個別にインストールする必要はありません）。
> - Claude Code を使う場合は `--target opencode` の代わりに `--target claude`（`.claude/skills/`
>   に展開）。

## Usage

各モジュールは**自分のフォルダ**にあり、そのフォルダから Claude Code / opencode を起動して
`/tensorrt` スキルを実行します。以下の walkthrough は各モジュールでスキルがどう進むか、何を聞かれ
どんな結果になるかを記載したものです。

- [Module A — clean backbone（ハッピーパス）](docs/module-a.md)
- [Module B — dynamic-shape module（steady-state が直し方）](docs/module-b.md)
- [Module C — custom CUDA kernel（plugin パス）](docs/module-c.md)
- [Module D — refactor が要るモジュール（参照透過性）](docs/module-d.md)
