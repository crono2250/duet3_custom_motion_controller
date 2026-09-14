# STM32G431CBT6移植性評価

評価日: 2026-09-15

## 結論

STM32G431CBT6でDuet Expansion互換ノードを試作することは、CPU・FDCAN・タイマ機能の面では可能です。ただし、**5軸対応の完成版を128KiB Flashへ収められるかは未確認で、現時点の最大リスク**です。まず1軸・最小I/Oに絞ったPoCでCAN列挙、時刻同期、motion executorを成立させ、コードサイズと割り込み遅延を測定する必要があります。

今回のCIはMCU向けコンパイラ、startup、リンカ、CMSIS peripheral contractが成立することを検証します。Duet CAN互換性や実時間性能を証明するものではありません。

## 固定した調査ベースライン

| 対象 | revision | 用途 |
|---|---|---|
| Duet3Expansion 3.6.3 | `9d9497bd656fed7302cf132c61ca1eabbdd65e06` | 1XD安定版の実装基準 |
| CANlib 3.6.3 | `f0a4c6d53d0cb3ccff8cc4309c2b4b67e158efab` | 安定版CAN message contract |
| Duet3Expansion `3.7-stm` | `d3ab5eb38fbae68319cae8dd6235ac7a420843d8` | 上流STM32H5実験実装の参考 |
| CMSIS_5 5.9.0 | `2b7495b8535bdcb306dac29b9ded4cfb679d7e5c` | Cortex-M4 core headers |
| CMSIS Device G4 v1.2.6 | `25664ddc3a7624ae9627ae8c4c672073dc5b2539` | G431 device headers/startup/system |

Duet3Expansion/CANlibはCIへまだリンクしていません。まずライセンス境界とSTM32プラットフォームAPIを明確にした後、上記revisionを基準に段階移植します。

## コード調査で確認した事実

### 再利用できる上位ロジック

- `NumDrivers`、STEP/DIR/ENABLE pin、driver stateは配列化されており、複数driverを扱う構造が既にあります。
- `CanInterface`は受信した`movementLinearShaped`をqueueへ送り、同期成立後に実行時刻を検査します。
- `StepTimer::ProcessTimeSyncMessage`はCAN受信timestampから処理遅延を補正し、master-local offsetをPI型補正しています。
- motion clockは750kHz（48MHz / 64）です。STM32側でも同じtick domainを維持すれば上位計算の変更を減らせます。

### 新規実装または移植が必要な層

- STM32G4用FDCAN driverと受信timestamp取得
- 32-bit free-running 750kHz motion clock、compare IRQ、wrap-around処理
- 5軸のSTEP pulse生成とGPIO atomic set/reset
- GPIO、EXTI、DMA、NVIC、Flash/NVM、watchdog、fault/reset処理
- FreeRTOSおよびDuet共通ライブラリのSTM32G4 build configuration
- STM32向けCAN bootloaderおよびfirmware update方式
- LQFP48で成立するalternate-function/pin配置

上流の`3.7-stm`ブランチにはSTM32H5向けplatform/FDCAN/NVMコードがあります。STM32という大枠の設計参考にはなりますが、H5とG4はclock tree、FDCAN message RAM、DMA、Flash、割り込みが異なるため、そのままG431用にはビルドできません。

## メモリ評価

- STM32G431CBT6: Flash 131,072 bytes、SRAM 32,768 bytes。
- RepRapFirmware 3.6.3配布の`Duet3Firmware_EXP1XD.bin`: 119,428 bytes。
- 単純差分は11,644 bytesです。
- 4KiBのbootloader領域を仮定してもapplication余裕は7,548 bytesしかありません。
- Duet3Expansion 3.6.3のEXP1XD buildが使う16KiB予約を適用すると、application領域は114,688 bytesとなり、現行1XDバイナリだけで4,740 bytes超過します。

さらに5軸分のstate/queue、STM32 HALまたはLL層、専用bootloaderが必要です。このため「既存1XD firmwareを機能そのままで移す」方針は危険です。PoCでは不要なheater、sensor、smart-driver等をbuild-timeに除外し、MAPファイルでサイズを継続監視します。完成要件が128KiBを超える場合は、Flash容量の大きいSTM32G4品種を選び直します。

なお、CBT6（LQFP48）と引き継ぎ資料で候補だったRBT6（LQFP64）はFlash/SRAM容量が同じです。RBT6は主にpin配置余裕を改善しますが、Flash問題は解消しません。

## 段階的な合格条件

1. 本CIでG431のELF/BIN/HEXを再現生成できる。
2. 評価基板でFDCAN nominal/data phase通信とRX timestampを測定できる。
3. 750kHzのlocal motion clockを実装し、1XDとclock sync収束値を比較できる。
4. remote GPIO 1入力/1出力がRRFから操作できる。
5. remote driver 1軸のSTEP/DIRを1XDと比較し、開始時刻・周期・加減速波形が一致する。
6. Flash/SRAM/ISR負荷に余裕がある場合のみ5軸へ拡張する。
7. bootloader partitionとCAN updateを確定し、CI linker budgetへ反映する。

## 参照先

- [Duet3Expansion](https://github.com/Duet3D/Duet3Expansion)
- [CANlib](https://github.com/Duet3D/CANlib)
- [RepRapFirmware 3.6.3 release](https://github.com/Duet3D/RepRapFirmware/releases/tag/3.6.3)
- [STM32G431CB product page](https://www.st.com/en/microcontrollers-microprocessors/stm32g431cb.html)
- [STM32G431xB datasheet](https://www.st.com/resource/en/datasheet/stm32g431cb.pdf)
- [CMSIS Device G4](https://github.com/STMicroelectronics/cmsis-device-g4)
- [CMSIS_5](https://github.com/ARM-software/CMSIS_5)
