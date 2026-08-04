#!/usr/bin/env bash
set -euo pipefail

# Garante que o script execute a partir da raiz do Anvil
cd "$(dirname "$0")/.." 2>/dev/null || cd "$(dirname "${BASH_SOURCE[0]}")/.."

# ─────────────────────────────────────────────
#  Cores e Estilos (ANSI)
# ─────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
DIM='\033[2m'
RESET='\033[0m'

success() { echo -e "${GREEN}${BOLD}[✓]${RESET} $*"; }
warn()    { echo -e "${YELLOW}${BOLD}[!]${RESET} $*"; }
error()   { echo -e "${RED}${BOLD}[✗]${RESET} $*" >&2; }
step()    { echo -e "\n${BOLD}${CYAN}══ $* ${RESET}"; }
divider() { echo -e "${DIM}────────────────────────────────────────${RESET}"; }

# ─────────────────────────────────────────────
#  Instalar CMake manualmente
# ─────────────────────────────────────────────
_install_cmake() {
    warn "cmake não encontrado. Iniciando instalação..."
    divider

    if command -v apt-get &>/dev/null; then
        sudo apt-get update -qq
        sudo apt-get install -y cmake ninja-build
        success "CMake instalado via apt!"
    elif command -v brew &>/dev/null; then
        brew install cmake ninja
        success "CMake instalado via Homebrew!"
    else
        error "Gerenciador de pacotes não encontrado."
        echo "Instale manualmente: https://cmake.org/download/"
        exit 1
    fi
}

# ─────────────────────────────────────────────
#  Selecionar Configuração
# ─────────────────────────────────────────────
CONFIG="debug"
if [[ $# -gt 0 ]]; then
    case "${1,,}" in
        clean)
            echo ""
            echo -e "${YELLOW}🧹 Limpando build...${RESET}"
            rm -rf build
            echo ""
            success "Build limpo com sucesso!"
            exit 0
            ;;
        debug)     CONFIG="debug" ;;
        optimized) CONFIG="relwithdebinfo" ;;
        release)   CONFIG="release" ;;
        *)
            echo "Uso: $0 {debug|optimized|release|clean}"
            exit 1
            ;;
    esac
fi

# ─────────────────────────────────────────────
#  Verificar Dependências
# ─────────────────────────────────────────────
step "Verificando dependências"

if command -v cmake &>/dev/null; then
    success "cmake encontrado: ${DIM}$(command -v cmake)${RESET}"
else
    _install_cmake
fi

if command -v ninja &>/dev/null; then
    success "ninja encontrado: ${DIM}$(command -v ninja)${RESET}"
elif command -v make &>/dev/null; then
    success "make encontrado: ${DIM}$(command -v make)${RESET}"
else
    warn "ninja ou make não encontrado. Instalando..."
    if command -v apt-get &>/dev/null; then
        sudo apt-get install -y ninja-build
    elif command -v brew &>/dev/null; then
        brew install ninja
    fi
fi

divider

# ─────────────────────────────────────────────
#  Compilar
# ─────────────────────────────────────────────
echo ""
echo -e "${YELLOW}[1/2] 🛠  Configuração Selecionada: ${BOLD}${CONFIG}${RESET}"
echo -e "${YELLOW}[2/2] 📦 Compilando Anvil...${RESET}"

mkdir -p build
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE="${CONFIG}"
cmake --build build --config "${CONFIG}" --parallel

echo ""
divider
success "${BOLD}Build '${CONFIG}' concluído com sucesso!${RESET}"
divider
echo ""
