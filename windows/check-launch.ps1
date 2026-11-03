param(
    [Parameter(Mandatory)][string]$AppDirectory,
    [Parameter(Mandatory)][string]$ReportDirectory
)

$ErrorActionPreference = 'Stop'
$appPath = (Resolve-Path $AppDirectory).Path
$reportPath = (Resolve-Path $ReportDirectory).Path
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
$failed = $false

foreach ($mode in @('title', 'demo')) {
    $process = $null
    try {
        $options = @{ FilePath = "$appPath/make10sion.exe"; WorkingDirectory = $appPath; PassThru = $true }
        if ($mode -eq 'demo') { $options.ArgumentList = '--demo' }
        $process = Start-Process @options
        Start-Sleep -Seconds 20
        $process.Refresh()
        if ($process.HasExited) { throw "Exited during startup with code $($process.ExitCode)" }
        if ($process.MainWindowHandle -eq 0) { throw 'No application window was detected.' }
        "$mode`: process alive, window=$($process.MainWindowTitle), pid=$($process.Id)" |
            Set-Content "$reportPath/$mode-startup.txt"
        $screen = [System.Windows.Forms.SystemInformation]::VirtualScreen
        $bitmap = [System.Drawing.Bitmap]::new($screen.Width, $screen.Height)
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        try {
            $graphics.CopyFromScreen($screen.Left, $screen.Top, 0, 0, $bitmap.Size)
            $bitmap.Save("$reportPath/$mode-screen.png")
        } finally {
            $graphics.Dispose()
            $bitmap.Dispose()
        }
    } catch {
        $failed = $true
        "${mode}: $($_.Exception.Message)" | Set-Content "$reportPath/$mode-startup.txt"
        Write-Warning "${mode}: $($_.Exception.Message)"
    } finally {
        if ($process -and -not $process.HasExited) { Stop-Process -Id $process.Id -Force }
        Get-ChildItem $appPath -File | Where-Object { $_.Extension -in '.html', '.log' } |
            ForEach-Object { Copy-Item $_.FullName "$reportPath/$mode-$($_.Name)" }
    }
}

if ($failed) { throw 'A Windows startup check failed. Inspect the startup reports and engine logs.' }
