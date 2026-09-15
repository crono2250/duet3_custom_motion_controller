# Third-party source ledger

| component | pinned revision | license status | integration |
|---|---|---|---|
| Duet3Expansion 3.6.3 | `9d9497bd656fed7302cf132c61ca1eabbdd65e06` | GPL-3.0-onlyとして扱う | `firmware/duet/`への移植元 |
| CANlib 3.6.3 | `f0a4c6d53d0cb3ccff8cc4309c2b4b67e158efab` | **明示license file未確認** | 確認完了までsourceを取り込まない |
| CMSIS_5 5.9.0 | `2b7495b8535bdcb306dac29b9ded4cfb679d7e5c` | Apache-2.0 | configure時に取得 |
| CMSIS Device G4 v1.2.6 | `25664ddc3a7624ae9627ae8c4c672073dc5b2539` | Apache-2.0 | configure時に取得 |
| STM32G4 HAL v1.2.6 | `6940aef00ac466305872e4153f41b52dc954d6cf` | BSD-3-Clause | configure時に取得 |

`license status`が未確認のcomponentは、公開repositoryであることだけを根拠にcopy、modify、redistributeしません。source取込み時は元fileのnoticeと、このledgerのrevisionを同時に更新します。
