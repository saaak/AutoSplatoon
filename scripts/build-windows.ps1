param(
  [ValidateSet('release','debug')]
  [string]$Configuration = 'release'
)

$ErrorActionPreference = 'Stop'

$projectDir = Join-Path $PSScriptRoot '..' | Join-Path -ChildPath 'AutoSplatoon'

function Resolve-QT {
  if ($env:Qt5_DIR) {
    $qmake = Join-Path $env:Qt5_DIR 'bin/qmake.exe'
    $windeploy = Join-Path $env:Qt5_DIR 'bin/windeployqt.exe'
    return @{ qmake = $qmake; windeploy = $windeploy }
  }
  $cmd = Get-Command qmake.exe -ErrorAction SilentlyContinue
  if ($cmd) {
    $qmake = $cmd.Path
    $windeploy = Join-Path (Split-Path $qmake -Parent) 'windeployqt.exe'
    return @{ qmake = $qmake; windeploy = $windeploy }
  }
  $candidates = Get-ChildItem -Path 'C:\Qt' -Recurse -Filter qmake.exe -ErrorAction SilentlyContinue | Where-Object { $_.FullName -match 'msvc.*\\bin\\qmake\.exe' }
  if ($candidates -and $candidates.Count -gt 0) {
    $qmake = $candidates[0].FullName
    $windeploy = Join-Path (Split-Path $qmake -Parent) 'windeployqt.exe'
    return @{ qmake = $qmake; windeploy = $windeploy }
  }
  throw '未找到 Qt qmake，请安装 Qt 5.15 (win64_msvc2019_64) 或设置环境变量 Qt5_DIR'
}

function Resolve-VCVars64 {
  $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
  if (Test-Path $vswhere) {
    $vsPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
    if ($LASTEXITCODE -eq 0 -and $vsPath) {
      $bat = Join-Path $vsPath 'VC/Auxiliary/Build/vcvars64.bat'
      if (Test-Path $bat) { return $bat }
    }
  }
  return $null
}

$qt = Resolve-QT
if (!(Test-Path $qt.qmake)) { throw "未找到 qmake: $($qt.qmake)" }
if (!(Test-Path $qt.windeploy)) { throw "未找到 windeployqt: $($qt.windeploy)" }

Push-Location $projectDir
try {
  & $qt.qmake 'AutoSplatoon.pro' "CONFIG+=$Configuration"
  if ($LASTEXITCODE -ne 0) { throw 'qmake 执行失败' }

  $nmakeCmd = Get-Command nmake.exe -ErrorAction SilentlyContinue
  if ($nmakeCmd) {
    & $nmakeCmd.Source
  } else {
    $vcvars64 = Resolve-VCVars64
    if ($vcvars64) {
      cmd /c "call \"$vcvars64\" && nmake"
    } else {
      throw '未检测到 nmake 或 MSVC 环境，请安装 Microsoft C++ Build Tools 或 Visual Studio 并使用 x64 Native Tools 命令提示符'
    }
  }
  if ($LASTEXITCODE -ne 0) { throw 'nmake 构建失败' }

  $exe = $null
  $releaseExe = Join-Path $projectDir 'release/AutoSplatoon.exe'
  $debugExe = Join-Path $projectDir 'debug/AutoSplatoon.exe'
  if (Test-Path $releaseExe) { $exe = $releaseExe }
  elseif (Test-Path $debugExe) { $exe = $debugExe }
  else {
    $found = Get-ChildItem -Path $projectDir -Recurse -Filter AutoSplatoon.exe -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($found) { $exe = $found.FullName }
  }
  if (-not $exe) { throw '未找到生成的 AutoSplatoon.exe' }

  $dist = Join-Path $projectDir 'dist'
  if (!(Test-Path $dist)) { New-Item -ItemType Directory -Path $dist | Out-Null }
  Copy-Item -Path $exe -Destination (Join-Path $dist 'AutoSplatoon.exe') -Force
  & $qt.windeploy --$Configuration --compiler-runtime (Join-Path $dist 'AutoSplatoon.exe')
  if ($LASTEXITCODE -ne 0) { throw 'windeployqt 依赖打包失败' }
}
finally {
  Pop-Location
}

Write-Host "构建完成，输出目录: $projectDir\dist"
