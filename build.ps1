[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$ZigExe, [switch]$Test)
$ErrorActionPreference = 'Stop'
$repoRoot = $PSScriptRoot
$spec = Get-Content -LiteralPath (Join-Path $repoRoot 'project.json') -Raw | ConvertFrom-Json
$zig = (Get-Command $ZigExe -ErrorAction Stop).Source
if ((& $zig version).Trim() -ne '0.13.0') { throw 'Use Zig 0.13.0 for this source snapshot.' }
$out = Join-Path $repoRoot 'out'
$minHook = Join-Path $repoRoot 'third_party/minhook'
$previousLocalCache = $env:ZIG_LOCAL_CACHE_DIR
$previousGlobalCache = $env:ZIG_GLOBAL_CACHE_DIR
$env:ZIG_LOCAL_CACHE_DIR = Join-Path $out 'cache-local'
$env:ZIG_GLOBAL_CACHE_DIR = Join-Path $out 'cache-global'
New-Item -ItemType Directory -Path $out -Force | Out-Null
Push-Location -LiteralPath $repoRoot
try {
    $objects = @()
    foreach ($unit in @('src/buffer.c','src/hook.c','src/trampoline.c','src/hde/hde64.c')) {
        $object = Join-Path $out ((Split-Path $unit -Leaf) + '.o')
        & $zig cc -c -O2 -target x86_64-windows-gnu (Join-Path $minHook $unit) -o $object
        if ($LASTEXITCODE -ne 0) { throw "MinHook compile failed: $unit" }
        $objects += $object
    }
    $includes = @('-I', (Join-Path $repoRoot 'src'), '-I', (Join-Path $repoRoot 'compat/eldenring_1_17'), '-I', (Join-Path $minHook 'include'))
    $sources = @($spec.translation_units | ForEach-Object { Join-Path $repoRoot $_ })
    $defines = @($spec.defines | ForEach-Object { '-D' + $_ })
    & $zig c++ -shared -O2 -std=c++17 -target x86_64-windows-gnu @defines @sources @objects @includes -o (Join-Path $out $spec.dll)
    if ($LASTEXITCODE -ne 0) { throw 'Runtime compile failed' }
    Copy-Item -LiteralPath (Join-Path $repoRoot $spec.config) -Destination $out
    if ($Test) {
        New-Item -ItemType Directory -Path (Join-Path $out 'test-fixtures') -Force | Out-Null
        foreach ($name in $spec.tests) {
            $inputs = @((Join-Path $repoRoot "tests/$name.cpp"))
            if ($name -eq 'test_idle_stances') { $inputs += (Join-Path $repoRoot 'src/ear_motion.cpp') }
            $testExe = Join-Path $out "$name.exe"
            & $zig c++ -O2 -UNDEBUG -std=c++17 -target x86_64-windows-gnu @inputs @objects @includes -o $testExe
            if ($LASTEXITCODE -ne 0) { throw "Test compile failed: $name" }
            & $testExe
            if ($LASTEXITCODE -ne 0) { throw "Test failed: $name" }
        }
        if ($spec.tail_skin_harness) {
            $testExe = Join-Path $out 'test_tail_skin.exe'
            & $zig c++ -O2 -UNDEBUG -std=c++17 -target x86_64-windows-gnu (Join-Path $repoRoot 'tests/test_tail_skin.cpp') @includes -o $testExe
            if ($LASTEXITCODE -ne 0) { throw 'Tail skin harness compile failed' }
            foreach ($scenario in @('walk','riding','idle')) {
                $arguments = @()
                if ($scenario -ne 'walk') { $arguments = @('--' + $scenario) }
                & $testExe @arguments | Out-Null
                if ($LASTEXITCODE -ne 0) { throw "Tail skin harness failed: $scenario" }
            }
            Write-Output 'PASS: tail skin harness (walk, riding, idle)'
        }
    }
    Write-Output ("Built out/" + $spec.dll)
} finally {
    Pop-Location
    $env:ZIG_LOCAL_CACHE_DIR = $previousLocalCache
    $env:ZIG_GLOBAL_CACHE_DIR = $previousGlobalCache
}
