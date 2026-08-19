# オーケストレーターモード（ガイド付き調査）

`/perf-orchestrator` はガイド付きモードです。hotspot から始め、各ターンの結果を読み取って「次に何を測るべきか」を提案します。`imgpipe` には[キャッシュミスのウォークスルー](cache-misses.md)で扱っていないボトルネックがあと 2 つあります。オーケストレーターを起動して、これらの問題を見つけられるか試してみてください。

## セットアップ

このウォークスルーは単独で完結します。リポジトリを取得し、対象プログラム `imgpipe` のディレクトリに移動してスキルをインストールした上で、そこで opencode を起動します。

```bash
git clone <apex-examples-repo-url>
cd apex-examples/perf/imgpipe

# APEX マーケットプレイスを登録（未登録の場合）
apm marketplace add __APEX_URL__/apex/marketplace.git
# perf プラグインのスキルをこのディレクトリに展開
apm install --target opencode perf@apex-marketplace#perf-v0.1.0

# imgpipe をビルド
make
./imgpipe --duration 2    # 動作確認（短時間実行）

# このディレクトリで opencode を起動
opencode   # または: claude
```

以降の各ステップでは、起動した opencode / Claude Code 上でスキルを呼び出します。

---

## ボトルネックの一覧

1. **pipeline stalled waiting on feeder (off-CPU)** — `imgpipe.c:33` 付近。フィーダースレッドが 40ms 固定スリープでトークンを生成し、メインループが `load_tile()` で同期待ちする。compute（`transform` + `reduce`）の方が早く終わるため、毎フレーム I/O 待ちが支配的になる。

2. **unpredictable branch in reduce (branch misprediction)** — `imgpipe.c:65` 付近。`reduce` のホットループ内の分岐 `(v & 0xff) >= 128` が 50% の確率でランダムに切り替わり、分岐予測器が学習できない。分岐ミス率 15.96%（通常 < 3%）、IPC 0.52（通常 2–4）。

---

## やってみよう

```
/perf-orchestrator
```

オーケストレーターが hotspot から始め、結果に基づいて次に何を測るべきかを提案します。上記 2 つのボトルネックが見つかるか、実際に試して確認してください。

-----

[← セットアップとインストールは README](../README.md)
