param(
    [Parameter(Mandatory=$true)]
    [ValidateSet('public','playtest')][string]$Channel,
    [ValidateSet('windows','linux','all')][string]$Platform = 'windows',
    [int]$Jobs = 2,
    [string]$WorkDir = 'build-package',
    [string]$OutDir = 'dist',
    [string]$Python = 'python'
)
$root = Split-Path -Parent $PSScriptRoot
& $Python (Join-Path $root 'psxrecomp/tools/package_title_release.py') package --root $root --channel $Channel --platform $Platform --jobs $Jobs --work-dir $WorkDir --out-dir $OutDir
exit $LASTEXITCODE
