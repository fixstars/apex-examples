# Build & Run (vectorize)

## 題材コード概要

3D点群のポーズアライメント（姿勢整合性）を評価する処理のデモです。
10万点規模の3Dベクトルペアに対し、クオータニオンで回転した後のコサイン類似度を計算し、平均・分散・最小最大などの統計量を求めます。
さらに数値微分により最適な回転クオータニオンを探索する処理も含まれてSIMD化の題材としてループ処理が豊富に存在します。

## プログラム動作確認 (人間用)

### x86_64

```sh
mkdir build-x86_64
cmake -B build-x86_64/ .
cmake --build build-x86_64/
./build-x86_64/test/bench
```

### aarch64

※ 以下、 drive orin 向けの動作確認方法です。（ビルド環境を調整したら、おおかたのaarch64環境で動くと思います。）

docker run を使って動かします。
drive os sdk で以下のイメージを保有していることが必要です。

- nvcr.io/drive/driveos-sdk/drive-agx-orin-linux-aarch64-sdk-build-x86:latest

```sh
mkdir build-aarch64
./docker-aarch64-cmd.sh cmake -B build-aarch64/ -DCMAKE_TOOLCHAIN_FILE=../toolchain-aarch64.cmake .
./docker-aarch64-cmd.sh cmake --build build-aarch64
REMOTE_USER=remote-user REMOTE_HOST=my-drive-orin ./send-and-run.sh build-aarch64/test/bench
```
