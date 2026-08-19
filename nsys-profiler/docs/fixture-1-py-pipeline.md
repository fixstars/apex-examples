# Fixture 1 — py-pipeline (Python, functional)


**plain 関数**（`preprocess` / `infer` / `postprocess`）で組んだ Python + CUDA ワークロード ——
`nn.Module` サブクラスは使いません。プロファイラーの**関数レベル NVTX アノテーション**パスを
試すための題材です。

仕込み済みのボトルネック: `preprocess` での pageable H2D、`infer` での tiny-kernel flood
（40× `sin`）、`postprocess` でのステップごとの `.item()` 同期。

## セットアップ

このウォークスルーは単独で完結します。リポジトリを取得し、この fixture のディレクトリに移動して
スキルをインストールした上で、そこで opencode を起動します。

```bash
git clone <apex-examples-repo-url>
cd apex-examples/nsys-profiler/py-pipeline

# APEX マーケットプレイスを登録（未登録の場合）
apm marketplace add __APEX_URL__/apex/marketplace.git
# nsys-profiler プラグインのスキルをこのディレクトリに展開
apm install --target opencode nsys-profiler@apex-marketplace#nsys-profiler-v0.1.0

# 動作確認（torch + CUDA が必要）
python pipeline.py --steps 5 --warmup 1

# このディレクトリで opencode を起動
opencode   # または: claude

# opencodeからskillを実行
/nsys-analyzer　python pipeline.py --steps 30
```

Claude Code を使う場合は `--target opencode` の代わりに `--target claude` を指定してください。
以降の各ステップでは、起動した opencode / Claude Code 上でスキルを呼び出します。


## Profiler interview — 何を聞かれ、何を答えるか

この fixture は、プロファイラーの**関数レベル NVTX アノテーション**分岐が「正解」になるよう
作ってあります。インタビューの順序（プロファイラーの DESIGN.md より）:
**context-detect → Q-target → Q-nvtx → Q-trace → Q-gpu → Q-distributed → Q-output**。

プロファイラーに渡す起動コマンド:
```
python pipeline.py --steps 30 --warmup 5     (cwd: py-pipeline/)
```

| ステップ | 質問 | 答えるべき回答 | 理由 / 確認できること |
|---|---|---|---|
| context-detect | 「プロファイリングしているのは …?」 | この pipeline だと確認 | README/CLAUDE があれば読む。module が無いので「PyTorch モデル」と過大申告しないのが正解 |
| Q-target | Python / PyTorch / C++ / generic | **Python + CUDA (PyTorch ops, functional)** | NVTX を関数レベルに振る。module hooks には行かない |
| Q-nvtx | already have / agent adds / none | **agent adds them** | nvtx-inject を起動 |
| └ どの NVTX 戦略 | Layer 0 / module hooks / phase ranges / **function annotation** | **function annotation** | ここが肝: `preprocess`/`infer`/`postprocess` は plain 関数なので、module hooks では何も見つからない。エージェントは `pipeline.py` を読んでこの 3 関数をリストアップし、ラップ提案をするはず |
| Q-capture | cudaProfilerApi markers / delay-duration | **cudaProfilerApi** | Python マーカー挿入（`torch.cuda.profiler.start()/stop()`）を定常ループ周り、`--warmup` の後に挿入するのを確認 |
| Q-trace | minimal / cpu-sampling / rich | **rich** (`cuda,osrt,nvtx -s cpu`) | NVTX には nvtx トレースが必須。cpu-sampling は host gap の説明に有用 |
| Q-gpu | どのデバイス | **0** | `CUDA_VISIBLE_DEVICES=0` を設定 |
| Q-distributed | （スキップ — torchrun/mpirun ではない） | — | 単一プロセス |
| Q-output | dir / base | 例: `prof/py_pipeline` | analyzer 向け SQLite エクスポート |

**後に analyzer が見つけるべき仕込み**（想定）: `preprocess` の pageable H2D、`infer` の
tiny-kernel flood（40× `sin`）、`postprocess` のステップごとの `.item()` 同期。

**別パターンとして試す:** Q-nvtx = **none / コードを触らない** → `--pytorch=autograd-nvtx`
（op レベル）にフォールバックし、ユーザー関数への帰属はできないという注意を述べるはず。

## 「合格」の基準

1. context-detect が過大申告せず妥当な推測をする。
2. Q-target が正しくルーティングする（function annotation が選べる）。
3. nvtx-inject を選んだ場合、**正しい対象**を可逆的に編集する: `pipeline.py` の 3 関数に
   `nvtx_range` ラッパーが付く。
4. capture-window が言語に合ったマーカー（torch）を挿入する、もしくは delay/duration を選び、
   組み立てた `nsys profile` コマンドが対応するフラグを運ぶ。
5. `run_nsys.sh` が `.nsys-rep` + `.sqlite` と `=== PROFILE RESULT ===` ブロックを出す。
6. （エンドツーエンド）analyzer の `nvtx-timeline` が、注入したレンジ
   （`preprocess`/`infer`/`postprocess`）を起点にできる。

-----

