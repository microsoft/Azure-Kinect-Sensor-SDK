param(
    $SourceBranch = ${env:Build.SourceBranch},
    $SourcesDirectory = ${env:Build.SourcesDirectory},
    $DocumentationPath = "d:\git\Azure-Kinect-Sensor-SDK\build\docs\html"
)

$ErrorActionPreference = [System.Management.Automation.ActionPreference]::Stop;

cd $SourcesDirectory;

if ($SourceBranch.StartsWith('refs/'))
{
    $SourceBranch = $SourceBranch.SubString(5);
}

# Check out the documentation branch
git checkout gh-pages -q -c http.https://github.com.extraheader="AUTHORIZATION: Bearer $env:SYSTEM_ACCESSTOKEN"
git pull -c http.https://github.com.extraheader="AUTHORIZATION: Bearer $env:SYSTEM_ACCESSTOKEN"



# Create the directory if it doesn't exist
$DestinationDirectory = mkdir -force ([System.IO.Path]::Combine($SourcesDirectory, $SourceBranch))

# Delete the directory to ensure it is empty
rmdir -Force $DestinationDirectory -Recurse

# Copy the documentation HTML to that directory
Copy-Item -Recurse $DocumentationPath -Destination ($DestinationDirectory.FullName)


# Find all documentation branches (any subdirectory with index.html)
$Branches = dir -r -fi index.html |? { $_.Directory.FullName.Length -gt $SourcesDirectory.Length } |% { $_.Directory.FullName.Substring($SourcesDirectory.Length + 1).Replace('\', '/') } |? { -not $_.StartsWith('build/') }

# Save the branch metadata in branches.js
$Javascript = "var BRANCHES = " + ($Branches | ConvertTo-Json);
$Javascript | Set-Content branches.js;

git add *
git commit -m "Updated documentation for $SourceBranch from commit $(${env:Build.SourceVersion})"
git push -c http.https://github.com.extraheader="AUTHORIZATION: Bearer $env:SYSTEM_ACCESSTOKEN"