@echo off
setlocal EnableDelayedExpansion

:: -----------------------------------------------------------------------------
:: Flags Internas de Elevação (Bypass para o prompt Administrador)
:: -----------------------------------------------------------------------------
if "%~1"=="_install_msvc_now" goto :elevated_msvc
if "%~1"=="_install_premake_now" goto :elevated_premake

:: -----------------------------------------------------------------------------
:: Configurações Iniciais (UTF-8 e Cores ANSI para o fluxo normal)
:: -----------------------------------------------------------------------------
chcp 65001 >nul

for /F %%a in ('echo prompt $E ^| cmd') do set "ESC=%%a"
set "C_RESET=%ESC%[0m"
set "C_CYAN=%ESC%[36m"
set "C_GREEN=%ESC%[92m"
set "C_YELLOW=%ESC%[93m"
set "C_RED=%ESC%[91m"
set "C_BLUE=%ESC%[94m"

:: Garante que o script execute a partir da raiz do projeto
cd /d "%~dp0.."

:: -----------------------------------------------------------------------------
:: Otimização: Pular etapas se já houver compilação anterior (via cache)
:: -----------------------------------------------------------------------------
set "SKIPPED_CHECKS=0"
if exist "scripts\.build_cache.bat" (
    call "scripts\.build_cache.bat"
    if exist "!CACHED_VCVARS!" (
        call "!CACHED_VCVARS!" x64 >nul
        if defined PREMAKE_DIR set "PATH=!PREMAKE_DIR!;!PATH!"

        set "ASAN_DLL_PATH="
        for /f "delims=" %%g in ('where clang_rt.asan_dynamic-x86_64.dll 2^>nul') do set "ASAN_DLL_PATH=%%g"

        set "SKIPPED_CHECKS=1"
    )
)

:: -----------------------------------------------------------------------------
:: Selecionar Configuração (Argumentos de Linha de Comando)
:: -----------------------------------------------------------------------------
set "ARG1=%~1"
if /i "%ARG1%"=="debug" (
    set "CONFIG_CHOICE=1"
    if "%SKIPPED_CHECKS%"=="1" (goto :run_premake) else (goto :check_msvc)
)
if /i "%ARG1%"=="optimized" (
    set "CONFIG_CHOICE=2"
    if "%SKIPPED_CHECKS%"=="1" (goto :run_premake) else (goto :check_msvc)
)
if /i "%ARG1%"=="release" (
    set "CONFIG_CHOICE=3"
    if "%SKIPPED_CHECKS%"=="1" (goto :run_premake) else (goto :check_msvc)
)

:: -----------------------------------------------------------------------------
:: Menu Interativo Principal
:: -----------------------------------------------------------------------------
:menu
cls
if "%SKIPPED_CHECKS%"=="1" (
    echo %C_GREEN%⚡ Ambiente pronto! Verificações de instalação puladas de forma inteligente.%C_RESET%
    echo.
)
echo %C_CYAN%╭──────────────────────────────────────────╮%C_RESET%
echo %C_CYAN%│       ⚙️ Configuração de Build ⚙️        │%C_RESET%
echo %C_CYAN%├──────────────────────────────────────────┤%C_RESET%
echo %C_CYAN%│%C_RESET% %C_GREEN%[1]%C_RESET% Debug (Padrão)                       %C_CYAN%│%C_RESET%
echo %C_CYAN%│%C_RESET% %C_GREEN%[2]%C_RESET% Optimized                            %C_CYAN%│%C_RESET%
echo %C_CYAN%│%C_RESET% %C_GREEN%[3]%C_RESET% Release                              %C_CYAN%│%C_RESET%
echo %C_CYAN%│%C_RESET% %C_YELLOW%[4]%C_RESET% Apagar build                         %C_CYAN%│%C_RESET%
echo %C_CYAN%│%C_RESET% %C_RED%[5]%C_RESET% Sair                                 %C_CYAN%│%C_RESET%
echo %C_CYAN%╰──────────────────────────────────────────╯%C_RESET%
set "CONFIG_CHOICE=1"
set /p "CONFIG_CHOICE= ❯ Selecione uma opção [1]: "

if "%CONFIG_CHOICE%"=="4" goto :clean_build
if "%CONFIG_CHOICE%"=="5" goto :exit_script

if "%SKIPPED_CHECKS%"=="1" goto :run_premake

:: -----------------------------------------------------------------------------
:: 1. Verificar MSVC Build Tools
:: -----------------------------------------------------------------------------
:check_msvc
echo.
echo %C_YELLOW%[1/5] 🔍 Verificando MSVC Build Tools...%C_RESET%

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VCVARS="

if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        if exist "%%i\VC\Auxiliary\Build\vcvarsall.bat" (
            set "VCVARS=%%i\VC\Auxiliary\Build\vcvarsall.bat"
        )
    )
)

if not exist "%VCVARS%" (
    echo %C_RED%✖ MSVC C++ Build Tools não encontrado.%C_RESET%
    echo %C_YELLOW%➔ Abrindo janela de Administrador para instalação automática...%C_RESET%

    :: [AJUSTE 2] try/catch para capturar rejeição do UAC
    powershell -Command "$ErrorActionPreference='Stop'; try { Start-Process cmd.exe -ArgumentList '/c', '\"%~f0\" _install_msvc_now' -Verb RunAs -Wait } catch { exit 1 }"

    if %errorlevel% neq 0 (
        echo.
        echo %C_RED%✖ Permissão de Administrador recusada pelo usuário. Encerrando...%C_RESET%
        pause
        exit /b 1
    )

    echo %C_BLUE%➔ Prompt elevado finalizado. Re-verificando ambiente...%C_RESET%
    goto :check_msvc
) else (
    echo %C_GREEN%✔ MSVC Encontrado: "%VCVARS%"%C_RESET%
    echo %C_BLUE%Carregando ambiente x64...%C_RESET%
    call "%VCVARS%" x64 >nul

    set "ASAN_DLL_PATH="
    for /f "delims=" %%g in ('where clang_rt.asan_dynamic-x86_64.dll 2^>nul') do set "ASAN_DLL_PATH=%%g"
)

:: -----------------------------------------------------------------------------
:: 2. Gerenciar Premake
:: -----------------------------------------------------------------------------
:check_premake
echo.
echo %C_YELLOW%[2/5] 🔍 Verificando Premake5...%C_RESET%

where premake5 >nul 2>&1
if %errorlevel% equ 0 (
    echo %C_GREEN%✔ Premake5 encontrado no PATH.%C_RESET%
    goto :run_premake
)

if exist "%ProgramFiles%\Premake\premake5.exe" (
    set "PATH=%ProgramFiles%\Premake;%PATH%"
    echo %C_GREEN%✔ Premake5 encontrado em '%ProgramFiles%\Premake'.%C_RESET%
    goto :run_premake
)

echo %C_RED%✖ Premake5 não encontrado.%C_RESET%
echo %C_YELLOW%➔ Abrindo janela de Administrador para instalar o Premake5...%C_RESET%

:: [AJUSTE 2] try/catch para capturar rejeição do UAC
powershell -Command "$ErrorActionPreference='Stop'; try { Start-Process cmd.exe -ArgumentList '/c', '\"%~f0\" _install_premake_now' -Verb RunAs -Wait } catch { exit 1 }"

if %errorlevel% neq 0 (
    echo.
    echo %C_RED%✖ Permissão de Administrador recusada pelo usuário. Encerrando...%C_RESET%
    pause
    exit /b 1
)

echo %C_BLUE%➔ Instalação do Premake concluída. Re-verificando...%C_RESET%
goto :check_premake

:: -----------------------------------------------------------------------------
:: Execução Principal (Geração e Compilação)
:: -----------------------------------------------------------------------------
:run_premake
if "%SKIPPED_CHECKS%"=="1" goto :skip_cache_save
set "PREMAKE_DIR="
if exist "%ProgramFiles%\Premake\premake5.exe" (
    set "PREMAKE_DIR=%ProgramFiles%\Premake"
) else (
    for /f "delims=" %%g in ('where premake5 2^>nul') do (
        for %%h in ("%%g") do set "PREMAKE_DIR=%%~dph"
    )
)
if not exist "scripts" mkdir "scripts"
echo set "CACHED_VCVARS=%VCVARS%"> "scripts\.build_cache.bat"
echo set "PREMAKE_DIR=%PREMAKE_DIR%">> "scripts\.build_cache.bat"
set "SKIPPED_CHECKS=1"
:skip_cache_save

if "%CONFIG_CHOICE%"=="1" set "CONFIG=Debug"
if "%CONFIG_CHOICE%"=="2" set "CONFIG=Optimized"
if "%CONFIG_CHOICE%"=="3" set "CONFIG=Release"
if not defined CONFIG set "CONFIG=Debug"

echo.
echo %C_YELLOW%[3/5] 🛠  Configuração Selecionada: %CONFIG%%C_RESET%

echo.
echo %C_YELLOW%[4/5] 📦 Gerando Projeto Visual Studio 2022 (premake5 vs2022)...%C_RESET%
premake5 vs2022
if %errorlevel% neq 0 (
    echo %C_RED%✖ Falha ao gerar o projeto com Premake.%C_RESET%
    pause
    exit /b %errorlevel%
)
echo %C_GREEN%✔ Projeto gerado com sucesso!%C_RESET%

echo.
echo %C_YELLOW%[5/5] 🚀 Compilando com msbuild...%C_RESET%

set "SLN_FILE="
for %%f in (*.sln) do set "SLN_FILE=%%f"

if not defined SLN_FILE (
    echo %C_RED%✖ Arquivo .sln não encontrado na raiz!%C_RESET%
    pause
    exit /b 1
)

echo %C_BLUE%Comando: msbuild "%SLN_FILE%" /p:Platform=x64 /p:Configuration=%CONFIG% /m%C_RESET%
msbuild "%SLN_FILE%" /p:Platform=x64 /p:Configuration=%CONFIG% /m

if %errorlevel% neq 0 (
    echo.
    echo %C_RED%✖ Erro durante a compilação.%C_RESET%
    pause
    exit /b %errorlevel%
)

echo.
echo %C_GREEN%✨ Build '%CONFIG%' concluído com sucesso!%C_RESET%

if defined ASAN_DLL_PATH (
    if exist "bin\%CONFIG%\" (
        echo %C_BLUE%📦 Copiando runtime do ASan para 'bin/%CONFIG%/'...%C_RESET%
        copy /Y "%ASAN_DLL_PATH%" "bin\%CONFIG%\" >nul
    )
)

echo.
:: [AJUSTE 3] Sai do script ao invés de voltar para o menu interativo
exit /b 0

:: -----------------------------------------------------------------------------
:: Seção: Apagar arquivos de build existentes
:: -----------------------------------------------------------------------------
:clean_build
echo.
echo %C_YELLOW%🧹 Apagando arquivos de todos os builds existentes...%C_RESET%

:: [AJUSTE 1] Parênteses removidos de dentro das strings do bloco IF para evitar erros de sintaxe
if exist "bin" (
    rmdir /s /q "bin"
    echo %C_GREEN%✔ Pasta bin de executaveis removida.%C_RESET%
)
if exist "obj" (
    rmdir /s /q "obj"
    echo %C_GREEN%✔ Pasta obj de arquivos temporarios removida.%C_RESET%
)

set "DELETED_PROJECTS=0"
for %%f in (*.sln) do (del /q "%%f" & set "DELETED_PROJECTS=1")
for /r %%f in (*.vcxproj) do (del /q "%%f" & set "DELETED_PROJECTS=1")
for /r %%f in (*.vcxproj.user) do (del /q "%%f")
for /r %%f in (*.vcxproj.filters) do (del /q "%%f")

if "%DELETED_PROJECTS%"=="1" (
    echo %C_GREEN%✔ Arquivos de solucao do Visual Studio ^(.sln, .vcxproj^) removidos.%C_RESET%
)

echo.
echo %C_GREEN%✨ Todos os artefatos de compilação foram limpos!%C_RESET%
pause
goto :menu

:: -----------------------------------------------------------------------------
:: Seção: Cancelar e Sair
:: -----------------------------------------------------------------------------
:exit_script
echo %C_BLUE%Saindo do script de build. Até mais!%C_RESET%
exit /b 0

:: -----------------------------------------------------------------------------
:: Seções Elevadas (Executadas apenas no prompt Admin secundário se necessário)
:: -----------------------------------------------------------------------------
:elevated_msvc
chcp 65001 >nul
echo ====================================================================
echo  Instalando Microsoft C++ Build Tools e ASan via Winget...
echo ====================================================================
winget install --id Microsoft.VisualStudio.2022.BuildTools --force --exact --silent --accept-package-agreements --accept-source-agreements --override "--passive --wait --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 --add Microsoft.VisualStudio.Component.Windows10SDK.19041 --add Microsoft.VisualStudio.Component.VC.ASAN"
exit /b 0

:elevated_premake
chcp 65001 >nul
echo ====================================================================
echo  Instalando Premake5 (v5.0.0-beta8) em Program Files...
echo ====================================================================
mkdir "%ProgramFiles%\Premake" >nul 2>&1
powershell -Command "Invoke-WebRequest -Uri 'https://github.com/premake/premake-core/releases/download/v5.0.0-beta8/premake-5.0.0-beta8-windows.zip' -OutFile '%TEMP%\premake.zip'"
powershell -Command "Expand-Archive -Path '%TEMP%\premake.zip' -DestinationPath '%ProgramFiles%\Premake' -Force"
del "%TEMP%\premake.zip"
exit /b 0
