# Connect this laptop to the ESP32 Wi-Fi first. Does not send motor commands.
[CmdletBinding()]
param([string]$BaseUrl='http://192.168.4.1',[string]$OutputDirectory='')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path $PSScriptRoot -Parent
$taskFolder=if($OutputDirectory){$OutputDirectory}else{Join-Path $taskRoot 'Simulation/data/raw/balance_runs'}
New-Item -ItemType Directory -Force $taskFolder | Out-Null
$taskPath=Join-Path $taskFolder ('twip_balance_rescued_'+(Get-Date -Format 'yyyyMMdd_HHmmss_fff')+'.txt')
$taskRequest=[System.Net.HttpWebRequest]::Create($BaseUrl.TrimEnd('/')+'/balance-log')
# Old firmware only enables chunked framing for HTTP/1.1. HTTP/1.0 recovers its raw body.
$taskRequest.ProtocolVersion=[System.Net.HttpVersion]::Version10
$taskRequest.KeepAlive=$false
$taskRequest.Proxy=$null
$taskRequest.Timeout=15000
$taskRequest.ReadWriteTimeout=15000
$taskResponse=$null;$taskInput=$null;$taskOutput=$null
try {
  Write-Host 'Dang tai log bang HTTP/1.0. Khong tat nguon ESP32...'
  $taskResponse=$taskRequest.GetResponse()
  $taskInput=$taskResponse.GetResponseStream()
  $taskOutput=[System.IO.File]::Create($taskPath)
  $taskInput.CopyTo($taskOutput)
} catch {
  throw "Khong tai duoc log. Ket noi laptop vao Wi-Fi ESP32, dung motor va doi 2 giay. Neu da mat nguon ESP32 thi log cu khong con. Chi tiet: $($_.Exception.Message)"
} finally {
  if($taskOutput){$taskOutput.Dispose()}
  if($taskInput){$taskInput.Dispose()}
  if($taskResponse){$taskResponse.Dispose()}
}
$taskLines=Get-Content -LiteralPath $taskPath -Encoding UTF8
if(!$taskLines.Count -or $taskLines[0] -notmatch '^# TWIP_BALANCE_V1') {
  throw "Tep khong phai log can bang: $taskPath"
}
$taskActual=@($taskLines | Where-Object { $_ -match '^\d+,' }).Count
$taskExpectedLine=$taskLines | Where-Object { $_ -match '^# samples=(\d+)' } | Select-Object -First 1
if($taskExpectedLine -match '^# samples=(\d+)' -and $taskActual -ne [int]$Matches[1]) {
  Write-Warning "Log thieu dong: nhan $taskActual, du kien $($Matches[1]). Tep van duoc giu de phan tich."
}
Write-Host "Da luu $taskActual mau: $taskPath"
Write-Host 'Gui file TXT nay de phan tich ly do xe dung. Chua xoa log tren ESP32.'
