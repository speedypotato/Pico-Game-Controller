<#!
.SYNOPSIS
  Compila la herramienta de configuración (tools/effect_selector.py) con PyInstaller.

.DESCRIPTION
  Crea/usa un entorno virtual local (.venv), instala PyInstaller y dependencias,
  y genera un ejecutable con icono. Por defecto produce un único archivo EXE.

.PARAMETER OneFile
  Genera un único ejecutable (--onefile). Activado por defecto.

.PARAMETER Clean
  Limpia artefactos de PyInstaller antes de compilar.

.PARAMETER DebugConsole
  Muestra consola (no usa --noconsole). Útil para depurar.

.PARAMETER IconPath
  Ruta al icono .ico para el ejecutable. Por defecto: <repo>/icon.ico

.EXAMPLE
  ./tools/build_config_tool.ps1

.EXAMPLE
  ./tools/build_config_tool.ps1 -DebugConsole -Clean
#>
[CmdletBinding()]
param(
  [switch]$Clean,
  [switch]$DebugConsole,
  [string]$IconPath
)

$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$VenvPath = Join-Path $RepoRoot '.venv'
$PyExe    = Join-Path $VenvPath 'Scripts/python.exe'
$PipExe   = Join-Path $VenvPath 'Scripts/pip.exe'
$AppFile  = Join-Path $RepoRoot 'tools/effect_selector.py'
$DistDir  = Join-Path $RepoRoot 'dist'
$BuildDir = Join-Path $RepoRoot 'build_pyinstaller'
$SpecDir  = Join-Path $RepoRoot 'tools'
$SpecFile = Join-Path $SpecDir 'PicoGameConfig.spec'
$ExeName  = 'PicoGameConfig'

if (-not (Test-Path $AppFile)) { throw "No se encontró $AppFile" }

if (-not $IconPath) { $IconPath = Join-Path $RepoRoot 'icon.ico' }
if (-not (Test-Path $IconPath)) { Write-Warning "No se encontró el icono: $IconPath. Continuando sin icono (se usará el del spec si existe)." }
if (-not (Test-Path $SpecFile)) { throw "No se encontró el spec de PyInstaller: $SpecFile" }

# Crear venv si falta
if (-not (Test-Path $VenvPath)) {
  Write-Host "Creando entorno virtual en $VenvPath ..."
  # Elegir lanzador
  $PythonLauncher = $null
  if (Get-Command python -ErrorAction SilentlyContinue) { $PythonLauncher = 'python' }
  elseif (Get-Command py -ErrorAction SilentlyContinue) { $PythonLauncher = 'py -3' }
  else { throw 'Python no está en PATH. Instálalo y vuelve a intentar.' }
  & $PythonLauncher -m venv $VenvPath
}

# Instalar/actualizar dependencias de construcción
& $PyExe -m pip install --upgrade pip | Out-Host
& $PipExe install --upgrade pyinstaller | Out-Host

# Asegurar dependencia runtime del app (hidapi)
$ReqFile = Join-Path $RepoRoot 'tools/requirements.txt'
if (Test-Path $ReqFile) { & $PipExe install -r $ReqFile | Out-Host }

# Opcionalmente limpiar artefactos anteriores
if ($Clean) {
  if (Test-Path $DistDir)  { Remove-Item -Recurse -Force $DistDir }
  if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
}

# Ensamblar argumentos e invocar PyInstaller utilizando el SPEC
Write-Host "Compilando con PyInstaller (spec): $SpecFile"
& $PyExe -m PyInstaller --noconfirm --clean --distpath $DistDir --workpath $BuildDir $SpecFile

if ($LASTEXITCODE -ne 0) { throw "PyInstaller falló con código $LASTEXITCODE" }

$OutOneFile = Join-Path $DistDir ("$ExeName.exe")
$OutOneDir  = Join-Path (Join-Path $DistDir $ExeName) ("$ExeName.exe")
if (Test-Path $OutOneFile) {
  Write-Host "Listo: $OutOneFile"
}
elseif (Test-Path $OutOneDir) {
  Write-Host "Listo: $OutOneDir"
}
else {
  Write-Warning "Compilación completada, pero no se encontró el EXE. Revisa el directorio 'dist/'."
}
