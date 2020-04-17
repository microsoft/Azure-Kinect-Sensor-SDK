$ErrorActionPreference = "Stop"

function Download-ToTemp
{
    param
    (
        [Parameter(Mandatory)]
        [string] $url,
        [string] $filename
    )

    if (-not ($filename))
    {
        $filename = Split-Path -Path $url -Leaf
    }

    if (-not ($filename))
    {
        Write-Error "Unable to parse filename from $url"
        return $null
    }

    $tempDir = [System.IO.Path]::GetTempPath()
    $path = Join-Path -Path $tempDir -ChildPath $filename

    Write-Host -NoNewline "Downloading $url to $path..."
    Invoke-WebRequest -Uri $url -OutFile $path -UserAgent "NativeClient" -MaximumRetryCount 5 -RetryIntervalSec 60
    Write-Host "Done"

    return $path
}

$retry = 1;
Do
{
    Write-Host "Attempting to download OpenCV, try #$retry"

    try
    {    
        # Download OpenCV
        $url = "https://sourceforge.net/projects/opencvlibrary/files/4.1.1/opencv-4.1.1-vc14_vc15.exe/download"
        $filename = "opencv-4.1.1-vc14_vc15.exe"
        $opencv_exe = Download-ToTemp -url $url -filename $filename
    }
    catch
    {
        Write-Host "An exception was thrown"
        Write-Host  "##vso[task.LogIssue type=error;]An exception was thrown"
    }

    if (-not(Test-Path $opencv_exe))
    {
        Start-Sleep -s 15
    }
    $retry+=1
}While ((-not (Test-Path $opencv_exe)) -and ($retry -lt 20))

Start-Process -Wait $opencv_exe -ArgumentList -o"C:\",-y
Write-Host "OpenCV installed."
