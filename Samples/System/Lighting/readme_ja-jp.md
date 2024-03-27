![](./media/image1.png)

# LampArray サンプル

*このサンプルは、Microsoft Game Development Kit と互換性があります (2023 年 3 月 QFE1)*

# 説明

このサンプルでは、LampArray API を使用して、キーボードやマウスなどの RGB デバイスでライトを操作する方法を示します。

> **ご注意ください:** 2023 年 3 月 QFE1 のリリース時点では、GDK LampArray API はコンソールで次のデバイスのみをサポートしています。 今後のリカバリ リリースでは、追加のデバイスのサポートが追加される予定です。
> - Xbox One 用 Razer Turret (キーボードとマウス)
> - Razer BlackWidow Tournament Edition Chroma V2

# サンプルのビルド

Xbox One 開発キットを使用している場合は、アクティブなソリューション プラットフォームを Gaming.Xbox.XboxOne.x64 に設定します。

| | |
|---|---|
| Xbox Series X を使用している場合 | S 開発キット、アクティブなソリューション プラットフォームを Gaming.Xbox.Scarlett.x64 に設定します。 |

*詳細については、**GDK ドキュメント*の「__サンプルの実行__」を参照してください。

# サンプルの使用方法

互換性のあるデバイスが接続されていることを確認します。  キーボードの矢印キーまたはゲームパッドの DPad を使用して、サンプル効果間を移動します。

Esc キーまたは [表示] ボタンを押して終了します。

# 実装メモ

効果の実装は、`LightingEffects.cpp` ファイルにあります。  コールバックとその他の機能は、`Lighting.cpp` ファイル内にあります。

# プライバシーにかんするせいめい

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプルの実行可能ファイルのファイル名が Microsoft に送信されます。 このデータ コレクションからオプトアウトするには、Main.cpp の "サンプル使用状況テレメトリ" というラベルの付いたコードのブロックを削除します。

Microsoft のプライバシー ポリシー全般の詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。


