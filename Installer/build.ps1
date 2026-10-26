#Requires -Version 5.1
<#
.SYNOPSIS
	CLCL のインストーラを作成します。

.DESCRIPTION
	CLCL 本体のビルド、インストールするファイルの収集、ZIP の作成、
	インストーラのビルドまでを一括で行います。
	出力するファイル名は clcl<バージョン>.exe になります。(Ver 2.2.0 なら clcl220.exe)

	インストールするファイルの収集と ZIP の作成は、Installer.vcxproj の
	ビルド前のイベントから prepare.ps1 が行います。Visual Studio で
	Installer プロジェクトをビルドした場合も同じ手順になります。

.EXAMPLE
	PS> .\build.ps1
	CLCL 本体をビルドしてからインストーラを作成します。

.EXAMPLE
	PS> .\build.ps1 -SkipBuild
	ビルド済みの Release フォルダのファイルからインストーラを作成します。

.EXAMPLE
	PS> .\build.ps1 -Version 2.2.1 -OutDir C:\temp
	バージョンと出力先を指定してインストーラを作成します。
#>
param(
	# ビルド構成
	[string]$Configuration = "Release",
	# ビルドするプラットフォーム
	[string]$Platform = "x86",
	# バージョン (省略時は CLCL.rc の FILEVERSION を使用)
	[string]$Version = "",
	# インストールするファイルの収集元 (省略時は <リポジトリ>\<ビルド構成>)
	[string]$SourceDir = "",
	# インストーラの出力先 (省略時は Installer\out)
	[string]$OutDir = "",
	# CLCL 本体のビルドを行わない (インストーラのみビルドする)
	[switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step([string]$message) {
	Write-Host "==> $message" -ForegroundColor Cyan
}

$InstallerDir = $PSScriptRoot
$RootDir = Split-Path -Parent $InstallerDir

# ------------------------------------------------------------------
# msbuild の検索
# ------------------------------------------------------------------
$vsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vsWhere)) {
	throw "vswhere.exe が見つかりません。Visual Studio 2017 以降が必要です。"
}
$msBuild = & $vsWhere -latest -products * `
	-requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
	-find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
if ([string]::IsNullOrEmpty($msBuild)) {
	throw "C++ のビルドツールを含む Visual Studio が見つかりません。"
}

# ------------------------------------------------------------------
# prepare.ps1 に渡す設定
# msbuild のコマンドラインの引用符を壊さないように末尾の \ を取り除く
# ------------------------------------------------------------------
$props = @()
if (-not [string]::IsNullOrEmpty($Version)) {
	$props += "/p:ClclVersion=$Version"
}
if (-not [string]::IsNullOrEmpty($SourceDir)) {
	$props += "/p:ClclSourceDir=$($SourceDir.TrimEnd('\'))"
}
if (-not [string]::IsNullOrEmpty($OutDir)) {
	$props += "/p:ClclOutDir=$($OutDir.TrimEnd('\'))"
}

# ------------------------------------------------------------------
# ビルド
# ------------------------------------------------------------------
if ($SkipBuild) {
	Write-Step "インストーラをビルドします ($Configuration|Win32)"
	& $msBuild (Join-Path $InstallerDir "Installer.vcxproj") /nologo /v:minimal `
		"/p:Configuration=$Configuration" "/p:Platform=Win32" @props
} else {
	Write-Step "CLCL 本体とインストーラをビルドします ($Configuration|$Platform)"
	& $msBuild (Join-Path $RootDir "CLCL.sln") /nologo /v:minimal /m `
		"/p:Configuration=$Configuration" "/p:Platform=$Platform" @props
}
if ($LASTEXITCODE -ne 0) {
	throw "ビルドに失敗しました。"
}

$targetPath = Join-Path $InstallerDir (Join-Path $Configuration "CLCLInst.exe")
if (-not (Test-Path $targetPath)) {
	throw "インストーラがビルドされていません: $targetPath"
}
if ($Configuration -ne "Release") {
	Write-Step "インストーラをビルドしました ($Configuration)"
	Write-Host "    $targetPath"
	Write-Host "    out フォルダへの出力は Release 構成のみです。" -ForegroundColor Yellow
	return
}

# msbuild が再ビルドを省略するとビルド後のイベントも実行されないため、
# out フォルダへの出力はここで必ず行う
$publishArgs = @{ Mode = "Publish"; TargetPath = $targetPath }
if (-not [string]::IsNullOrEmpty($OutDir)) {
	$publishArgs.OutDir = $OutDir.TrimEnd('\')
}
if (-not [string]::IsNullOrEmpty($Version)) {
	$publishArgs.Version = $Version
}
& (Join-Path $InstallerDir "prepare.ps1") @publishArgs

$outRoot = if ([string]::IsNullOrEmpty($OutDir)) { Join-Path $InstallerDir "out" } else { $OutDir }
$exe = Get-ChildItem -Path (Join-Path $outRoot "clcl*.exe") -ErrorAction SilentlyContinue |
	Where-Object { $_.LastWriteTimeUtc -ge (Get-Item $targetPath).LastWriteTimeUtc } |
	Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($null -eq $exe) {
	throw "インストーラが出力されていません: $outRoot"
}
Write-Step "インストーラを作成しました"
Write-Host ("    {0} ({1:N0} bytes)" -f $exe.FullName, $exe.Length) -ForegroundColor Green
