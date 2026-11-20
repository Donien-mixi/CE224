# Tao lai source snapshot cho RT29 / RT34 trong .build/ide_src
# de mo bang STM32CubeIDE / Arduino IDE va nap bang nut Build + Run / Upload.
# Khong dua source snapshot vao git (nam trong .build, da gitignore).
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$out  = Join-Path $root '.build/ide_src'
New-Item -ItemType Directory -Force $out | Out-Null

function Export-Version([string]$name, [string]$commit) {
  $dir = Join-Path $out $name
  $tar = Join-Path $out "$name.tar"
  if (Test-Path $dir) { Remove-Item -Recurse -Force $dir }
  Push-Location $root
  try {
    git archive -o $tar $commit Xe_Can_Bang ESP32_S3_Gateway
    if ($LASTEXITCODE) { throw "git archive failed for $commit" }
  } finally { Pop-Location }
  New-Item -ItemType Directory -Force $dir | Out-Null
  tar -xf $tar -C $dir
  if ($LASTEXITCODE) { throw "tar extract failed for $name" }
  Remove-Item $tar -Force
  $stm = Select-String -Path (Join-Path $dir 'Xe_Can_Bang/Core/Inc/balance_profile.h') -Pattern '#define BALANCE_BUILD'
  $esp = Select-String -Path (Join-Path $dir 'ESP32_S3_Gateway/firmware_build.h') -Pattern '#define TWIP_WEB_BUILD'
  Write-Output ("{0}  {1}" -f $name, $stm.Line.Trim())
  Write-Output ("{0}  {1}" -f $name, $esp.Line.Trim())
  Write-Output ("{0}  -> {1}" -f $name, $dir)
}

Export-Version 'RT29' 'ceb3fd6'
Export-Version 'RT34' 'dccb707'
Export-Version 'RT37' '7e03db3'
Export-Version 'RT38' 'ec2deca'
Export-Version 'RT39' 'ccac2d5'
Export-Version 'RT40' 'b7b0e52'
Export-Version 'RT41' '26d58ae'
Write-Output ''
Write-Output 'RT42 (ban hien hanh) dung truc tiep: D:\DA_HTN\Xe_Can_Bang va D:\DA_HTN\ESP32_S3_Gateway'
Write-Output 'Mo STM32CubeIDE / Arduino IDE theo Docs/Nap_Bang_IDE.md'
