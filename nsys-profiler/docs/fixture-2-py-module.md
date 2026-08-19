# Fixture 2 — py-module (PyTorch, user nn.Module tree)


**ユーザー定義の `nn.Module` サブクラス**（`Stem` / `Block` / `Head` / `TinyNet`）で組んだ
PyTorch ワークロード。プロファイラーの **Layer-1 module-hooks** NVTX パスを試す題材です
（`auto_instrument` が `stem` / `blocks.0..2` / `head` の named ranges を付与）。

仕込み済みのボトルネック: ステップごとの pageable H2D、**`Block` 内**の tiny-op flood
（`blocks.N` に局在）、ステップごとの `.item()` 同期。

## セットアップ

このウォークスルーは単独で完結します。リポジトリを取得し、この fixture のディレクトリに移動して
スキルをインストールした上で、そこで opencode を起動します。

```bash
git clone <apex-examples-repo-url>
cd apex-examples/nsys-profiler/py-module

# APEX マーケットプレイスを登録（未登録の場合）
apm marketplace add __APEX_URL__/apex/marketplace.git
# nsys-profiler プラグインのスキルをこのディレクトリに展開
apm install --target opencode nsys-profiler@apex-marketplace#nsys-profiler-v0.1.0

# 動作確認（torch + CUDA が必要）
python model.py --steps 5 --warmup 1

# このディレクトリで opencode を起動
opencode   # または: claude


# opencodeからskillを実行
/nsys-analyzer　python model.py --steps 5
```

Claude Code を使う場合は `--target opencode` の代わりに `--target claude` を指定してください。
以降の各ステップでは、起動した opencode / Claude Code 上でスキルを呼び出します。


## Profiler interview — 何を聞かれ、何を答えるか

この fixture は、プロファイラーの **Layer-1 module-hooks** 分岐が「正解」になるよう作って
あります。インタビューの順序（プロファイラーの DESIGN.md より）:
**context-detect → Q-target → Q-nvtx → Q-trace → Q-gpu → Q-distributed → Q-output**。

起動コマンド:
```
python model.py --steps 30 --warmup 5     (cwd: py-module/)
```

| ステップ | 質問 | 答えるべき回答 | 理由 / 確認できること |
|---|---|---|---|
| context-detect | 「プロファイリングしているのは …?」 | 確認: 小さな PyTorch nn.Module ネット | PyTorch を検出し、ユーザー定義 module を持つことを認識するはず |
| Q-target | Python / **PyTorch** / C++ / generic | **PyTorch** | model-aware NVTX（Layer 0/1/2）を有効化 |
| Q-nvtx | already have / agent adds / none | **agent adds them** | nvtx-inject を起動 |
| └ どの NVTX 戦略 | Layer 0 / **module hooks (L1)** / phase ranges / function annotation | **module hooks (Layer 1)** | ここが肝: `TinyNet` はユーザー定義クラスのツリー（`Stem`/`Block`/`Head`）なので、`auto_instrument` が `stem`/`blocks.0..2`/`head` の named ranges を付ける。エージェントは `model = TinyNet(...)` の後に 2 行を追加（または env gate 経由）し、`APEX_NSYS_NVTX_MODULES=1` を設定するはず |
| └ depth サブ質問 | 値 / unlimited / agent-recommended | **unlimited**（浅いモデル） | depth タブの確認 |
| Q-capture | cudaProfilerApi / delay-duration | **cudaProfilerApi** | torch マーカーを定常ループ周りに挿入（注: model は `inference_mode` で backward 無し —— Layer-1 forward hooks でカバー） |
| Q-trace | minimal / cpu-sampling / **rich** | **rich** | nvtx トレースが必須 |
| Q-gpu | どのデバイス | **0** | — |
| Q-distributed | （スキップ） | — | 単一プロセス |
| Q-output | dir / base | `prof/py_module` | — |

**後に analyzer が見つけるべき仕込み:** ステップごとの pageable H2D、**`Block` 内**の
tiny-op flood（analyzer が `blocks.N` に局在させるはず）、ステップごとの `.item()` 同期。
この fixture では module スコープの帰属（`--filter-nvtx=blocks.1`、または
`nvtx-timeline --root blocks.1`）が効くはずです。

**比較チェック:** ここで**関数アノテーション**戦略を走らせるのも*妥当ですが自然ではありません*
—— この fixture では module hooks、`py-pipeline` では関数アノテーションを推奨するか確認
してください。

## 「合格」の基準

1. context-detect が過大申告せず妥当な推測をする。
2. Q-target が正しくルーティングする（ユーザー定義 module 付き PyTorch → module hooks が選べる）。
3. nvtx-inject を選んだ場合、**正しい対象**を可逆的に編集する: `model.py` の
   `model = TinyNet(...)` の後に 2 行の hook 登録が追加される。
4. capture-window が言語に合ったマーカー（torch）を挿入し、組み立てた `nsys profile` コマンドが
   対応するフラグを運ぶ。
5. `run_nsys.sh` が `.nsys-rep` + `.sqlite` と `=== PROFILE RESULT ===` ブロックを出す。
6. （エンドツーエンド）analyzer の `nvtx-timeline` が、注入した module レンジ
   （`stem`/`blocks.0..2`/`head`）を起点にできる。`--filter-nvtx=blocks.1` で `Block` に絞れる。

-----

