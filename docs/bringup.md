# STM32G431単軸PoC bring-up

## 現在の起動シーケンス

1. HSI16とPLLからsystem clockを168MHzへ設定する。
2. TIM2を32-bit free-running counter、750kHzで開始する。
3. build modeに応じてFDCAN1を内部ループバックまたは外部normal mode、nominal 1Mbit/s、data 3Mbit/sで開始する。
4. loopback版はExtended IDのCAN-FD/BRS frameを送受信し、payloadとRX timestampを検査する。
5. normal版はRX FIFO0 interrupt、4-frame software queue、単軸GPIOを初期化する。driverは無効状態を維持する。
6. 成功時は`BRINGUP_STAGE_READY`、失敗時は`BRINGUP_STAGE_FAULT`で待機する。

168MHzを選んだ理由は、G431の上限170MHzに近く、TIM2を整数分周（168MHz / 224）してDuetと同じ750kHz motion clockを正確に生成できるためです。

## SWDで確認する値

`g_bringup_diagnostics`をwatch expressionへ登録します。`magic`が`0x47343331`、`version`が`1`であることを最初に確認してください。

| field | 正常値または意味 |
|---|---|
| `stage` | `5` (`BRINGUP_STAGE_READY`) |
| `system_clock_hz` | `168000000` |
| `motion_clock_hz` | `750000` |
| `fdcan_result` | `0` (`FDCAN_PORT_OK`) |
| `fdcan_mode` | `0`: loopback、`1`: normal |
| `fdcan_receive_timestamp` | frame受信開始時の16-bit timestamp |
| `fdcan_timestamp_after_receive` | CPUがframeを取り出した時点のtimestamp |

`fdcan_result`が0以外の場合は`fdcan_port_result_t`を参照してください。内部ループバックではCAN transceiverや外部pinを使用しません。

単軸runtimeの状態は`g_axis_runtime_diagnostics`で確認できます。normal版でも起動直後の`enabled`はfalseで、STEPはlowです。

外部CANの受信数、software queue overflow、送受信エラーは`g_fdcan_port_diagnostics`で確認できます。CAN-FD payload長はDLCで表現可能な0〜8、12、16、20、24、32、48、64 byteだけを送信APIが受け付けます。Classic CANは8 byte以下かつBRSなしに制限します。

`axis_runtime_enqueue()`はdriverが無効な間は`AXIS_RUNTIME_DISABLED`を返します。disableまたはemergency stop時は予約済みcompareをキャンセルし、STEP low、queue破棄、ENABLE無効を保証します。

## この段階で意図的に未接続のもの

- TIM1/TIM8によるhardware pulse生成
- Duet extended CAN ID filterとmessage decoder
- 実際のtime-sync messageから`clock_sync_observe()`を呼ぶ受信経路

normal版のFDCANとSTEP/DIR/ENABLEは暫定ピン契約で接続済みです。loopback版は未確定pinへ信号を出しません。TIM1 hardware pulse化とDuet message接続は、pin contract確定後に行います。

## 次の実機合格条件

- SWDで`BRINGUP_STAGE_READY`へ到達する。
- TIM2の増分を複数回観測し、750kHz相当であることを確認する。
- 内部ループバックのRX timestamp差分が正で安定している。
- pin assignment確定後、normal modeで1M/3M CAN-FD frameを1XDまたはmain boardと送受信する。
