# Compatible with Windows PowerShell 5.1 and PowerShell 7 local CK wrappers.
function Assert-GameProcessesStopped {
    $blocked = @(
        'CreationKit', 'SkyrimSE', 'SkyrimTogether', 'SkyrimTogetherServer', 'Vortex',
        'xTESEdit', 'xTESEdit64', 'SSEEdit', 'SSEEdit64', 'xSSEEdit', 'xSSEEdit64'
    )
    # Enumerate successfully before deciding that no process is running.
    $running = @(Get-Process -ErrorAction Stop | Where-Object { $_.ProcessName -in $blocked })
    if ($running.Count) {
        throw "Close game/CK/Vortex/xEdit before snapshot, import or deployment: $($running.ProcessName -join ', ')"
    }
}
