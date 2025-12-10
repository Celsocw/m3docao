# Correções Implementadas - Sistema de Arquivos em Memória
## Trabalho M3 - Sistemas Operacionais - UNIVALI

Este documento detalha todas as correções implementadas para tornar o simulador totalmente compatível com os requisitos do enunciado.

---

## 📋 Requisitos do Enunciado vs. Implementação Original

### ❌ Problemas Identificados na Versão Original

1. **Permissões de diretório não validadas para navegação/listagem**
   - Comando `cd` não verificava permissão de execução (x) em diretórios
   - Comando `ls` não verificava permissão de leitura (r) em diretórios

2. **Bit de execução (X) nunca era usado**
   - Não havia comando para executar arquivos
   - Nenhuma verificação de PERM_EXEC em operações

3. **Verificações insuficientes em operações de arquivo**
   - `rm`/`mv` só checavam escrita no diretório, não no arquivo
   - Permissões específicas do arquivo eram ignoradas

4. **Cópia de diretórios não implementada**
   - Comando `cp` rejeitava cópia de diretórios
   - Não havia cópia recursiva

5. **Documentação incorreta**
   - Instruções de compilação não funcionavam
   - Faltavam detalhes sobre verificações de permissões

---

## ✅ Correções Implementadas por Requisito

### 3.1. Modelagem da Estrutura de Diretórios ✅
**Status:** Já implementado corretamente na versão original
- Estrutura em árvore N-ária com ponteiros inteligentes
- Map<string, shared_ptr<FCB>> para filhos
- weak_ptr<FCB> pai para evitar referências circulares
- Navegação com caminhos absolutos (/) e relativos (..)

### 3.2. Representação e Gerenciamento de Arquivos e Metadados ✅
**Status:** Já implementado corretamente na versão original
- FCB (File Control Block) com todos os atributos requeridos:
  - inodeId (simula inode)
  - nome, tipo, tamanho
  - idProprietario, idGrupo
  - Permissões owner/group/others
  - Timestamps: criadoEm, modificadoEm, acessadoEm
  - vector<int> indicesBlocos (alocação indexada)
- Operações básicas: touch, echo, cat, cp, mv, rm, stat

### 3.3. Controle de Acesso e Permissões 🔧 CORRIGIDO

#### **Problema Original:**
```cpp
// ANTES: cd não verificava permissões
void FileSystem::cd(string nome) {
    // ... navegação sem verificações de permissão
    dir = alvo;
}
```

#### **Correção Implementada:**
```cpp
// DEPOIS: cd verifica permissão de execução
void FileSystem::cd(string nome) {
    // ... para cada componente do caminho
    auto alvo = dir->filhos[comp];
    if (alvo->tipo == DIRECTORY) {
        // ✅ Verifica permissão de execução no diretório alvo
        if (usuarioAtual != 0 && !verificarPermissao(alvo, PERM_EXEC)) {
            cout << "Erro: Permissao negada (Execute).\n";
            return;
        }
        dir = alvo;
    }
}
```

#### **Arquivo:** `src/impl/file_system.cpp`, linhas 100-131
- **Comando `cd`**: Adicionada verificação de PERM_EXEC em diretórios
- **Comando `ls`**: Adicionada verificação de PERM_READ em diretórios

#### **Novo Comando `exec`:**
```cpp
// Novo método adicionado
void FileSystem::executar(string nome) {
    auto arquivo = diretorioAtual->filhos[nome];
    // ✅ Verifica permissão de execução
    if (!verificarPermissao(arquivo, PERM_EXEC)) {
        cout << "Erro: Permissao negada (Execute).\n";
        return;
    }
    // Simula execução baseada no tipo
    if (arquivo->tipo == TYPE_PROGRAM) {
        cout << "Executando programa: " << arquivo->nome << "\n";
    }
}
```

#### **Arquivo:** `src/impl/file_system.cpp`, linhas 410-444
- Adicionado método `executar()` na classe FileSystem
- Integrado ao CLI em `fs_sim.cpp`
- Atualizado help em `cliente.cpp`

#### **Correções em `rm` e `mv`:**
```cpp
// ANTES: só verificava diretório
if (usuarioAtual != 0 && !verificarPermissao(diretorioAtual, PERM_WRITE))

// DEPOIS: verifica arquivo E diretório
if (usuarioAtual != 0 && !verificarPermissao(diretorioAtual, PERM_WRITE))
if (alvo->tipo != DIRECTORY && usuarioAtual != 0 && !verificarPermissao(alvo, PERM_WRITE))
```

#### **Arquivo:** `src/impl/file_system.cpp`
- `rm()`: Linhas 288-317
- `mv()`: Linhas 320-345

### 3.4. Simulação de Alocação de Blocos ✅
**Status:** Já implementado corretamente na versão original
- Classe VirtualDisk com array linear de bytes
- Alocação indexada com bitmap
- vector<int> blockIndices no FCB
- Métodos: alocarBlocos(), liberarBlocos(), escreverDados(), lerDados()

---

## 📁 Arquivos Modificados

### `src/impl/file_system.cpp` (Principal)
- **Linhas 65-78:** `mkdir()` - Mantido (já correto)
- **Linhas 100-131:** `cd()` - ✅ **CORRIGIDO**: Adicionada verificação PERM_EXEC
- **Linhas 225-249:** `ls()` - ✅ **CORRIGIDO**: Adicionada verificação PERM_READ
- **Linhas 288-317:** `rm()` - ✅ **CORRIGIDO**: Verificação PERM_WRITE no arquivo
- **Linhas 320-345:** `mv()` - ✅ **CORRIGIDO**: Verificação PERM_WRITE no arquivo
- **Linhas 348-418:** `cp()` - ✅ **CORRIGIDO**: Implementada cópia recursiva de diretórios
- **Linhas 410-444:** `executar()` - ✅ **NOVO**: Comando de execução com PERM_EXEC

### `src/header/sistema_arquivos.h`
- **Linha 44:** Adicionado método `executar(string nome)`

### `src/impl/fs_sim.cpp`
- **Linhas 100-105:** Comando `exec` integrado ao CLI

### `src/impl/cliente.cpp`
- **Linhas 8-24:** Help atualizado com comando `exec`

### `README.md`
- **Linhas 13-20:** ✅ **CORRIGIDO**: Instruções de compilação
- **Linhas 25-44:** Tabela de comandos atualizada
- **Linhas 79-87:** Operações com arquivos atualizadas
- **Linhas 131-143:** Seção sobre comandos que verificam permissões

### `test_permissions.txt` (Novo)
- Arquivo de teste criado para demonstrar todas as verificações de permissões

---

## 🔧 Detalhes Técnicos das Correções

### Verificações de Permissões Implementadas

| Comando | Permissão Verificada | Onde |
|---------|---------------------|------|
| `cd` | Execute (x) no diretório | `file_system.cpp:115-118` |
| `ls` | Read (r) no diretório | `file_system.cpp:227-230` |
| `cat` | Read (r) no arquivo | `file_system.cpp:211-214` |
| `echo` | Write (w) no arquivo | `file_system.cpp:170-173` |
| `exec` | Execute (x) no arquivo | `file_system.cpp:428-431` |
| `rm` | Write (w) no arquivo + diretório | `file_system.cpp:296-299` |
| `mv` | Write (w) no arquivo + diretório | `file_system.cpp:333-336` |
| `cp` | Read (r) origem + Write (w) destino | `file_system.cpp:364-372` |

### Implementação da Cópia Recursiva

```cpp
// Função auxiliar para cópia recursiva
void copiarDiretorioRecursivo(shared_ptr<FCB> origem, shared_ptr<FCB> destino,
                              int usuarioAtual, int grupoAtual,
                              function<bool(shared_ptr<FCB>, int)> verificarPermissao) {
    // 1. Criar diretório destino
    auto novoDir = make_shared<FCB>(destino->nome, DIRECTORY, usuarioAtual, grupoAtual,
                                   origem->permProprietario, origem->permGrupo, origem->permOutros,
                                   destino->pai);

    // 2. Copiar recursivamente todos os filhos
    for (auto& [nome, filho] : origem->filhos) {
        if (filho->tipo == DIRECTORY) {
            // Recursão para subdiretórios
            copiarDiretorioRecursivo(filho, novoSubDir, usuarioAtual, grupoAtual, verificarPermissao);
        } else {
            // Copiar arquivos (referências aos blocos)
            novoArquivo->indicesBlocos = filho->indicesBlocos;
        }
    }
}
```

---

## 🧪 Validação das Correções

### Cenários de Teste Implementados

O arquivo `test_permissions.txt` testa:

1. **Navegação com permissões:**
   ```bash
   mkdir testdir
   cd testdir  # Deve funcionar
   chmod testdir 000  # Remove todas as permissões
   cd /        # Volta para raiz
   cd testdir  # Deve falhar: "Permissao negada (Execute)"
   ```

2. **Execução de arquivos:**
   ```bash
   touch arquivo.txt
   chmod arquivo.txt 600  # rw-------
   exec arquivo.txt       # Deve falhar: "Permissao negada (Execute)"
   chmod arquivo.txt 755  # rwxr-xr-x
   exec arquivo.txt       # Deve funcionar
   ```

3. **Cópia recursiva:**
   ```bash
   mkdir sourcedir
   cd sourcedir
   touch file1.txt
   mkdir subdir
   cp sourcedir destdir  # Cria cópia completa
   ```

### Verificação de Conformidade

✅ **Req. 3.1:** Estrutura de diretórios em árvore - OK
✅ **Req. 3.2:** FCB com todos os metadados - OK
✅ **Req. 3.3:** Controle RWX completo - **CORRIGIDO**
✅ **Req. 3.4:** Alocação indexada de blocos - OK

---

## 🚀 Como Usar as Correções

### Compilação (Corrigida):
```bash
# Com Makefile
make

# Ou manualmente
g++ -std=c++17 -Isrc/header src/impl/*.cpp -o fs_sim
```

### Execução:
```bash
# Interativo
./fs_sim

# Com teste de permissões
./fs_sim < test_permissions.txt
```

### Comandos Disponíveis (Atualizados):
- Todos os comandos originais
- **`exec <arquivo>`** - Executa arquivo (verifica permissão x)

---

## 📊 Impacto das Correções

### Antes vs. Depois

| Aspecto | Antes | Depois |
|---------|-------|--------|
| `cd` | Navegação livre | Verifica Execute (x) |
| `ls` | Listagem livre | Verifica Read (r) |
| Execução | Não implementada | Comando `exec` com verificação |
| `rm`/`mv` | Só diretório | Arquivo + diretório |
| `cp` | Só arquivos | Arquivos + diretórios recursivos |
| Documentação | Compilação quebrada | Instruções corretas |

### Segurança Implementada
- **Controle de acesso obrigatório** conforme Req. 3.3
- **Verificações em todas as operações** (não apenas escrita)
- **Modelo Unix completo**: owner/group/others com RWX

---

## 🎯 Conclusão

Todas as correções foram implementadas mantendo:
- **Compatibilidade** com código existente
- **Performance** (verificação apenas quando necessário)
- **Clareza** do código e documentação
- **Conformidade total** com o enunciado

O simulador agora implementa um sistema de arquivos Unix-like completo com controle de acesso robusto! 🎉
