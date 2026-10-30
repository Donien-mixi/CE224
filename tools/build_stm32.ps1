$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$plugins = 'C:/ST/STM32CubeIDE_2.1.1/STM32CubeIDE/plugins'
$gccDir = (Get-ChildItem $plugins -Directory -Filter '*gnu-tools*').FullName + '/tools/bin'
$make = (Get-ChildItem $plugins -Directory -Filter 'com.st.stm32cube.ide.mcu.externaltools.make.win32*').FullName + '/tools/bin/make.exe'
$release = Join-Path $root '.build/balance_release_r12_rt8_20261024'
New-Item -ItemType Directory -Force $release | Out-Null
$oldPath = $env:PATH
try {
  $env:PATH = $gccDir + ';' + $env:PATH
  $objects = Get-ChildItem (Join-Path $root 'Xe_Can_Bang/Core/Src') -File -Filter '*.c' | ForEach-Object { "Core/Src/$($_.BaseName).o" }
  & $make -C (Join-Path $root 'Xe_Can_Bang/Debug') -B -j4 @objects
  if ($LASTEXITCODE) { throw 'STM32 compilation failed' }
  Push-Location (Join-Path $root 'Xe_Can_Bang/Debug')
  try {
    & "$gccDir/arm-none-eabi-gcc.exe" -o Xe_Can_Bang.elf '@objects.list' -mcpu=cortex-m4 "-T$root/Xe_Can_Bang/STM32F411CEUX_FLASH.ld" --specs=nosys.specs '-Wl,-Map=Xe_Can_Bang.map' '-Wl,--gc-sections' -static --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -u _printf_float '-Wl,--start-group' -lc -lm '-Wl,--end-group'
    if ($LASTEXITCODE) { throw 'STM32 link failed' }
    & "$gccDir/arm-none-eabi-objcopy.exe" -O ihex Xe_Can_Bang.elf Xe_Can_Bang.hex
    if ($LASTEXITCODE) { throw 'STM32 HEX failed' }
    & "$gccDir/arm-none-eabi-objcopy.exe" -O binary Xe_Can_Bang.elf Xe_Can_Bang.bin
    if ($LASTEXITCODE) { throw 'STM32 BIN failed' }
    & "$gccDir/arm-none-eabi-size.exe" Xe_Can_Bang.elf
    foreach ($ext in 'elf','hex','bin') { Copy-Item -LiteralPath "Xe_Can_Bang.$ext" -Destination "$release/STM32_BAL_R12_RT8.$ext" -Force }
  } finally { Pop-Location }
  Write-Output "Built only; no flashing. Artifacts: $release"
} finally { $env:PATH = $oldPath }
