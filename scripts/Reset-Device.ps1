# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# This uses the Microsoft HMD Exerciser
# (https://docs.microsoft.com/en-us/windows-hardware/test/hmd/hmd-exerciser-kit-hardware)
# to reset the connection to the K4A device.

$ErrorActionPreference = "Stop"

# This utilizes serial APIs which are not available in PowerShell Core
if ($PSVersionTable.PSVersion.Major -ne 5) {
    Write-Error -Message "Script will only work with PowerShell 5 due to usage of [System.IO.Ports.SerialPort]"
}

$hmdKitCsFile = Join-Path -Path "$PSScriptRoot" -ChildPath "HmdKit.cs"
if (-not (Test-Path -Path $hmdKitCsFile)) {
    Write-Error -Message "HmdKit.cs does not exist at $hmdKitCsFile"
}
$source = Get-Content $hmdKitCsFile | Out-String

# Pull in the CSharp to be used
Add-Type -TypeDefinition $source -Language CSharp

# Search for the exerciser
$hmdkit = [HmdAutomation.HmdKit]::new()

if (-not $hmdkit.FindHmdKitDevice()) {
    Write-Error -Message "Unable to find HMD Exerciser"
}

# Find the port with a device connected by checking which port is pulling
# current
$ports = @("J2", "J3", "J4", "J6")
$connectedPorts = @()
foreach ($port in $ports) {
    $hmdkit.SetUsbPort($port)
    Start-Sleep -Milliseconds 500

    $amps = [int]$hmdkit.Amps()
    if (($amps -ne 1000) -and ($amps -ne 000)) {
        $connectedPorts += $port
    }
}

if ($connectedPorts.Length -ne 1) {
    $hmdkit.Dispose()
    if ($connectedPorts.Length -eq 0) {
        Write-Error -Message "Unable to a device pulling current on HMD Exerciser. Failing assuming this means no device is connected"
    } else {
        Write-Error -Message "Found multiple devices pulling current on HMD Exerciser. Failing assuming this means multiple devices are connected"
    }
}

$port = $connectedPorts[0]

$hmdkit.SetUsbPort($port)

Write-Host "Successfully Reset Device"

# Wait a little bit so the device can enumerate, 1.4 seconds has seemed good.
$waitTimeInMilliseconds = 1400
Write-Host "Waiting $waitTimeInMilliseconds milliseconds for the device to enumerate..." -NoNewline
Start-Sleep -Milliseconds $waitTimeInMilliseconds 

Write-Host "Done"

$hmdkit.Dispose()