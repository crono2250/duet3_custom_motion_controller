# STM32G431単軸PoC bring-up

## 現在の起動シーケンス

1. HSI16とPLLからsystem clockを168MHzへ設定する。
2. TIM2を32-bit free-running counter、750kHzで開始する。
3. FDCAN1を内部ループバック、nominal 1Mbit/s、data 3Mbit/sで開始する。
4. Extended IDのCAN-FD/BRS frameを送受信し、payloadとRX timestampを検査する。
5. 成功時は`BRINGUP_STAGE_READY`、失敗時は`BRINGUP_STAGE_FAULT`で待機する。

168MHzを選んだ理由は、G431の上限170MHzに近く、TIM2を整数分周（168MHz / 224）してDuetと同じ750kHz motion clockを正確に生成できるためです。

## SWDで確認する値

`g_bringup_diagnostics`をwatch expressionへ登録します。`magic`が`0x47343331`、`version`が`1`であることを最初に確認してください。

| field | 正常値または意味 |
|---|---|
| `stage` | `5` (`BRINGUP_STAGE_READY`) |
| `system_clock_hz` | `168000000` |
| `motion_clock_hz` | `750000` |
| `fdcan_result` | `0` (`FDCAN_LOOPBACK_OK`) |
| `fdcan_receive_timestamp` | frame受信開始時の16-bit timestamp |
| `fdcan_timestamp_after_receive` | CPUがframeを取り出した時点のtimestamp |

`fdcan_result`が0以外の場合は`fdcan_loopback_result_t`を参照してください。内部ループバックではCAN transceiverや外部pinを使用しません。

## この段階で意図的に未接続のもの

- FDCAN TX/RX alternate function pinとtransceiver standby
- STEP、DIR、ENABLE GPIO
- TIM1/TIM8によるhardware pulse生成
- Duet extended CAN ID filterとmessage decoder
- 実際のtime-sync messageから`clock_sync_observe()`を呼ぶ受信経路

これらは回路図とpin assignmentを確定してから接続します。未確定のpinへ信号を出さないため、現イメージは外部GPIOを設定しません。

## 次の実機合格条件

- SWDで`BRINGUP_STAGE_READY`へ到達する。
- TIM2の増分を複数回観測し、750kHz相当であることを確認する。
- 内部ループバックのRX timestamp差分が正で安定している。
- pin assignment確定後、normal modeで1M/3M CAN-FD frameを1XDまたはmain boardと送受信する。
