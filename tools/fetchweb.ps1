param([string]$outDir = "V:\boomkr-re\out\web")
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$ua = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0 Safari/537.36"
$targets = @(
  @("bing_alchemist", "https://www.bing.com/search?q=%ED%81%AC%EB%A0%88%EC%9D%B4%EC%A7%80%20%EC%95%84%EC%BC%80%EC%9D%B4%EB%93%9C%20%EC%97%B0%EA%B8%88%EC%88%A0%EC%82%AC&setlang=ko"),
  @("bing_baseball",  "https://www.bing.com/search?q=%ED%81%AC%EB%A0%88%EC%9D%B4%EC%A7%80%20%EC%95%84%EC%BC%80%EC%9D%B4%EB%93%9C%20%EC%95%BC%EA%B5%AC&setlang=ko"),
  @("google_alch",    "https://www.google.com/search?q=%ED%81%AC%EB%A0%88%EC%9D%B4%EC%A7%80+%EC%95%84%EC%BC%80%EC%9D%B4%EB%93%9C+%EC%97%B0%EA%B8%88%EC%88%A0%EC%82%AC&hl=ko")
)
foreach ($t in $targets) {
  $name = $t[0]; $url = $t[1]
  try {
    $r = Invoke-WebRequest -Uri $url -UseBasicParsing -TimeoutSec 25 -Headers @{ "User-Agent" = $ua; "Accept-Language" = "ko-KR,ko;q=0.9" }
    [IO.File]::WriteAllText((Join-Path $outDir "$name.html"), $r.Content, [Text.Encoding]::UTF8)
    Write-Output "$name OK $($r.Content.Length)"
  } catch { Write-Output "$name ERR $($_.Exception.Message)" }
}
Write-Output "ALLDONE"
