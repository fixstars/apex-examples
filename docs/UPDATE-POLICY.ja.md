# `apex-examples` 更新ポリシー

本ドキュメントは `apex-examples` が `apex-marketplace` の変更に伴っていつ・どのように
更新されるか、および examples リポジトリと `apex-marketplace-serve` の関係を公式に定義する。

**メンテナ向け**の文書である。3つの決定（lockstep enforcement、examples tagging、
serve-floor policy）は provisional recommendation を併記した上で未決定として残して
あり、*provisional* と表示している。これらは APEX チームが ratify するまで拘束力を持たない。

---

## 1. 背景 — 3リポジトリの関係

```
apex-examples          (public)    downstream consumer。GitHub に open-source 公開
    ^
    |  apm 経由で plugin を install
    |
apex-marketplace       (private)  plugin ソース。各 plugin は
    |                             <pkg>-v<semver> (例: perf-v0.1.0) で tag 付け
    |  ビルド時に main を clone し全 tag を frozen image に mirror
    v
apex-marketplace-serve (released) snapshot 配信サーバ。ビルド時に
                                 存在した tag のみ公開する
```

plugin の **git tag** (`<pkg>-v<semver>`) が example と plugin の間の実契約である。
`apex-marketplace-serve` は契約を versioning するのではなく、単に plugin tag を
frozen image に mirror する。したがって serve image v1.0.0 が公開するのは
その image がビルドされた時点で存在した plugin tag そのものであり、再ビルド
しない限り更新されない。

ユーザは example dir の中で `apm install` を実行して plugin を install する。
これは example の `apm.yml` を読み、その `ref` field に記録された plugin tag を
install する (section 6)。これが per-example の version pin であり、各 example は
自身の walkthrough が validated された tag を記録する。"always latest" (section 2)
は `main` が常に最新の **validated** tag を持つことであり、ユーザが serve 上の
最新 tag に float することではない。examples と serve の coupling は **ゼロ** であり、
marketplace との coupling は **明示的かつ event-driven** である（section 3 で定義）。

---

## 2. "Always latest" モデル

- **ユーザ側:** `apex-examples/main` を clone し、example dir に `cd` して
  `apm install` を実行する（example の `apm.yml` を読み、その `ref` に記録された
  validated plugin tag を install する）。"always latest" は `main` が常に最新の
  **validated** tag を持つことを意味する — plugin 変更が trigger checklist
  (section 3) を通過した場合、該当 example の `apm.yml` の `ref` が paired MR で
  前に進められる。ユーザは "always newest tag on serve" ではなく
  "always latest validated" を受け取る。version の取捨選択や tag の選び方は無い。
- **メンテナ側:** `apex-examples/main` は `apex-marketplace/main` と lockstep を
  維持する。examples リポジトリは実質的に marketplace の downstream consumer かつ
  gated test fixture である。plugin の変更が example の end-user experience を
  変える場合 (section 3) に**のみ**更新され、その更新は plugin の変更と**同時に**
  (後ではなく) 着地する。

`main` が唯一のサポート対象である。user-facing の version 情報は1つだけ存在する:
`COMPATIBILITY.md` (section 10) に記録された serve-floor である。古い serve image の
ユーザが最新 examples を clone した場合、serve を更新 (`docker compose pull &&
docker compose up -d`) するか、(examples tag が存在すれば section 8 の方針に従い)
古い examples commit を checkout する。

---

## 3. トリガーチェックリスト — いつ example を更新しなければならないか

plugin の変更が以下のいずれか ("end-user experience" 面) を引き起こす場合、example の
更新が必要である:

1. walkthrough の **expected output** が変わる — 数値、表の形状、top function や
   top source-line の帰属 (例: "56.35% `transform._omp_fn.0`" が変化)。
2. **interview の質問文** または answer choice の集合が変わる。
3. **skill 名** または slash-command が rename / remove / add される (例: `/perf-hotspot`
   が rename される、workflow に新 skill が追加される)。
4. walkthrough がユーザに渡すよう指示する **CLI flag** または skill 引数が変わる。
5. **planted bug の位置** またはそれを surface する code path が移動する (稀。example
   source も移動した場合のみ)。
6. **skill workflow の順序** が変わる — どの skill がどの skill を呼ぶか、
   orchestrator の decision tree。

上記のいずれにも触れない patch level の plugin fix は **install-compatible** であり、
example の更新は不要である。end-user 可視の挙動が変わらないため、walkthrough は
patch に対してもそのまま成立する。更新が発火した時、それを記録する具体的な仕組みが
該当 example の `apm.yml` の `ref` field の bump である (section 6)。

---

## 4. Authorship (決定済み)

**振る舞いを変更した plugin author が、pair となる example 更新を書く。** 何が
変わったかの context は author が最も持っている。どの example が影響するか不明確な
場合は examples maintainer に hand-off するが、trigger detection (section 7) が
影響先 example を自動で surface することが期待される。

---

## 5. 更新 workflow (いつ・どのように)

1. plugin author が `apex-marketplace` に MR を起票する。
2. diff がいずれかの example について checklist 項目 (section 3) に該当する場合、
   author は `apex-examples` に **paired MR** を起票し、該当 example の walkthrough
   および/または source を更新する。
3. author は該当 example の `apm.yml` の `ref` field を新しい validated plugin tag に
   bump する (section 6)。
4. 両 MR は **同じ release window** に着地する。plugin tag は example が更新される
   **まで cut されない** — これが lockstep gate である。
5. `COMPATIBILITY.md` (section 10) の "main" 行を更新し、新しい plugin tag と、
   変更が serve floor を上げる場合は新しい最低 serve version を記録する (ratify
   された section 9 の policy に従う)。
6. この時点以降に `main` を pull したユーザは、自動的に整合する example + plugin を
   受け取る。

---

## 6. Per-example `apm.yml` の慣習

各 example dir には `apm.yml` があり、その example の walkthrough が validated された
plugin tag を**機械可読で記録**する。これが per-example の契約であり、
`COMPATIBILITY.md` (section 10) はそこから導出される人間可読の要約である。

### Fields

| Field | 値 |
|---|---|
| `name` | example 自身の名前 (例: `tensorrt-demo`) |
| `version` | example 自身の version (semver) |
| `description` | 短い説明 |
| `author` | `APEX team` |
| `license` | `MIT` (examples は MIT。marketplace plugin は `LicenseRef-Fixstars-Proprietary`) |
| `devDependencies.apm[]` | example が依存する plugin 1つにつき1エントリ |
| `.git` | marketplace URL の placeholder `https://<your-marketplace-host>/apex/marketplace.git` |
| `.path` | `packages/<pkg>` |
| `.ref` | validated plugin tag。例: `tensorrt-v0.1.0` |
| `mcp` | `[]` |
| `includes` | `auto` |

### install path

ユーザは `main` を clone し、example dir に `cd` して `apm install` を実行する
（`apm.yml` を読み、pinned された `ref` を install する）。unpinned 形式の
`apm install <pkg>@apex-marketplace` は本 model では **documented install path ではない** —
各 example の README install snippet は unpinned 形式ではなく、example dir で
`apm install` を実行するようユーザに案内すべきである。

### Template (tensorrt)

```yaml
name: tensorrt-demo
version: 1.0.0
description: tensorrt package demo project
author: APEX team
license: MIT
devDependencies:
  apm:
    - git: https://<your-marketplace-host>/apex/marketplace.git
      path: packages/tensorrt
      ref: tensorrt-v0.1.0
  mcp: []
includes: auto
```

### `ref` が意味するもの

`ref` は walkthrough が最後に validated された tag である。serve 上に存在しても
trigger checklist (section 3) を通過していないより新しい plugin tag は、paired MR が
`ref` を bump する**まで**いかなる example からも参照されない。したがってユーザは
この経路で unvalidated な tag を受けることはない。

### placeholder host

`git:` URL は `https://<your-marketplace-host>/apex/marketplace.git` のままにする
（ユーザは `apm marketplace add` で自身の serve URL に置き換える）。`ref` は
host 非依存（git tag である）のため、ユーザがどの serve instance を指していても
pinning は機能する。

### Multi-module example: module 単位の `apm.yml`

multi-module example（`tensorrt/`、`nsys-profiler/`）では、`apm.yml` は top-level
directory 単位ではなく**module 単位**（module / fixture 1つにつき1つ）に置く。各
module はちょうど1つの plugin に依存するため、`ref` pin および（ADR-0003 option D4
による）mirror された `version` は一意に定まる:

| Module | 依存 plugin |
|---|---|
| `tensorrt/a_backbone/` | `tensorrt` |
| `tensorrt/b_dynamic/` | `tensorrt` |
| `tensorrt/c_kernel/` | `tensorrt` |
| `tensorrt/d_stateful/` | `tensorrt` |
| `nsys-profiler/py-pipeline/` | `nsys-profiler` |
| `nsys-profiler/py-module/` | `nsys-profiler` |
| `nsys-profiler/cpp-saxpy/` | `nsys-profiler` |

`tensorrt/` と `nsys-profiler/` はどちらも **all-in-1 hub README** pattern を使う:
top-level に1つの `README.md` を置き、Installation、Usage（skill sub-section 付き）、
module 毎の walkthrough section を保持する。module sub-folder には source + `apm.yml`
のみ（module 毎の `README.md` は無い）。agent は module sub-folder で起動し、hub の
walkthrough は読まないよう指示される（anti-cheat design を維持）。

single-project example（`perf/`、`vectorize/`、`nsys-analyzer/`）は上記 template の
通り top-level に1つの `apm.yml` を置く。`nsys-analyzer/` にはまだ fixture が無い
（pre-captured profile は後日公開）。

### README 構造: 単一 hub、必須 section

全ての example hub `README.md`（docs-merge target）には `## Installation` と
`## Usage` が main section として必須。複数の use case を持つ skill は Usage 配下に
sub-section を置く。multi-module example（`tensorrt/`、`nsys-profiler/`）は per-
module walkthrough を同じ hub README のさらに `##` section として埋め込む
（all-in-1 pattern）— module sub-folder は自身の `README.md` を持たない。agent は
module sub-folder で起動し、hub の walkthrough は読まないよう指示される
（anti-cheat design を維持）。`vectorize/demo/README.md` は例外（agent 可読の build
command のみ。walkthrough は deny された `README.md`）。

これらの慣習は [`docs/lint-examples.sh`](lint-examples.sh) で enforce する。確認
項目: README section の存在、leaf dir 毎の `apm.yml` の存在、`apm.yml` の `ref`
形式（`path:` field から導出した `<package-name>-v<semver>` に一致）、旧 folder
名の不存在。lint は offline のみ — ref tag が remote に実在するかは検証しない
（それは ADR-0001 により L2 release gate の役割。paired-MR 時の chicken-and-egg
を避けるため）。

---

## 7. Decision A — Lockstep enforcement

*Provisional recommendation: **B — bot-nudge**. ratify 待ち。*

"plugin MR は example を更新しなければならない" というルールをどう enforce するか。

### A. Process-only
example-touched skill file 上の CODEOWNERS + MR template checklist
("あなたの変更はいずれかの example walkthrough に影響するか?")。

- **Pros:** infra ゼロ。高速。bot 保守不要。
- **Cons:** 人間の記憶に依存。忘れると silent drift。indirect な変更 (walkthrough
  文字列に触れずに output を変える refactor) の検出が無い。

### B. Bot-nudge (provisional recommendation)
bot が plugin MR の diff を走査し、いずれかの example walkthrough で参照されている
skill 名 / interview 文字列 / CLI flag を検出した場合、コメントを投稿する:
"この MR は `perf/` と `tensorrt/` に影響する可能性が高い — pair となる
`apex-examples` MR を起票してください。" detection は自動化、fix は人間。CI gate は無い。

- **Pros:** 自動 detection。安価。CI infrastructure 不要。人間 reviewer が見落とす
  indirect な変更を surface する。
- **Cons:** heuristic — false positive (noise) と false negative (文字列一致で
  拾えない semantic change)。小さな script と bot wiring (例: GitLab webhook や
  examples repo の walkthrough file に対する diff を取る CI job) が必要。

### C. Hard CI gate
marketplace MR を、該当 example の walkthrough を新 plugin に対して実行し expected
output との diff を取る CI で block する。

- **Pros:** 最強の保証。bot が見逃す semantic drift を捕捉する。
- **Cons:** 高コスト — perf は CPU、nsys/tensorrt は CUDA が CI に必要。GPU example
  では infeasible な可能性。遅い。保守が重い。

*Provisional recommendation: **B**。コストが許容でき保証の価値が高い CPU-only example
(perf) については、後で **C** に移行できる。*

---

## 8. Decision B — Examples tagging

*Provisional recommendation: **C — tags only on serve-floor bumps**. ratify 待ち。*

examples リポジトリ自体に git tag を付けるか。

### A. No tags — pure main
`main` のみ。ユーザは常に `main` を clone する。

- **Pros:** 最もシンプル。"always latest" の promise と完全に一致。ceremony ゼロ。
- **Cons:** serve を更新できない/したくないユーザに、整合する examples snapshot への
  rollback 先が無い。

### B. Checkpoint tags on each update
plugin 変更のために `main` が更新されるたびに examples の git tag (例:
`examples-v2026.02` や `examples-v1.0.0`) を cut する。

- **Pros:** 古い serve image に固定されたユーザに recovery point を提供する。
- **Cons:** 実際には古い serve を壊さない routine の walkthrough-refresh 更新で
  tag が増殖し、signal-to-noise が低い。

### C. Tags only on serve-floor bumps (provisional recommendation)
plugin 変更が serve floor を上げる場合 (section 9 による breaking change) のみ
examples を tag する。routine の walkthrough refresh は `main` 上で untagged の
まま float する。

- **Pros:** tag に意味がある (各 tag = "少なくともこの serve が必要")。tag が少ない。
  signal が綺麗。
- **Cons:** floor が実際にいつ動くかを知る必要がある (section 9 の decision に依存)。
  release 毎の思考がやや増える。

*Provisional recommendation: **C** — tag は churn ではなく signal を持つ。section 9 の
adaptive serve-floor policy と自然に組み合わさる (floor が動いた時にちょうど tag が
cut される)。*

---

## 9. Decision C — Serve-floor / lag policy

*Provisional recommendation: **C — adaptive**. ratify 待ち。*

serve floor (`examples/main` を実行するために必要な最低 `apex-marketplace-serve`
image version) をどう決定するか。"どの version が事象を壊すかによる" という直感を
encoding する。

### A. Strict — latest examples needs latest serve
floor = いかなる plugin 変更時点での最新 serve version。`COMPATIBILITY.md` は単に
"≥ latest serve" と書く。

- **Pros:** 最もシンプルな mental model。compatibility の驚きが無い。
- **Cons:** 変更がより新しい serve 機能を必要としない場合でも、breaking な example
  変更のたびにユーザに serve 更新を強要する。

### B. Lenient — one-version backward compat
`main` は serve *N* と *N-1* で動く必要がある。author は毎回 backward compat を
verify し、floor は major 変更時のみ動く。

- **Pros:** 更新が遅い顧客に lenient。
- **Cons:** 毎回の verification 負荷。plugin tag が本当に serve *N* にしか存在しない
  場合 (tag が serve *N-1* 構築後に cut された場合) は不可能な場合がある。

### C. Adaptive — floor moves only when a plugin tag requires it (provisional recommendation)
floor = example が依存する plugin tag を **最初に同梱した** serve version。
plugin 変更が既存 tag の `major.minor` 上の patch (現在の floor image に既に存在)
の場合、floor は動かない。"tag X を最初に同梱した serve image はどれか" の問い合わせ
(GitLab/mirror lookup 1回、または serve release notes に対する小さな script)、
もしくは参照用の古い serve image に対する `apm install` 試行で決定する。

- **Pros:** floor が本当に必要な時のみ動く。ユーザの disruption が最小。
  "どの version が事象を壊すかによる" という直感に直接一致する。
- **Cons:** "first-shipped-in-serve" の lookup が必要 (情報は serve release notes に
  あり、mirror から導出可能 — 安価)。strict よりやや記録事務が増える。

### D. No floor tracked
古い serve が example を壊した場合、bug として報告され、example を直すかユーザに
更新を促す。

- **Pros:** 記録事務ゼロ。
- **Cons:** 最も危険。更新が遅い顧客の silent breakage。proactive な契約が無い。

*Provisional recommendation: **C** — "どの version が事象を壊すかによる" という直感を
直接 encoding する。section 8 option **C** と組み合わさる (examples tag は floor が
動いた時にちょうど cut され、tag に意味を持たせる)。*

---

## 10. `COMPATIBILITY.md` の役割 (上記 decision とは独立)

section 7–9 のいずれの option が ratify されても、root にある `COMPATIBILITY.md`
(public) が契約 surface を記録する:

- **trigger checklist** (section 3) — open-source contributor とユーザが
  example-affecting な変更の定義を見られるように。
- **current serve floor** — `examples/main` を実行するのに必要な最低
  `apex-marketplace-serve` version。ratify された section 9 policy に従う。
- **最新の paired MR への pointer** (`main` を更新した "examples update X ↔
  marketplace MR Y" の短い log)。
- per-example の `apm.yml` `ref` 値 (section 6) が機械可読の source of truth である。
  `COMPATIBILITY.md` は人間可読の要約であり、**refs から生成できる** — 全 example が
  `apm.yml` refs を持つ場合、compatibility table は手動保守ではなく導出可能になる。

本ファイルが public contract であり、本 policy 文書はその *メンテナンス方法* を
govern する。内容は最小限かつ人間可読であり、machine-consumed な lockfile ではない。

---

## 11. Open questions

以下の3つの decision は **provisional** であり、APEX チームが ratify するまで
拘束力を持たない:

| Decision | Section | Provisional recommendation |
|---|---|---|
| Lockstep enforcement | 7 | **B — bot-nudge** |
| Examples tagging | 8 | **C — tags only on serve-floor bumps** |
| Serve-floor policy | 9 | **C — adaptive** |

ratify されるまで、3つの provisional recommendation を作業仮定とする。本ドキュメントは
living reference であり、decision が ratify された時に更新し、section header に
ratify 日付と決定主体を記録する。

追加の decision は ADR (Architecture Decision Record) として
[`docs/adr/`](adr/) 配下に記録する:

| ADR | Status | Title |
|---|---|---|
| [ADR-0001](adr/adr-0001-release-gate.md) | Accepted | Release gate: hybrid nudge at L1, hard gate at L2 tag push |
| [ADR-0002](adr/adr-0002-trigger-flags-role.md) | In consideration | Role of the 5 trigger flags when the gate is on version/tag |
| [ADR-0003](adr/adr-0003-example-version-bump.md) | In consideration | Example's own `version` field bump strategy |
| [ADR-0004](adr/adr-0004-skill-runs-guarantee.md) | In consideration | Guarantee that the skill runs on the new plugin tag |
| [ADR-0005](adr/adr-0005-verification-flow.md) | In consideration | User verification flow documentation |

status が "In consideration" の ADR は ratify まで拘束力を持たない。

---

## Change log

| 日付 | 変更 |
|---|---|
| 2026-08-03 | 初版ドラフト。3つの decision を APEX チームの ratify 待ちとして provisional で残した。 |
| 2026-08-03 | section 6 を追加: per-example `apm.yml` の慣習 (Model A — validated tag を pin し、ユーザは `apm.yml` 経由で install する)。以降の section を renumber。 |
| 2026-08-03 | section 6 に multi-fixture note を追加 (nsys-examples と tensorrt-example の apm.yml を sub-folder level に)。 |
| 2026-08-03 | release gate および open decision (trigger-flags の役割、example version bump、skill-runs guarantee、verification flow) を `docs/adr/` 配下の ADR に移動。section 11 に ADR index を追加。 |
| 2026-08-03 | example フォルダ名を変更 (suffix 削除: `-example`/`-plugin`/`-examples`): `perf-plugin/`→`perf/`、`vectorize-example/`→`vectorize/`、`tensorrt-example/`→`tensorrt/`。`nsys-examples/` を3つの top-level example に分割 (`nsys-py-pipeline/`、`nsys-py-module/`、`nsys-cpp-saxpy/`)。section 6 の multi-fixture table を新構造に更新 (nsys fixture は top-level に移行し、`tensorrt/` のみ multi-module として残る)。 |
| 2026-08-03 | nsys を marketplace package に合わせて再グループ化: 3つの nsys fixture を `nsys-profiler/` 配下の sub-folder に移動 (py-pipeline, py-module, cpp-saxpy)。`nsys-analyzer/` を作成し placeholder profiles を置いた。nsys-profiler と tensorrt はどちらも all-in-1 hub README pattern を使用（Installation/Usage/walkthrough を持つ単一 README。module sub-folder は README なし）。全 leaf example (perf、4つの tensorrt module、3つの nsys fixture) に `apm.yml` を追加し vectorize の ref を修正。`docs/lint-examples.sh` を追加し以下を enforce: README section (Installation, Usage)、apm.yml の存在、apm.yml ref 形式 (`<pkg>-v<semver>` に一致)、旧 folder 名の不存在。section 6 に README 構造 criterion と lint 参照を追加。 |
