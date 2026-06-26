#!/usr/bin/env bash
# -e: Sai se algum comando falhar | -u: Sai se uma variável não definida for usada
set -euo pipefail

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

# ─────────────────────────────────────────────
#  Funções Auxiliares de Log
# ─────────────────────────────────────────────
info()    { echo -e "${CYAN}${BOLD}[•]${RESET} $*"; }
success() { echo -e "${GREEN}${BOLD}[✓]${RESET} $*"; }
warn()    { echo -e "${YELLOW}${BOLD}[!]${RESET} $*"; }
error()   { echo -e "${RED}${BOLD}[✗]${RESET} $*" >&2; }
step()    { echo -e "\n${BOLD}${CYAN}══ $* ${RESET}"; }
divider() { echo -e "${DIM}────────────────────────────────────────${RESET}"; }

# ─────────────────────────────────────────────
#  Apagar Build Existente
# ─────────────────────────────────────────────
_clean_build() {
    echo ""
    warn "Limpando arquivos de todos os builds existentes..."
    divider
    if [ -d "${BUILD_DIR}" ]; then
        rm -rf "${BUILD_DIR}"
        success "Pasta '${BUILD_DIR}' de executáveis removida."
    fi
    if [ -d "obj" ]; then
        rm -rf "obj"
        success "Pasta 'obj' de arquivos temporários removida."
    fi
    if [ -f "Makefile" ]; then
        rm -f Makefile
        success "Makefile principal gerado removido."
    fi

    # Remove arquivos de projeto .make gerados pelo Premake
    find . -maxdepth 2 -name "*.make" -delete 2>/dev/null
    success "Arquivos de projeto (.make) removidos."

    echo ""
    success "Todos os artefatos de compilação foram limpos!"
    exit 0
}

# Instala o premake dinamicamente caso necessário
_install_premake_manual() {
    warn "premake não encontrado. Iniciando instalação..."
    divider

    if ! command -v curl &>/dev/null && ! command -v wget &>/dev/null; then
        error "curl ou wget são necessários para o download."
        exit 1
    fi

    TMP_DIR=$(mktemp -d)
    TMP_TAR="${TMP_DIR}/premake.tar.gz"

    info "Baixando premake ${PREMAKE_VERSION}..."
    if command -v curl &>/dev/null; then
        curl -fsSL --progress-bar "${PREMAKE_URL}" -o "${TMP_TAR}"
    else
        wget -q --show-progress "${PREMAKE_URL}" -O "${TMP_TAR}"
    fi

    info "Extraindo..."
    tar -xzf "${TMP_TAR}" -C "${TMP_DIR}"

    info "Instalando em ${PREMAKE_INSTALL_DIR}/premake..."
    if [ -w "${PREMAKE_INSTALL_DIR}" ]; then
        cp "${TMP_DIR}/premake5" "${PREMAKE_BIN}"
        chmod +x "${PREMAKE_BIN}"
    else
        warn "Necessário permissão de Administrador (sudo) para instalar em ${PREMAKE_INSTALL_DIR}"

        if ! sudo cp "${TMP_DIR}/premake5" "${PREMAKE_BIN}"; then
            echo ""
            error "Permissão de Administrador recusada pelo utilizador. Encerrando..."
            rm -rf "${TMP_DIR}"
            exit 1
        fi
        sudo chmod +x "${PREMAKE_BIN}"
    fi

    rm -rf "${TMP_DIR}"

    if command -v premake &>/dev/null || command -v premake5 &>/dev/null; then
        success "premake instalado com sucesso em ${PREMAKE_BIN}"
    else
        error "Falha na instalação do premake."
        exit 1
    fi
}

# ─────────────────────────────────────────────
#  Configurações Globais
# ─────────────────────────────────────────────
PREMAKE_VERSION="5.0.0-beta8"
PREMAKE_URL="https://github.com/premake/premake-core/releases/download/v${PREMAKE_VERSION}/premake-${PREMAKE_VERSION}-linux.tar.gz"
PREMAKE_INSTALL_DIR="/usr/local/bin"
PREMAKE_BIN="${PREMAKE_INSTALL_DIR}/premake"

CONFIGS=("debug" "optimized" "release")
DEFAULT_CONFIG="debug"
BUILD_DIR="bin"
CACHE_FILE="scripts/.build_cache"

# Garante que o script execute a partir da raiz do projeto
cd "$(dirname "$0")/.." 2>/dev/null || cd "$(dirname "${BASH_SOURCE[0]}")/.."

# ─────────────────────────────────────────────
#  Pular etapas via Cache
# ─────────────────────────────────────────────
SKIPPED_CHECKS=0
if [ -f "$CACHE_FILE" ]; then
    if command -v premake &>/dev/null || command -v premake5 &>/dev/null; then
        SKIPPED_CHECKS=1
    fi
fi

# ─────────────────────────────────────────────
#  Banner Principal
# ─────────────────────────────────────────────
echo ""
echo -e "${BOLD}${CYAN}"
echo "  ╔════════════════════════════════════╗"
echo "  ║         BUILD SYSTEM - LINUX       ║"
echo "  ╚════════════════════════════════════╝"
echo -e "${RESET}"

if [[ "$SKIPPED_CHECKS" == "1" ]]; then
    echo -e "${GREEN}⚡ Ambiente pronto! Verificações de instalação puladas de forma inteligente.${RESET}"
    echo ""
fi

# ─────────────────────────────────────────────
#  Verificar / Instalar Premake
# ─────────────────────────────────────────────
if [[ "$SKIPPED_CHECKS" == "1" ]]; then
    # Se apenas o premake5 existir globalmente, cria a função de fallback em cache
    if ! command -v premake &>/dev/null && command -v premake5 &>/dev/null; then
        premake() { premake5 "$@"; }
        export -f premake
    fi
else
    step "Verificando premake"

    if command -v premake &>/dev/null; then
        PREMAKE_FOUND=$(command -v premake)
        INSTALLED_VERSION=$(premake --version 2>&1 | grep -oP 'v?\K[0-9]+\.[0-9]+\.[0-9]+' | head -1 || echo "0.0.0")
        success "premake encontrado: ${DIM}${PREMAKE_FOUND}${RESET}"
        echo -e "  ${DIM}versão instalada: v${INSTALLED_VERSION}${RESET}"

        if [[ "$INSTALLED_VERSION" != "$PREMAKE_VERSION" ]]; then
            warn "Versão desejada: v${PREMAKE_VERSION} | Instalada: v${INSTALLED_VERSION}"
            echo -n "  Deseja atualizar? [Y/n]: "
            read -r UPDATE_ANSWER
            UPDATE_ANSWER=${UPDATE_ANSWER:-Y}

            if [[ "${UPDATE_ANSWER,,}" == "n" ]]; then
                info "Continuando com a versão instalada..."
            else
                step "Atualizando premake"
                if command -v update-premake &>/dev/null; then
                    info "Usando update-premake..."
                    if update-premake; then
                        success "premake atualizado com sucesso via update-premake"
                    else
                        warn "update-premake falhou. Tentando atualização manual..."
                        _install_premake_manual
                    fi
                else
                    info "update-premake não disponível. Fazendo atualização manual..."
                    _install_premake_manual
                fi
            fi
        else
            success "premake já está na versão mais recente (v${PREMAKE_VERSION})"
        fi
    elif command -v premake5 &>/dev/null; then
        PREMAKE_FOUND=$(command -v premake5)
        success "premake5 encontrado (usando como premake): ${DIM}${PREMAKE_FOUND}${RESET}"
        premake() { premake5 "$@"; }
        export -f premake
    else
        _install_premake_manual
    fi

    # Grava o cache para agilizar os próximos builds
    mkdir -p scripts
    touch "$CACHE_FILE"

    divider
    echo -e "  ${DIM}versão: $(premake --version 2>&1 | head -1)${RESET}"
fi

# ─────────────────────────────────────────────
#  Menu Interativo
# ─────────────────────────────────────────────
step "Configuração de build"

if [[ $# -gt 0 ]]; then
    ARG="${1,,}"
    case "$ARG" in
        clean)
            _clean_build
            ;;
        debug|optimized|release)
            CONFIG="$ARG"
            ;;
        *)
            error "Argumento inválido: '${1}'. Use: debug, optimized, release, clean"
            echo ""
            echo "Uso: $0 {debug|optimized|release|clean}"
            exit 1
            ;;
    esac
else
    echo ""
    echo -e "  Escolha uma opção:"
    echo -e "  ${BOLD}[1]${RESET} debug         ${DIM}(símbolos, sem otimização)${RESET}"
    echo -e "  ${BOLD}[2]${RESET} optimized     ${DIM}(otimizado, runtime Release)${RESET}"
    echo -e "  ${BOLD}[3]${RESET} release       ${DIM}(otimização completa)${RESET}"
    echo -e "  ${BOLD}[4]${RESET} Apagar build  ${DIM}(apagar arquivos de build)${RESET}"
    echo -e "  ${BOLD}[5]${RESET} Sair"
    echo ""
    read -rp "  Opção [1]: " CHOICE

    case "${CHOICE}" in
        1|debug)     CONFIG="debug" ;;
        2|optimized) CONFIG="optimized" ;;
        3|release)   CONFIG="release" ;;
        4|apagar)    _clean_build ;;
        5|sair)      info "Saindo do script de build. Até mais!"; exit 0 ;;
        *)           CONFIG="debug" ;;
    esac
fi

# Validação estrita da configuração escolhida
VALID=false
for c in "${CONFIGS[@]}"; do
    [[ "$c" == "$CONFIG" ]] && VALID=true && break
done

if ! $VALID; then
    error "Configuração inválida: '${CONFIG}'. Use: ${CONFIGS[*]}"
    exit 1
fi

success "Configuração: ${BOLD}${CONFIG}${RESET}"

# ─────────────────────────────────────────────
#  Gerar Makefiles com o Premake
# ─────────────────────────────────────────────
step "Gerando Makefiles"

if [ ! -f "premake5.lua" ]; then
    error "premake5.lua não encontrado. Execute o script a partir da raiz do projeto."
    exit 1
fi

premake gmake --os=linux --cc=gcc
success "Makefiles gerados com sucesso!"

# ─────────────────────────────────────────────
#  Compilação (Build)
# ─────────────────────────────────────────────
step "Compilando"

NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
info "Usando ${NPROC} thread(s) de processamento"
divider

# Executa o make
if make config="${CONFIG}" -j"${NPROC}"; then
    echo ""
    divider
    success "${BOLD}Build concluído com sucesso!${RESET}"
    echo -e "  ${DIM}config: ${CONFIG}${RESET}"
    echo -e "  ${DIM}output: ${BUILD_DIR}/${CONFIG}/${RESET}"
    divider
    echo ""
    exit 0
else
    echo ""
    divider
    error "Build falhou durante a compilação com o make."
    divider
    echo ""
    exit 1
fi
