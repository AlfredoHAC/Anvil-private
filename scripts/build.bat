@echo off
setlocal EnableDelayedExpansion

:: Garante que o script execute a partir da raiz do Anvil
cd /d "%~dp0.."

:: -----------------------------------------------------------------------------
:: Selecionar Configuração
:: -----------------------------------------------------------------------------
set "ARG1=%~1"
if /i "%ARG1%"=="clean" goto :clean_build
if /i "%ARG1%"=="debug" (
    set "CMAKE_CFG=Debug"
) else if /i "%ARG1%"=="optimized" (
    set "CMAKE_CFG=RelWithDebInfo"
) else if /i "%ARG1%"=="release" (
    set "CMAKE_CFG=Release"
) else (
    set "CMAKE_CFG=Debug"
)

:: -----------------------------------------------------------------------------
:: Verificar Dependências
:: -----------------------------------------------------------------------------
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo Erro: cmake nao encontrado.
    echo Instale via: winget install OpenSourceFoundation.CMake
    exit /b 1
)

:: -----------------------------------------------------------------------------
:: Compilar
:: -----------------------------------------------------------------------------
if "%CMAKE_CFG%"=="RelWithDebInfo" (
    set "DISPLAY_CFG=Optimized"
) else (
    set "DISPLAY_CFG=%CMAKE_CFG%"
)
echo Compilando Anvil (%DISPLAY_CFG%)...
if not exist "build" mkdir "build"
cmake -S . -B "build" -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=%CMAKE_CFG%
if %errorlevel% neq 0 (
    echo Erro ao configurar.
    exit /b %errorlevel%
)
cmake --build "build" --config %CMAKE_CFG% --parallel
if %errorlevel% neq 0 (
    echo Erro ao compilar.
    exit /b %errorlevel%
)

:: -----------------------------------------------------------------------------
:: Copiar ASan runtime (se disponivel)
:: -----------------------------------------------------------------------------
set "ASAN_DLL_PATH="
for /f "delims=" %%g in ('where clang_rt.asan_dynamic-x86_64.dll 2^>nul') do set "ASAN_DLL_PATH=%%g"

if defined ASAN_DLL_PATH (
    if exist "build\%CMAKE_CFG%\" (
        copy /Y "%ASAN_DLL_PATH%" "build\%CMAKE_CFG%\" >nul
    )
)

echo Build '%DISPLAY_CFG%' concluido com sucesso!
exit /b 0

:: -----------------------------------------------------------------------------
:: Limpar Build
:: -----------------------------------------------------------------------------
:clean_build
echo Limpando arquivos de build...

if exist "build" rmdir /s /q "build"

echo Build limpo com sucesso!
exit /b 0
