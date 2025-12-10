# Mapeamento dos Requisitos do Trabalho M3

Este documento lista cada quesito solicitado pelo professor, o que faz e onde está implementado no código.

---

## 3.1 Estrutura de Diretórios em Árvore
- **Função/Serviço**: Representar diretórios como árvore N-ária, navegar com caminhos absolutos/relativos.
- **Onde está**:
  - Estrutura `FCB` com `filhos` e `pai`: `src/header/bloco_controle.h`
  - Criação de diretórios: `FileSystem::mkdir` — `src/impl/file_system.cpp`
  - Navegação: `FileSystem::cd`, montagem de caminho: `FileSystem::obterCaminho` — `src/impl/file_system.cpp`

## 3.2 Representação e Metadados (FCB)
- **Função/Serviço**: File Control Block simulando inode, com metadados completos.
- **Onde está**:
  - Estrutura `FCB` (inodeId, nome, tipo, tamanho, owner/group, permissões, timestamps, índices de blocos): `src/header/bloco_controle.h`
  - Construtor do FCB: inicializa inode e timestamps — `src/impl/fcb.cpp`
  - Criação/atualização de arquivos: `FileSystem::touch`, `FileSystem::echo` — `src/impl/file_system.cpp`
  - Leitura e acesso: `FileSystem::cat` (atualiza acesso), `FileSystem::stat` (exibe metadados) — `src/impl/file_system.cpp`

## 3.3 Controle de Acesso e Permissões (RWX)
- **Função/Serviço**: Permissões para owner/grupo/outros com bits RWX; checagem em todas as operações.
- **Onde está**:
  - Máscaras de permissão: `PERM_READ`, `PERM_WRITE`, `PERM_EXEC` — `src/header/constantes.h`
  - Resolução de permissão efetiva: `FileSystem::verificarPermissao` — `src/impl/file_system.cpp`
  - Aplicação por comando (`src/impl/file_system.cpp`):
    - `cd` exige `x` no diretório atravessado.
    - `ls` exige `r` no diretório.
    - `cat` exige `r` no arquivo.
    - `echo` exige `w` no arquivo.
    - `rm`/`mv` exigem `w` no diretório e, para arquivos, `w` no alvo.
    - `cp` exige `r` na origem e `w` no destino (inclui diretórios recursivos).
    - `exec` exige `x` no arquivo.
    - `chmod` permite mudar permissão (dono ou root).
    - `su`/`quemSou` mudam e exibem o usuário atual.
  - CLI com todos os comandos: `src/impl/fs_sim.cpp`

## 3.4 Simulação de Alocação de Blocos
- **Função/Serviço**: Disco virtual com alocação indexada, bitmap de blocos livres/ocupados.
- **Onde está**:
  - Classe `VirtualDisk` (dados + mapaBits): `src/header/disco_virtual.h`
  - Alocação/liberação: `alocarBlocos`, `liberarBlocos`
  - I/O de blocos: `escreverDados`, `lerDados`
  - Uso pelas operações (todas em `src/impl/file_system.cpp`):
    - `touch` aloca bloco inicial vazio.
    - `echo` realoca blocos conforme o tamanho e escreve os dados.
    - `cat` lê blocos conforme o tamanho.
    - `rm`/`cp` liberam e copiam blocos conforme necessário.

## Interface de Linha de Comando (CLI)
- **Função/Serviço**: Expor operações do simulador via terminal.
- **Onde está**: `src/impl/fs_sim.cpp`
  - Comandos: `mkdir`, `cd`, `ls`, `touch`, `echo`, `cat`, `cp`, `mv`, `rm`, `chmod`, `stat`, `exec`, `su`, `whoami`, `help`, `exit`.

---

### Observação
Comentários-guia foram inseridos diretamente no código para indicar os pontos acima, especialmente em `src/impl/file_system.cpp` (lógicas principais) e nos headers (`bloco_controle.h`, `constantes.h`, `disco_virtual.h`, `sistema_arquivos.h`).

