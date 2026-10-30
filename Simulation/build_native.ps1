param([string]$Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$headers = Join-Path $root '../Xe_Can_Bang/Core/Inc'
$jobs = @(
    @('tools/twip_integrate.c', 'tools/twip_integrate.dll'),
    @('tools/twip_pair_integrate.c', 'tools/twip_pair_integrate.dll'),
    @('current/pair_catalog_v2.c', 'current/pair_catalog_v2.dll'),
    @('current/control_native.c', 'current/control_native.dll'),
    @('current/motion_native.c', 'current/motion_native.dll'),
    @('current/pair_contact.c', 'current/contact_native.dll'),
    @('current/recovery_native.c', 'current/recovery_native.dll')
)
foreach ($job in $jobs) {
    & $Compiler -O2 -shared -static-libgcc '-I' $headers (Join-Path $root $job[0]) -o (Join-Path $root $job[1]) -lm
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $($job[0])" }
}
Write-Host 'Simulation libraries built. Execute robot_can_bang_pid_1.sce in Scilab.'
Copy-Item -LiteralPath (Join-Path $root 'current/motion_native.dll') -Destination (Join-Path $root 'current/motion_rt5.dll') -Force
Copy-Item -LiteralPath (Join-Path $root 'current/recovery_native.dll') -Destination (Join-Path $root 'current/recovery_rt5.dll') -Force
