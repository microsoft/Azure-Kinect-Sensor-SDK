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

    Write-Host "Downloading $url to $path..."
    Invoke-WebRequest -Uri $url -OutFile $path -UserAgent "NativeClient" -MaximumRetryCount 5 -RetryIntervalSec 60
    Write-Host "Downloading Done"

    return $path
}

# Total timeout is 20 minutes
$delay_in_seconds = 15
$max_retry_attempts = 80
$url = "https://sourceforge.net/projects/opencvlibrary/files/4.1.1/opencv-4.1.1-vc14_vc15.exe/download"
$filename = "opencv-4.1.1-vc14_vc15.exe"

$retry = 1;
Do
{
    Write-Host
    Write-Host
    Write-Host "Attempting to download OpenCV, try #$retry"

    $opencv_exe = "error_Download-ToTemp_did_not_return_a_file" # default value incase of exception
    try
    {    
        # Download OpenCV
        $opencv_exe = Download-ToTemp -url $url -filename $filename
    }
    catch
    {
        Write-Host
        Write-Host "An exception was thrown: $($PSItem.ToString())"
    }

    Write-Host "Processing downloaded file: $opencv_exe"

    $retry+=1
    if (-not(Test-Path $opencv_exe))
    {
        if ($retry -gt $max_retry_attempts)
        {
            Write-Host "ERROR: Retries exhausted!"
            exit 1
        }
        Write-Host "Retry in $delay_in_seconds seconds ..."
        Start-Sleep -s $delay_in_seconds
    }
}While (-not (Test-Path $opencv_exe))

Start-Process -Wait $opencv_exe -ArgumentList -o"C:\",-y
Write-Host "OpenCV installed."
