# Duet 3 custom motion controller

Duet 3 Expansion 1XDをリファレンスに、OpenPnP向け5軸STEP/DIR・リモートI/OノードをSTM32へ移植するためのリポジトリです。第一ターゲットは **STM32G431CBT6** です。

現時点では、STM32G431CBT6用のクロスビルド環境と最小bring-upイメージを提供します。Duet CANプロトコル、FDCAN通信、時刻同期、STEP生成はまだ実装していません。この成果物をDuet Expansion Board互換ファームウェアとして実機運転しないでください。

## CIで確認していること

- Arm GNU Toolchain 15.2.Rel1によるCortex-M4F/hard-floatビルド
- 公式CMSIS G4のG431 startupとsystem初期化コードのコンパイル
- STM32G431CBT6の128KiB Flash / 32KiB SRAMリンカ配置
- G431用FDCAN1、TIM1、TIM8定義の存在
- ELF、BIN、HEX、MAP、サイズ情報の生成
- ベクタテーブルが`0x08000000`に配置され、必須シンボルを含むこと

GitHub Actionsは`main`へのpush、Pull Request、手動実行で起動し、`openpnp-motion-stm32g431cbt6`という成果物を14日間保存します。

## ローカルビルド

必要なツールはGit、CMake 3.24以上、Ninja、Arm GNU Toolchainです。`arm-none-eabi-gcc`が`PATH`から参照できる状態で実行します。

```console
cmake --preset ci-release
cmake --build --preset ci-release
```

生成物は`build/ci-release/artifacts/`に出力されます。初回configure時に、固定した公式CMSISリポジトリを取得します。ネットワークを使わない場合は、既存checkoutを指定できます。

```console
cmake --preset ci-release -DCMSIS_CORE_ROOT=/path/to/CMSIS_5 -DCMSIS_DEVICE_G4_ROOT=/path/to/cmsis-device-g4
cmake --build --preset ci-release
```

## 現在の評価

STM32G431への移植は、FDCANとタイマ資源の観点では実行可能です。ただしSTM32G431CBT6はFlashが128KiBしかなく、既存1XD 3.6.3バイナリが119,428 bytesであるため、1XDと同じ16KiBのbootloader領域を仮定すると現状の1軸版さえ収まりません。機能を絞った1軸PoCを先に成立させ、サイズ実測後にG474等への変更要否を判断します。

調査根拠、固定した上流revision、未解決事項は[移植性評価](docs/porting-assessment.md)を参照してください。

## ライセンス

このリポジトリ固有のコードは[MIT License](LICENSE)です。ビルド時に取得するCMSIS_5およびSTM32 CMSIS Device G4はApache-2.0です。将来Duet3Expansion/CANlibのGPL-3.0コードを組み込んだ結合成果物にはGPL-3.0の条件が適用されます。
