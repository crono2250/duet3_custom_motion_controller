# Duet 3 custom motion controller

Duet 3 Expansion 1XDをリファレンスに、OpenPnP向け5軸STEP/DIR・リモートI/OノードをSTM32へ移植するためのリポジトリです。第一ターゲットは **STM32G431CBT6** です。

現時点では、STM32G431CBT6用のクロスビルド環境と単軸PoCのbring-upイメージを提供します。168MHz system clock、750kHz motion clock、FDCAN内部ループバック／外部normal mode、時刻同期フィルタ、単軸STEP/DIR/ENABLE runtimeまで実装済みです。Duet CAN message decoderはまだ未実装なので、この成果物をDuet Expansion Board互換ファームウェアとして実機運転しないでください。

## CIで確認していること

- Arm GNU Toolchain 15.2.Rel1によるCortex-M4F/hard-floatビルド
- 公式CMSIS G4とSTM32G4 HAL v1.2.6の固定revisionによるビルド
- STM32G431CBT6の128KiB Flash / 32KiB SRAMリンカ配置
- 168MHz system clockとTIM2による32-bit・750kHz motion clock
- FDCAN 1Mbit/s nominal、3Mbit/s data phaseの内部ループバック設定
- 外部FDCAN割り込み受信queueとCAN-FD送信APIを含むnormal-mode build
- 16-bit FDCAN timestampからmotion tickへのwrap-around対応変換
- PA8 STEP、PB0 DIR、PB1 ENABLEによる単軸runtime（暫定pin contract）
- 時刻同期、STEP queue、protocol-neutral motion gatewayのnative host単体テスト
- ELF、BIN、HEX、MAP、サイズ情報の生成
- ベクタテーブルが`0x08000000`に配置され、割り込みと診断シンボルを含むこと

GitHub Actionsは`main`へのpush、Pull Request、手動実行で起動し、loopback版とnormal版を`openpnp-motion-stm32g431cbt6`という成果物へまとめて14日間保存します。normal版は[暫定ピン契約](docs/pin-contract.md)と実回路が一致するまで書き込まないでください。

## ローカルビルド

必要なツールはGit、CMake 3.24以上、Ninja、Arm GNU Toolchainです。`arm-none-eabi-gcc`が`PATH`から参照できる状態で実行します。

```console
cmake --preset ci-release
cmake --build --preset ci-release

# 外部FDCANと単軸GPIOを含むnormal版
cmake --preset normal-release
cmake --build --preset normal-release
```

生成物はloopback版が`build/ci-release/artifacts/`、normal版が`build/normal-release/artifacts/`に出力されます。初回configure時に、固定した公式CMSISリポジトリを取得します。ネットワークを使わない場合は、既存checkoutを指定できます。

```console
cmake --preset ci-release -DCMSIS_CORE_ROOT=/path/to/CMSIS_5 -DCMSIS_DEVICE_G4_ROOT=/path/to/cmsis-device-g4 -DSTM32G4_HAL_ROOT=/path/to/stm32g4xx-hal-driver
cmake --build --preset ci-release
```

native環境でmotion coreの単体テストだけを実行する場合は次の通りです。

```console
cmake -S tests -B build/host-tests -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/host-tests
ctest --test-dir build/host-tests --output-on-failure
```

実機bring-up時の診断方法と合格条件は[bring-upガイド](docs/bringup.md)、現在のLQFP48割当は[暫定ピン契約](docs/pin-contract.md)を参照してください。

## License

汎用STM32／motion-core fileはMITを維持し、Duet由来層は`firmware/duet/`へ隔離してGPL-3.0-onlyで管理します。両者をlinkしたfirmware image全体はGPL-3.0-onlyです。詳細は[ライセンス境界と配布方針](docs/licensing.md)、依存元は[third-party source ledger](THIRD_PARTY.md)を参照してください。

## 現在の評価

STM32G431への移植は、FDCANとタイマ資源の観点では実行可能です。ただしSTM32G431CBT6はFlashが128KiBしかなく、既存1XD 3.6.3バイナリが119,428 bytesであるため、1XDと同じ16KiBのbootloader領域を仮定すると現状の1軸版さえ収まりません。機能を絞った1軸PoCを先に成立させ、サイズ実測後にG474等への変更要否を判断します。

調査根拠、固定した上流revision、未解決事項は[移植性評価](docs/porting-assessment.md)を参照してください。

## ライセンス

このリポジトリ固有のコードは[MIT License](LICENSE)です。ビルド時に取得するCMSIS_5およびSTM32 CMSIS Device G4はApache-2.0、STM32G4 HALはBSD-3-Clauseです。将来Duet3Expansion/CANlibのGPL-3.0コードを組み込んだ結合成果物にはGPL-3.0の条件が適用されます。
