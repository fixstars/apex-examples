# nsys-analyzer

NVIDIA Nsight Systems (nsys) でプロファイルした GPU プログラムのプロファイリング結果
`.sqlite` を解析する **nsys-analyzer** APEX プラグインの例題集。

イテレーション検出、区間ごとの GPU busy/idle・シェア、シグナル（idle / kernel launches /
blocking sync / H2D transfer / CUDA API）の抽出をユーザーの問いに応じたレベルで実行し、
必要に応じて ad-hoc SQL でさらに掘り下げます。プロファイルの取得は nsys-profiler を使います。

## Installation

```bash
apm marketplace add __APEX_URL__/apex/marketplace.git
```

解析対象の `.sqlite` を置いたディレクトリでスキルをインストールします:

```bash
# 1. プラグインモジュールをローカルに取得
apm install --target opencode nsys-analyzer@apex-marketplace#nsys-analyzer-v0.1.0

# 2. 取得したモジュールを .agents/ (または .claude/) に展開
apm install --target opencode
```

そのディレクトリで `opencode` を起動すると `nsys-analyzer` スキルが使えます。
Claude Code を使う場合は `--target opencode` の代わりに `--target claude` を指定してください。

## Usage

以下のいずれかで起動できます:

- スラッシュコマンドで直接指定する:

  ```
  /nsys-analyzer
  ```

- 自然言語で依頼する:

  > `nsys-reports/sample.sqlite` を解析してください

いずれの場合も、エージェントがイテレーションの内訳とシグナルテーブルを提示し、興味のある区間・
イテレーションを掘り下げる選択肢を案内します。

### スキル一覧

| スキル | 内容 |
|--------|------|
| `nsys-analyzer` | オーケストレーター。ユーザーの問いに応じて inspect / sql を呼び分ける |
| `nsys-analyzer-inspect` | 任意の窓を 1 イテレーション単位で描画し、GPU busy/idle・シェア・シグナルを出す |
| `nsys-analyzer-sql` | 固定シグナルで表せない位置依存の問いに ad-hoc SQL で答える支援スキル |
| `nsys-analyzer-core-library` | 他の analyzer スキルが import する共有エンジン（直接は実行しない） |

### `/nsys-analyzer`

オーケストレーター。ユーザーの問い（「ボトルネックを教えて」「`infer` 区間の GPU busy は?」等）
に応じて `inspect` / `sql` を呼び分け、イテレーション検出 → シグナルテーブル → ドリルダウンの
流れで案内します。

### `/nsys-analyzer-inspect`

任意の窓を 1 イテレーション単位で描画し、GPU busy/idle の比率・シェア・シグナル
（idle / kernel launches / blocking sync / H2D transfer / CUDA API）を出します。
`--root <range>` で NVTX レンジに絞った解析も可能（`nsys-profiler-nvtx-inject` で付与した
`preprocess`/`infer`/`postprocess` 等のレンジに anchoring できる）。

### `/nsys-analyzer-sql`

固定シグナルで表せない位置依存の問い（「step 3 の `infer` 内の kernel launch の数は?」等）に
ad-hoc SQL で答えます。`.sqlite` を直接クエリするので、`inspect` の定型レポートに無い項目も
掘り下げられます。

### `/nsys-analyzer-core-library`

他の analyzer スキル（`nsys-analyzer`, `nsys-analyzer-inspect`, `nsys-analyzer-sql`）が import する
共有エンジン。ユーザーが直接実行することはありません。

