param([string]$Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$taskSource = Join-Path $PSScriptRoot 'twip_pair_integrate.c'
$taskLibrary = Join-Path $PSScriptRoot 'twip_pair_integrate.dll'
$taskCompiler = (Get-Command $Compiler -ErrorAction Stop).Source
# Close Scilab before rebuilding a library it has already loaded.
& $taskCompiler -O2 -Wall -Wextra -Werror -shared -static-libgcc -o $taskLibrary $taskSource
if ($LASTEXITCODE -ne 0) { throw 'Pair diagnostic kernel build failed.' }
@{
    source_sha256 = (Get-FileHash -LiteralPath $taskSource -Algorithm SHA256).Hash
    library_sha256 = (Get-FileHash -LiteralPath $taskLibrary -Algorithm SHA256).Hash
    compiler = $taskCompiler
    options = '-O2 -Wall -Wextra -Werror -shared -static-libgcc; no fast-math'
    physical_parameters_validated = $false
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'pair_native_build.json') -Encoding utf8
