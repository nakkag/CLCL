CLCL インストーラ
--

■ 概要
CLCL のインストーラと、インストーラを作成するツールです。

インストーラは C のみで記述しており、Windows の API 以外のライブラリは使用して
いません。ZIP の展開処理 (deflate) もインストーラ内に実装しているため、実行時に
必要なランタイムやライブラリはありません。

■ ファイル
  Installer.vcxproj インストーラのプロジェクト (CLCL.sln に含まれる)
  build.ps1         インストーラを作成する PowerShell スクリプト
  prepare.ps1       ファイルの収集と ZIP の作成 (プロジェクトから呼ばれる)
  Installer.c       インストーラ本体
  Installer.h       インストーラの定義
  Installer.rc      リソース (ダイアログ / 各言語の文字列 / ZIP)
  resource.h        リソース ID
  unzip.c           ZIP の展開 (deflate と CRC32 の実装)
  unzip.h           ZIP の展開の定義
  res/manifest.xml  マニフェスト (管理者権限で実行)

■ インストーラの作成
Visual Studio 2017 以降 (C++ デスクトップ開発) と Windows SDK が必要です。
PowerShell で以下を実行すると、CLCL 本体のビルドからインストーラの作成までを
一括で行います。

  PS> cd Installer
  PS> .\build.ps1

作成したインストーラは Installer\out に出力されます。
出力するファイル名は clcl<バージョン>.exe です。Ver 2.2.0 なら clcl220.exe に
なります。バージョンは CLCL.rc の FILEVERSION から取得します。
インストーラに含まれるのは Release 構成でビルドした CLCL.exe などです。

ビルド済みのファイルからインストーラだけを作成する場合は -SkipBuild を指定しま
す。

  PS> .\build.ps1 -SkipBuild

主なオプション
  -Configuration <構成>   ビルド構成 (既定: Release)
  -Platform <プラットフォーム>
                          ビルドするプラットフォーム (既定: x86)
  -Version <バージョン>   バージョン (既定: CLCL.rc の FILEVERSION)
  -SourceDir <フォルダ>   収集元のフォルダ (既定: <リポジトリ>\<ビルド構成>)
  -OutDir <フォルダ>      出力先のフォルダ (既定: Installer\out)
  -SkipBuild              CLCL 本体のビルドを行わない

■ Visual Studio でのビルド
Installer.vcxproj は CLCL.sln に含まれています。ソリューションをビルドすると、
CLCL 本体のビルドの後にインストーラが作成されます。Installer プロジェクトだけを
ビルドすることもできます。その場合は CLCL 本体をビルド済みにしておいてください。

ビルド前のイベントで prepare.ps1 がファイルの収集と ZIP の作成、バージョン情報の
instinfo.h の生成を行い、ビルド後のイベントで prepare.ps1 が Installer\out へ
clcl<バージョン>.exe として出力します。build.ps1 はこのプロジェクトを msbuild で
ビルドしているだけなので、どちらの方法でも結果は同じです。

Installer\out に出力するのは Release 構成のときだけです。Debug 構成でビルドした
インストーラは Installer\Debug\CLCLInst.exe になります。Debug 構成では Debug 構成
の CLCL.exe などを収集するため、配布するインストーラと混ざらないようにしています。

プロジェクトの設定は msbuild のプロパティで変更できます。build.ps1 のオプション
はこれらに対応しています。フォルダは末尾に \ を付けないでください。
  ClclVersion    バージョン
  ClclSourceDir  インストールするファイルの収集元
  ClclOutDir     インストーラの出力先

インストーラのバージョン情報はビルド時に生成する instinfo.h で決まります。この
ファイルはリポジトリには含めていません。

■ インストールするファイル
prepare.ps1 が以下のファイルを収集して ZIP にまとめ、インストーラのリソース
(RCDATA) として登録します。インストーラは実行時にこの ZIP を展開して配置します。

  CLCL.exe        Release フォルダから収集
  CLCLSet.exe     Release フォルダから収集
  readme_jp.txt   リポジトリのルートから収集
  readme_en.txt
  readme_de.txt
  readme_uk.txt
  readme_zh.txt
  LICENSE.txt     リポジトリの LICENSE から収集

clcl_app.ini は収集しません。ファイルが無い場合は portable=0 として動作するため、
更新時に利用者の設定を上書きしないようにしています。

収集するファイルを変更する場合は prepare.ps1 の Get-TargetFiles を編集してくださ
い。

■ インストーラの動作
管理者権限で実行します (マニフェストで requireAdministrator を指定)。

・インストール先を選択できます。
  既定は %ProgramFiles%\CLCL です。既にインストールされている場合は、登録されて
  いるインストール先が既定になります。
・スタートアップ、スタートメニュー、デスクトップへのショートカットの作成を選択
  できます。ショートカットはすべてのユーザ用の場所に作成します。
・コントロールパネルの「プログラムと機能」(アプリと機能) に登録します。
・既にインストールされている場合は更新します。
  登録済みのアプリの一覧のキーをそのまま使用するため、二重に登録されません。
・CLCL が起動中でファイルを上書きできない場合は、CLCL の終了を促します。
・インストール後に CLCL を起動するかどうかを確認します。起動する場合は管理者権
  限を引き継がないように、エクスプローラ経由で起動します。

■ アンインストール
コントロールパネルから実行できます。
インストーラに引数を指定して実行することもできます。

  clcl220.exe /uninstall

インストール先に配置される uninstall.exe も同じ動作です。

  "C:\Program Files (x86)\CLCL\uninstall.exe" /uninstall

インストールしたファイルとインストール時に作成したフォルダの一覧はインストール先
の uninstall.dat に記録しており、アンインストールではこの一覧のファイルとショート
カット、空になったフォルダを削除します。一覧に書かれていても、インストール先の配
下にあるファイルと、スタートアップ / スタートメニュー / デスクトップの直下にある
CLCL*.lnk 以外は削除しません。
設定と履歴のデータ (%LOCALAPPDATA%\CLCL) を削除するかどうかは実行時に確認します。
アンインストーラは管理者権限で動作するため、削除されるのは昇格に使用したアカウント
の %LOCALAPPDATA%\CLCL です。標準ユーザーが別の管理者アカウントの資格情報で昇格し
た場合は、その管理者アカウントのデータが対象になり、ログオン中のユーザーのデータは
残ります。

■ アプリの一覧への登録
以下のいずれかのキーの配下に、表示名が「CLCL」で始まるキーがあるかを検索し、
見つかった場合はそのキーをそのまま使用します。

  HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Uninstall
    (32bit ビューと 64bit ビューの両方)
  HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Uninstall

見つからない場合は以下に新規に作成します。

  HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Uninstall\CLCL
    (32bit のため、64bit 環境では WOW6432Node の配下になります)

これは Ver 2.1 系のインストーラが使用しているキーと同じ場所のため、既存の環境に
上書きしても登録が二重になることはありません。

登録する値
  DisplayName, DisplayVersion, DisplayIcon, Publisher, URLInfoAbout,
  InstallLocation, UninstallString, InstallDate, NoModify, NoRepair,
  EstimatedSize, VersionMajor, VersionMinor

値を設定する前にキーの値をすべて削除します。以前のインストーラが設定した値
(別のアンインストーラを指す UnInstallString など) が残ると、コントロールパネル
から古いアンインストーラが呼ばれてしまうためです。

以前のインストーラがインストール先に残すファイルの一覧 (install.DAT) がある場合
は、アンインストール時に削除するように記録します。インストール先のフォルダが残
らないようにするためです。

既存のインストーラが別のキー名を使用している場合は、Installer.h の
UNINSTALL_SUBKEY を変更してください。既存の登録が見つかった場合はキー名に関わら
ずそのキーを使用するため、通常は変更する必要はありません。

■ 多言語対応
CLCL 本体と同じ 5 言語のリソースを持っています。

  日本語 / 英語 / ドイツ語 / 中国語 (簡体字) / ウクライナ語

Windows の UI 言語に合わせて自動的に切り替わります。リソースが無い言語の場合は
英語で表示します。ダイアログのレイアウトは言語に依存しないようにしており、表示
する文字列はすべて実行時に文字列リソースから設定しています。
翻訳を修正する場合は Installer.rc の STRINGTABLE を編集してください。

■ ライセンス
CLCL 本体と同じライセンスです。LICENSE を参照してください。
