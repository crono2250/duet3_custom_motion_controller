# STM32G431CBT6暫定ピン契約 v1

この割当はFirmwareのnormal-mode経路をコンパイル・検証するための暫定値です。実回路との一致が確認されるまでnormal版を書き込まないでください。loopback版はこれらの外部pinを設定しません。

## LQFP48割当

| signal | MCU pin | package pin | mode | 起動時状態 |
|---|---|---:|---|---|
| FDCAN1_RX | PA11 | 33 | AF9 | input |
| FDCAN1_TX | PA12 | 34 | AF9 | recessive |
| CAN_STANDBY | PB2 | 18 | GPIO output | high後、FDCAN開始時low |
| AXIS0_STEP | PA8 | 30 | GPIO output | low |
| AXIS0_DIR | PB0 | 16 | GPIO output | low |
| AXIS0_ENABLE | PB1 | 17 | GPIO output | high（driver無効） |

PA11/PA12のFDCAN1 alternate functionとPA8のTIM1_CH1対応はSTM32G431xB datasheet DS12589 Rev 6のpin definition／alternate-function表を根拠としています。PA11/PA12をCANへ割り当てるため、同じpinを使うUSB data pairとは排他です。

## Firmwareが仮定している電気仕様

- MCU I/O電圧は3.3V。
- CAN transceiver standbyはactive-highで、lowにすると通常動作へ入る。
- stepper driver enableはactive-low。
- STEPはactive-high、DIR highをpositive directionとする。
- 起動中とfault時はSTEP low、ENABLE highを維持する。

上記の極性が回路と異なる場合は、実機へ書き込む前に`firmware/include/board_pins.h`を変更します。

## 現在のSTEPタイミング方式

PA8は将来TIM1_CH1へ移行できるpinですが、現在のPoCはTIM2 compare interruptからGPIO BSRRへ書き込みます。これにより時刻同期からSTEP edgeまでの経路を先に検証できます。ISR遅延を測定した後、必要ならTIM1 output compareまたはDMAへ移行します。
