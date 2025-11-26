# Mini Sistema de Arquivos em Memória
## Trabalho M3 - Sistemas Operacionais - UNIVALI

Este projeto implementa um simulador de sistema de arquivos em memória em C++, demonstrando os conceitos fundamentais de sistemas operacionais Unix-like.

---

## Como Compilar e Executar

### Requisitos
- Compilador C++ com suporte a C++17 (G++, Clang)

### Compilação
```bash
g++ -std=c++17 fs_sim.cpp -o fs_sim
```

### Execução
```bash
./fs_sim
```

---

## Comandos Disponíveis

| Comando | Descrição |
|---------|-----------|
| `mkdir <nome>` | Cria um diretório |
| `cd <nome\|..\|/>` | Navega entre diretórios |
| `ls` | Lista arquivos com metadados (permissões, tipo, tamanho, UID, GID) |
| `touch <nome> [tipo]` | Cria arquivo (tipo: text/num/bin/prog) |
| `echo <arq> <conteudo>` | Escreve conteúdo no arquivo |
| `cat <arq>` | Lê conteúdo do arquivo |
| `cp <orig> <dest>` | Copia arquivo |
| `mv <orig> <dest>` | Move/renomeia arquivo |
| `rm <nome>` | Remove arquivo ou diretório |
| `chmod <arq> <perm>` | Altera permissões (ex: 755, 644) |
| `stat <arq>` | Mostra metadados detalhados (inode, blocos) |
| `su <uid> [gid]` | Troca usuário/grupo atual |
| `whoami` | Mostra usuário/grupo atual |
| `help` | Mostra ajuda |
| `exit` | Sai do simulador |

---

## Estrutura do Projeto e Conceitos Teóricos

### 1. Conceito de Arquivo e seus Atributos (Req 3.2)

Um **arquivo** é um tipo de dado abstrato que representa uma coleção de informações relacionadas. Na implementação, cada arquivo é representado por um **File Control Block (FCB)**, que armazena:

```cpp
struct FCB {
    int inodeId;          // ID único (simula inode)
    string name;          // Nome do arquivo
    FileType type;        // Tipo: TEXT, NUMERIC, BINARY, PROGRAM, DIRECTORY
    int size;             // Tamanho em bytes
    int ownerId;          // ID do proprietário
    int groupId;          // ID do grupo
    int ownerPerm;        // Permissões do dono (0-7)
    int groupPerm;        // Permissões do grupo (0-7)
    int otherPerm;        // Permissões de outros (0-7)
    time_t createdAt;     // Data de criação
    time_t modifiedAt;    // Data de modificação
    time_t accessedAt;    // Data de acesso
    vector<int> blockIndices;  // Ponteiros para blocos no disco
    // ...
};
```

O **inode** (index node) é simulado pelo campo `inodeId`, que é um identificador único gerado por um contador global. Em sistemas reais, o inode contém metadados e ponteiros para os blocos de dados.

### 2. Operações com Arquivos

As operações básicas implementadas seguem o padrão Unix:

- **Criar (touch)**: Aloca um FCB e um bloco inicial no disco virtual
- **Escrever (echo)**: Libera blocos antigos, aloca novos baseado no tamanho, escreve dados
- **Ler (cat)**: Verifica permissões, lê dados dos blocos referenciados, atualiza `accessedAt`
- **Copiar (cp)**: Lê dados da origem, cria novo arquivo, escreve dados (nova alocação de blocos)
- **Mover/Renomear (mv)**: Atualiza referências na árvore de diretórios (não move dados)
- **Excluir (rm)**: Libera blocos no disco, remove entrada do diretório pai

### 3. Estrutura de Diretórios em Árvore (Req 3.1)

A estrutura de diretórios é implementada como uma **árvore N-ária** usando ponteiros inteligentes:

```cpp
map<string, shared_ptr<FCB>> children;  // Filhos (arquivos e subdiretórios)
weak_ptr<FCB> parent;                    // Ponteiro para o pai (evita referência circular)
```

**Vantagens da estrutura em árvore:**
- **Eficiência**: Busca rápida de arquivos em O(log n) por nível
- **Nomeação**: Permite nomes duplicados em diretórios diferentes
- **Agrupamento**: Organização lógica de arquivos relacionados
- **Navegação**: Suporte a caminhos absolutos (/) e relativos (..)

### 4. Mecanismo de Proteção de Acesso (Req 3.3)

O controle de acesso segue o modelo Unix com **três classes de usuários**:

| Classe | Descrição |
|--------|-----------|
| **Owner** | Proprietário do arquivo (UID) |
| **Group** | Membros do grupo do arquivo (GID) |
| **Others** | Todos os outros usuários |

Cada classe possui três bits de permissão (bitmask):

| Bit | Valor | Significado |
|-----|-------|-------------|
| R (Read) | 4 | Leitura permitida |
| W (Write) | 2 | Escrita permitida |
| X (Execute) | 1 | Execução permitida |

**Implementação do chmod:**
```cpp
// chmod 755 → owner=7(rwx), group=5(r-x), other=5(r-x)
file->otherPerm = octalPerm % 10;         // 5
file->groupPerm = (octalPerm / 10) % 10;  // 5
file->ownerPerm = (octalPerm / 100) % 10; // 7
```

**Verificação de permissões:**
```cpp
bool checkPermission(shared_ptr<FCB> file, int requiredPerm) {
    int effectivePerm;
    if (file->ownerId == currentUser)
        effectivePerm = file->ownerPerm;
    else if (file->groupId == currentGroup)
        effectivePerm = file->groupPerm;
    else
        effectivePerm = file->otherPerm;
    
    return (effectivePerm & requiredPerm) != 0;
}
```

### 5. Simulação de Alocação de Blocos (Req 3.4)

O disco virtual é simulado por um array linear de bytes:

```cpp
class VirtualDisk {
    vector<char> rawData;      // "Disco" simulado (100 blocos × 64 bytes)
    vector<bool> blockBitmap;  // Mapa de bits para blocos livres/ocupados
};
```

**Método de alocação: Indexada**

Na alocação indexada, o FCB mantém uma lista de índices de blocos onde os dados estão armazenados:

```cpp
vector<int> blockIndices;  // Ex: [0, 5, 12] → dados nos blocos 0, 5 e 12
```

**Processo de alocação:**
1. Calcula quantidade de blocos necessários: `ceil(tamanho / BLOCK_SIZE)`
2. Percorre o bitmap procurando blocos livres
3. Marca blocos como ocupados e retorna seus índices
4. Os índices são armazenados no FCB

**Vantagens da alocação indexada:**
- Acesso direto a qualquer bloco
- Sem fragmentação externa
- Facilita expansão do arquivo

---

## Exemplo de Uso

```
user@/$ mkdir docs
Diretorio criado: docs

user@/$ cd docs
user@.../docs$ touch relatorio.txt
Arquivo criado: relatorio.txt (tipo: TEXT)

user@.../docs$ echo relatorio.txt Conteudo do relatorio
Gravado com sucesso.

user@.../docs$ stat relatorio.txt
  File: relatorio.txt
  Size: 21 bytes
 Inode: 2
  Type: TEXT
Blocks: [0]
Access: (644/rw-r--r--)
   Uid: 1  Gid: 1
Access: 2025-11-26 19:48
Modify: 2025-11-26 19:48
 Birth: 2025-11-26 19:48

user@.../docs$ chmod relatorio.txt 600
Permissoes alteradas para 600 (rw-------)

user@.../docs$ su 2 2
Usuario alterado para UID: 2, GID: 2

user@.../docs$ cat relatorio.txt
Erro: Permissao negada (Read).
```

---

## Comparação com Comandos Linux Reais

| Simulador | Linux Real | Comportamento |
|-----------|------------|---------------|
| `ls` | `ls -l` | Lista com permissões, tipo, tamanho, UID, GID |
| `stat arq` | `stat arq` | Mostra inode, blocos, timestamps |
| `chmod 755 arq` | `chmod 755 arq` | Altera permissões no formato octal |
| `touch arq prog` | `touch arq` | Cria arquivo (simulador permite especificar tipo) |

---

## Autores

Trabalho desenvolvido para a disciplina de **Sistemas Operacionais** - UNIVALI  
Professor: Michael D C Alves
