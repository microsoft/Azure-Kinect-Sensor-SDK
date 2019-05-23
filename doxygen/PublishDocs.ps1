param(
    $SourceBranch = ${env:Build_SourceBranch},
    $SourcesDirectory = ${env:Build_SourcesDirectory},
    $DocumentationPath = "${env:Build_BinariesDirectory}\docs\html"
)

$ErrorActionPreference = [System.Management.Automation.ActionPreference]::Stop;

cd $SourcesDirectory;

if ($SourceBranch.StartsWith('refs/heads/'))
{
    $SourceBranch = $SourceBranch.SubString('refs/heads/'.Length);
}

# Identity of the automated commits
git config --local user.email "k4abot@microsoft.com"
git config --local user.name "Azure Kinect Bot"

$Authorization = "Basic " + [System.Convert]::ToBase64String([System.Text.Encoding]::UTF8.GetBytes( "${env:GITHUBBOT_USER}:${env:GITHUBBOT_PAT}"))

git config --local --add "http.https://github.com/.extraheader" "AUTHORIZATION: $Authorization"

# Check out the documentation branch
git checkout gh-pages -q
git pull



# Create the directory if it doesn't exist
$DestinationDirectory = mkdir -force ([System.IO.Path]::Combine($SourcesDirectory, $SourceBranch))

# Delete the directory to ensure it is empty
rmdir -Force $DestinationDirectory -Recurse

# Copy the documentation HTML to that directory
Copy-Item -Recurse $DocumentationPath -Destination ($DestinationDirectory.FullName)


# Find all documentation branches (any subdirectory with index.html)
$Branches = Get-ChildItem -Recurse -Filter index.html |? { $_.Directory.FullName.Length -gt $SourcesDirectory.Length } |% { $_.Directory.FullName.Substring($SourcesDirectory.Length + 1).Replace('\', '/') } |? { -not $_.StartsWith('build/') }

# Save the branch metadata in branches.js
$Javascript = "var BRANCHES = " + (ConvertTo-Json @($Branches));
$Javascript | Set-Content branches.js;

git add *
git commit -m "Updated documentation for $SourceBranch from commit $(${env:Build_SourceVersion})"
git push
