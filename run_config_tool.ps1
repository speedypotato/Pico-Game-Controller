# Run the Pico IIDX config tool (tools/effect_selector.py) using a Python virtual environment
[CmdletBinding()]
param(
    [switch]$SkipInstall
)

$ErrorActionPreference = 'Stop'
$RepoRoot   = Split-Path -Parent $MyInvocation.MyCommand.Path
$VenvPath   = Join-Path $RepoRoot '.venv'
$ActivatePs = Join-Path $VenvPath 'Scripts/Activate.ps1'
$PyExe      = Join-Path $VenvPath 'Scripts/python.exe'
$ReqFile    = Join-Path $RepoRoot 'tools/requirements.txt'
$AppFile    = Join-Path $RepoRoot 'tools/effect_selector.py'
${ExeOneFile} = Join-Path $RepoRoot 'dist/PicoGameConfig.exe'
${ExeOneDir}  = Join-Path $RepoRoot 'dist/PicoGameConfig/PicoGameConfig.exe'

Write-Host "Repo: $RepoRoot"

# If a compilado EXE exists, run it directly
if (Test-Path ${ExeOneFile}) {
    Write-Host "Ejecutando binario empaquetado: ${ExeOneFile}"
    & ${ExeOneFile}
    return
}
elseif (Test-Path ${ExeOneDir}) {
    Write-Host "Ejecutando binario empaquetado: ${ExeOneDir}"
    & ${ExeOneDir}
    return
}

# Pick Python launcher (python or py -3)
$PythonLauncher = $null
if (Get-Command python -ErrorAction SilentlyContinue) {
    $PythonLauncher = 'python'
} elseif (Get-Command py -ErrorAction SilentlyContinue) {
    $PythonLauncher = 'py -3'
} else {
    Write-Error 'Python no está en PATH. Instálalo y vuelve a intentar.'
}

# Create venv if missing
if (-not (Test-Path $VenvPath)) {
    Write-Host "Creando entorno virtual en $VenvPath ..."
    & $PythonLauncher -m venv $VenvPath
}

# Try activating the venv; if blocked by policy, fall back to using the venv's python directly
$Activated = $false
try {
    Write-Host 'Activando entorno virtual...'
    . $ActivatePs
    $Activated = $true
}
catch {
    Write-Warning 'No se pudo activar el venv (posible ExecutionPolicy). Continuando con el ejecutable del venv directamente.'
}

# Install requirements unless skipped
if (-not $SkipInstall -and (Test-Path $ReqFile)) {
    Write-Host "Instalando dependencias desde $ReqFile ..."
    if ($Activated) {
        pip install -r $ReqFile
    }
    else {
        & $PyExe -m pip install -r $ReqFile
    }
}

# Launch the app
Write-Host 'Ejecutando herramienta de configuración...'
if ($Activated) {
    python $AppFile
}
else {
    & $PyExe $AppFile
}
