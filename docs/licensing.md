# ライセンス境界と配布方針

この文書はプロジェクト内の運用方針であり、個別案件への法的助言ではありません。製品販売や顧客へのbinary配布を始める前に、必要に応じて専門家の確認を受けてください。

## 採用する境界

| 範囲 | file単位のlicense | 用途 |
|---|---|---|
| `firmware/include/`、`firmware/src/` | MIT | STM32 platform、FDCAN、motion clock、汎用STEP実行器 |
| `cmake/`、`tests/`、`docs/` | MIT | build、検証、文書 |
| `firmware/duet/` | GPL-3.0-only | Duet3Expansionからコピーまたは派生したapplication／protocol層 |
| link済みfirmware image | GPL-3.0-only | MIT層とGPL層を含む結合著作物 |

MITはGPLv3と組み合わせ可能です。結合imageをGPL-3.0-onlyで配布しても、自作MIT fileの受領者に与えたMITの利用許諾は失われません。したがって、汎用STM32層を別製品でMITとして再利用できます。

## 汚染ではなく混入を防ぐ運用

1. Duet由来codeは`firmware/duet/`以外へコピーしない。
2. Duet由来fileのcopyright、license notice、取得元repository、tag、commitを保持する。
3. MIT側との境界には、このprojectで新規設計した小さなadapter APIだけを置く。上流のclassや内部data structureをMIT headerへ複製しない。
4. 各fileのlicenseを`REUSE.toml`またはfile内SPDX headerで機械可読にする。上流fileに既存noticeがある場合は削除しない。
5. `reuse lint`をCI必須checkにし、license不明fileやlicense text不足をmerge前に検出する。
6. 依存関係はrevisionとlicenseを`THIRD_PARTY.md`へ記録する。licenseが確認できないsourceは取り込まない。
7. release tagごとに、binaryと同じrevisionの完全な対応source、build script、linker script、toolchain情報を保存する。

## 分離できないもの

同じSTM32 firmware imageへstatic linkしたMIT層とGPL層は、directoryやlibrary targetを分けても配布上は一つのprogramです。境界分割はMIT fileの再利用性と出所管理には有効ですが、結合binaryをMITまたはproprietaryにする手段ではありません。

GPLの影響を結合imageへ及ぼさない強い境界が必要な場合は、別processまたは別MCU上の独立programとして、一般的なwire protocol越しに通信させる設計が候補です。このPoCは単一MCU／単一imageなので採用しません。

## Binary配布時のchecklist

- GPL-3.0-only全文とcopyright noticeを同梱する。
- 対応sourceをbinaryと同時提供するか、GPLv3が認める方法で有効なsource提供を行う。
- 再現buildに必要なscript、interface定義、設定、変更内容を対応sourceへ含める。
- ST、Arm、その他third-party componentのnoticeとlicense条件も満たす。
- 製品がGPLv3上のUser Productに該当する場合、変更版をinstall・実行するために必要なInstallation Informationを検討する。
- source archiveからnetworkなしで依存関係とlicense一覧を監査できるrelease工程を用意する。

## License identifierの選択

Duet3Expansion repositoryにはGPL Version 3のlicense本文がありますが、project固有の明確な「or later」許諾を確認できないため、上流由来fileと結合imageには保守的に`GPL-3.0-only`を使用します。著作権者から別の明示許諾が確認できた場合にだけ変更します。
