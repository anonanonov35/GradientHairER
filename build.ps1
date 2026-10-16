param(
    [Parameter(Mandatory=$true)][string]$ZigExe,
    [string]$MinHookDir = (Join-Path $PSScriptRoot 'third_party/minhook'),
    [switch]$Test
)
$ErrorActionPreference = 'Stop'
$out = Join-Path $PSScriptRoot 'out'
$src = Join-Path $PSScriptRoot 'src'
New-Item -ItemType Directory -Path $out -Force | Out-Null
$env:ZIG_LOCAL_CACHE_DIR = Join-Path $out 'zig-local'
$env:ZIG_GLOBAL_CACHE_DIR = Join-Path $out 'zig-global'
$objects = @()
foreach ($file in @('src/buffer.c','src/hook.c','src/trampoline.c','src/hde/hde64.c')) {
    $object = Join-Path $out ((Split-Path $file -Leaf) + '.o')
    & $ZigExe cc -c -O2 -target x86_64-windows-gnu (Join-Path $MinHookDir $file) -o $object
    if ($LASTEXITCODE -ne 0) { throw ('Compilation failed: ' + $file) }
    $objects += $object
}
& $ZigExe c++ -shared -O2 -std=c++17 -Wall -Wextra -target x86_64-windows-gnu '-DHAIR_GRADIENT_COMPACT=1' '-DHAIR_GRADIENT_RUNTIME_MENU=1' '-DHAIR_GRADIENT_UMA_COMPAT=1' (Join-Path $src 'runtime.cpp') @objects -I (Join-Path $MinHookDir 'include') -I $src -o (Join-Path $out 'hair_gradient.dll')
if ($LASTEXITCODE -ne 0) { throw 'Runtime compilation failed' }
Write-Output 'Built out/hair_gradient.dll. Shader payloads are built separately.'
if ($Test) {
    foreach ($name in @('settings_test','menu_patch_test','runtime_menu_test','config_detection_test','appearance_selection_test','hook_chain_test')) {
        $exe = Join-Path $out ($name + '.exe')
        & $ZigExe c++ -O2 -std=c++17 -Wall -Wextra -target x86_64-windows-gnu (Join-Path $PSScriptRoot ('tests/' + $name + '.cpp')) @objects -I (Join-Path $MinHookDir 'include') -I $src -o $exe
        if ($LASTEXITCODE -ne 0) { throw ('Test compilation failed: ' + $name) }
        & $exe
        if ($LASTEXITCODE -ne 0) { throw ('Test failed: ' + $name) }
    }
}
