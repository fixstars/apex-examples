# Fixture 3 — cpp-saxpy (CUDA C++)


生の **CUDA C++** ワークロード。warmup + ステップごとに H2D コピー + カーネル起動を繰り返す
定常ループです。プロファイラーの **C++ capture markers**（`cudaProfilerStart/Stop` +
sync-before-stop）および **manual NVTX**（`nvtxRangePush/Pop`）パスを試す題材です。
マーカーも NVTX も意図的に持たせていません。

仕込み済みのボトルネック: ステップごとの pageable H2D、20× のカーネル起動 flood、ステップごとの
`cudaDeviceSynchronize`。

## セットアップ

このウォークスルーは単独で完結します。リポジトリを取得し、この fixture のディレクトリに移動して
スキルをインストールした上で、そこで opencode を起動します。

```bash
git clone <apex-examples-repo-url>
cd apex-examples/nsys-profiler/cpp-saxpy

# APEX マーケットプレイスを登録（未登録の場合）
apm marketplace add __APEX_URL__/apex/marketplace.git
# nsys-profiler プラグインのスキルをこのディレクトリに展開
apm install --target opencode nsys-profiler@apex-marketplace#nsys-profiler-v0.1.0

# ビルド（CUDA ツールキット nvcc 必要）
nvcc -O2 -o saxpy saxpy.cu
./saxpy 3 1    # 動作確認（短時間実行）

# このディレクトリで opencode を起動
opencode   # または: claude

# opencodeからskillを実行
/nsys-analyzer ./saxpy 3 1 
```

Claude Code を使う場合は `--target opencode` の代わりに `--target claude` を指定してください。
以降の各ステップでは、起動した opencode / Claude Code 上でスキルを呼び出します。


## Profiler interview — 何を聞かれ、何を答えるか

この fixture は、プロファイラーの **C++ capture markers + manual NVTX** 分岐が「正解」に
なるよう作ってあります。インタビューの順序（プロファイラーの DESIGN.md より）:
**context-detect → Q-target → Q-nvtx → Q-trace → Q-gpu → Q-distributed → Q-output**。

先にビルド（nvcc 必要）: `nvcc -O2 -o saxpy saxpy.cu`。起動コマンド: `./saxpy 30 5`。

| ステップ | 質問 | 答えるべき回答 | 理由 / 確認できること |
|---|---|---|---|
| context-detect | 「プロファイリングしているのは …?」 | 確認: CUDA C++ の saxpy | C++/CUDA を検出し、PyTorch パスを提案しないのが正解 |
| Q-target | Python / PyTorch / **C++ / CUDA** / generic | **Raw CUDA / C++** | 自動注入を無効化し、C++ マーカー + manual NVTX に振る |
| Q-nvtx | already have / agent adds / none | **agent adds them** → **manual snippet** を期待 | C++ は自動注入不可。エージェントは `nvtxRangePush("infer")/Pop()` スニペット + `#include <nvtx3/nvToolsExt.h>` とリンクの注意を提示するはず |
| Q-capture | cudaProfilerApi markers / delay-duration | **cudaProfilerApi** | C++ マーカー挿入を確認: `#include <cuda_profiler_api.h>`、warmup 後に `cudaProfilerStart()`、**`cudaDeviceSynchronize()` してから `cudaProfilerStop()`**（stop 前に同期） |
| └ 別案 | delay-duration | **delay 1s / duration 3s** | 編集なしの汎用パス。未修正バイナリでも動くはず |
| Q-trace | minimal / cpu-sampling / rich | **rich** | フルトレースを試す |
| Q-gpu | どのデバイス | **0** | — |
| Q-distributed | （スキップ） | — | 単一プロセス |
| Q-output | dir / base | `prof/saxpy` | — |

**後に analyzer が見つけるべき仕込み:** ステップごとの pageable H2D、20× のカーネル起動 flood、
ステップごとの `cudaDeviceSynchronize`。

**相対パスのチェック:** 別の cwd から `./saxpy …` として起動コマンドを与え、プロファイラーの
相対パス警告（Q1）が発火して絶対パス / 明示的 cwd を求めてくるか確認します。

## 「合格」の基準

1. context-detect が過大申告せず妥当な推測をする。
2. Q-target が正しくルーティングする（cpp → manual NVTX のみ、自動注入なし）。
3. nvtx-inject を選んだ場合、**貼り付けるスニペット**を渡す（自動編集ではない）:
   `nvtxRangePush("infer")/Pop()` 呼び出し + `#include` とリンクの注意。
4. capture-window が言語に合ったマーカー（`cudaProfilerStart()` +
   `cudaDeviceSynchronize()` してから `cudaProfilerStop()`）を挿入する、もしくは
   delay/duration を選び、組み立てた `nsys profile` コマンドが対応するフラグを運ぶ。
5. `run_nsys.sh` が `.nsys-rep` + `.sqlite` と `=== PROFILE RESULT ===` ブロックを出す。
6. （エンドツーエンド）analyzer の `nvtx-timeline` が、注入したレンジを起点にできる。

-----

