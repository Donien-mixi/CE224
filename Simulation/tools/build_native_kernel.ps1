param([string]$Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$taskSource = Join-Path $PSScriptRoot 'twip_integrate.c'
$taskLibrary = Join-Path $PSScriptRoot 'twip_integrate.dll'
$taskCompiler = (Get-Command $Compiler -ErrorAction Stop).Source
& $taskCompiler -O2 -shared -static-libgcc -o $taskLibrary $taskSource
if ($LASTEXITCODE -ne 0) { throw 'Native interval build failed.' }
@{
    source_sha256 = (Get-FileHash -LiteralPath $taskSource -Algorithm SHA256).Hash
    library_sha256 = (Get-FileHash -LiteralPath $taskLibrary -Algorithm SHA256).Hash
    compiler = $taskCompiler
    options = '-O2 -shared -static-libgcc; no fast-math'
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'native_build.json') -Encoding utf8
