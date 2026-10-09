param(
    [Parameter(Mandatory = $false)][string]$Executable,
    [Parameter(Mandatory = $false)][int]$Port,
    [Parameter(Mandatory = $false)][string]$ReadyFile,
    [switch]$Server
)

$ErrorActionPreference = 'Stop'

function Write-HttpResponse {
    param(
        [System.Net.Sockets.NetworkStream]$Stream,
        [int]$Status,
        [byte[]]$Body,
        [hashtable]$Headers = @{},
        [int]$DeclaredLength = -1
    )
    $reason = if ($Status -eq 200) { 'OK' } elseif ($Status -eq 302) { 'Found' } else { 'Not Found' }
    $length = if ($DeclaredLength -ge 0) { $DeclaredLength } else { $Body.Length }
    $header = "HTTP/1.1 $Status $reason`r`nContent-Length: $length`r`nConnection: close`r`n"
    foreach ($name in $Headers.Keys) { $header += "$name`: $($Headers[$name])`r`n" }
    $header += "`r`n"
    $headerBytes = [Text.Encoding]::ASCII.GetBytes($header)
    $Stream.Write($headerBytes, 0, $headerBytes.Length)
    if ($Body.Length -gt 0) { $Stream.Write($Body, 0, $Body.Length) }
    $Stream.Flush()
}

if ($Server) {
    $listener = [Net.Sockets.TcpListener]::new([Net.IPAddress]::Loopback, $Port)
    $listener.Start()
    [IO.File]::WriteAllText($ReadyFile, 'ready')
    try {
        for ($requestIndex = 0; $requestIndex -lt 7; $requestIndex++) {
            $client = $listener.AcceptTcpClient()
            try {
                $stream = $client.GetStream()
                $reader = [IO.StreamReader]::new($stream, [Text.Encoding]::ASCII, $false, 1024, $true)
                $requestLine = $reader.ReadLine()
                while ($reader.ReadLine()) { }
                $path = ($requestLine -split ' ')[1]
                switch ($path) {
                    '/json' {
                        Write-HttpResponse $stream 200 ([Text.Encoding]::UTF8.GetBytes('{"ok":true}')) @{ 'Content-Type' = 'application/json' }
                    }
                    '/redirect' {
                        Write-HttpResponse $stream 302 ([byte[]]::new(0)) @{ 'Location' = "/json" }
                    }
                    '/oversized' {
                        Write-HttpResponse $stream 200 ([Text.Encoding]::ASCII.GetBytes(('x' * 256)))
                    }
                    '/missing' {
                        Write-HttpResponse $stream 404 ([Text.Encoding]::UTF8.GetBytes('{"error":"missing"}')) @{ 'Content-Type' = 'application/json' }
                    }
                    '/binary' {
                        Write-HttpResponse $stream 200 ([byte[]](0, 1, 2, 3, 255)) @{ 'Content-Type' = 'application/octet-stream' }
                    }
                    '/partial' {
                        Write-HttpResponse $stream 200 ([byte[]](7, 8, 9)) @{} 10
                    }
                    default {
                        Write-HttpResponse $stream 404 ([byte[]]::new(0))
                    }
                }
            } finally {
                $client.Dispose()
            }
        }
    } finally {
        $listener.Stop()
    }
    exit 0
}

if (-not $Executable) { throw 'Executable is required' }
$probe = [Net.Sockets.TcpListener]::new([Net.IPAddress]::Loopback, 0)
$probe.Start()
$selectedPort = ([Net.IPEndPoint]$probe.LocalEndpoint).Port
$probe.Stop()
$ready = Join-Path ([IO.Path]::GetTempPath()) "pluto-http-fixture-$PID.ready"
$powershellPath = (Get-Process -Id $PID).Path
if (-not $powershellPath) { throw 'Unable to locate the active PowerShell executable' }
$serverProcess = Start-Process -FilePath $powershellPath -WindowStyle Hidden -PassThru -ArgumentList @(
    '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $PSCommandPath,
    '-Server', '-Port', $selectedPort, '-ReadyFile', $ready
)
try {
    $deadline = [DateTime]::UtcNow.AddSeconds(30)
    while (-not (Test-Path -LiteralPath $ready)) {
        if ($serverProcess.HasExited) { throw "HTTP fixture exited with $($serverProcess.ExitCode)" }
        if ([DateTime]::UtcNow -ge $deadline) { throw 'HTTP fixture did not become ready' }
        Start-Sleep -Milliseconds 25
    }
    & $Executable "http://127.0.0.1:$selectedPort"
    if ($LASTEXITCODE -ne 0) { throw "Updater I/O tests failed with exit code $LASTEXITCODE" }
    $serverProcess.WaitForExit(10000) | Out-Null
    if (-not $serverProcess.HasExited) { throw 'HTTP fixture did not exit after serving all requests' }
    if ($serverProcess.ExitCode -ne 0) { throw "HTTP fixture failed with exit code $($serverProcess.ExitCode)" }
} finally {
    if (-not $serverProcess.HasExited) { Stop-Process -Id $serverProcess.Id -Force }
    Remove-Item -LiteralPath $ready -Force -ErrorAction SilentlyContinue
}

Write-Output 'updater HTTP fixture tests passed'
