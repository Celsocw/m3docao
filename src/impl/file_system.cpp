
#include "../header/sistema_arquivos.h"
#include <iostream>
#include <iomanip>
#include <ctime>

using namespace std;

// Constructor
FileSystem::FileSystem() {
    usuarioAtual = 1;  // Usuário inicial (simula root)
    grupoAtual = 1; // Grupo inicial
    // Cria diretório raiz com permissões 755 (rwxr-xr-x)
    raiz = make_shared<FCB>("/", DIRECTORY, 0, 0, 7, 5, 5, nullptr);
    raiz->pai = raiz; // Pai do root é ele mesmo
    diretorioAtual = raiz;
}

// Helper: Verifica permissão (Req 3.3 - owner/group/others)
bool FileSystem::verificarPermissao(shared_ptr<FCB> arquivo, int permRequerida) {
    int permEfetiva;
    
    // Determina qual conjunto de permissões usar
    if (arquivo->idProprietario == usuarioAtual) {
        permEfetiva = arquivo->permProprietario;  // Owner
    } else if (arquivo->idGrupo == grupoAtual) {
        permEfetiva = arquivo->permGrupo;  // Group
    } else {
        permEfetiva = arquivo->permOutros;  // Others (public)
    }
    
    // Verifica se a permissão requerida está no bitmask
    return (permEfetiva & permRequerida) != 0;
}

// Utilitário para formatar tempo
string FileSystem::tempoParaString(time_t t) {
    struct tm *tm = localtime(&t);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", tm);
    return string(buf);
}

// Helper: Converte FileType para string
string FileSystem::tipoArquivoString(FileType t) {
    switch(t) {
        case DIRECTORY: return "DIR";
        case TYPE_TEXT: return "TEXT";
        case TYPE_NUMERIC: return "NUMERIC";
        case TYPE_BINARY: return "BINARY";
        case TYPE_PROGRAM: return "PROGRAM";
        default: return "UNKNOWN";
    }
}

// Helper: Converte permissão numérica (0-7) para string rwx
string FileSystem::permParaStr(int p) {
    string s = "";
    s += (p & PERM_READ)  ? "r" : "-";
    s += (p & PERM_WRITE) ? "w" : "-";
    s += (p & PERM_EXEC)  ? "x" : "-";
    return s;
}

void FileSystem::mkdir(string nome) {
    if (diretorioAtual->filhos.count(nome)) {
        cout << "Erro: Diretorio ja existe.\n";
        return;
    }
    // Cria novo FCB do tipo Directory com permissões 755 (rwxr-xr-x)
    auto novoDiretorio = make_shared<FCB>(nome, DIRECTORY, usuarioAtual, grupoAtual, 7, 5, 5, diretorioAtual);
    diretorioAtual->filhos[nome] = novoDiretorio;
    cout << "Diretorio criado: " << nome << endl;
}

// Helper: Split string by delimiter
vector<string> split(const string& s, char delimiter) {
    vector<string> tokens;
    string token;
    for (char c : s) {
        if (c == delimiter) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }
    if (!token.empty()) {
        tokens.push_back(token);
    }
    return tokens;
}

void FileSystem::cd(string nome) {
    shared_ptr<FCB> dir;
    if (!nome.empty() && nome[0] == '/') {
        dir = raiz;
    } else {
        dir = diretorioAtual;
    }
    vector<string> components = split(nome, '/');
    for (const string& comp : components) {
        if (comp == "" || comp == ".") {
            continue;
        } else if (comp == "..") {
            if (dir != raiz) {
                dir = dir->pai.lock();
            }
        } else {
            if (dir->filhos.count(comp)) {
                auto alvo = dir->filhos[comp];
                if (alvo->tipo == DIRECTORY) {
                    dir = alvo;
                } else {
                    cout << "Erro: '" << comp << "' nao e um diretorio.\n";
                    return;
                }
            } else {
                cout << "Erro: Diretorio '" << comp << "' nao encontrado.\n";
                return;
            }
        }
    }
    diretorioAtual = dir;
}

// Cria arquivo com tipo especificado (Req 3.2: numérico, caractere, binário, programa)
void FileSystem::touch(string nome, FileType tipo) {
    if (diretorioAtual->filhos.count(nome)) {
        // Atualiza timestamp se já existe
        time(&diretorioAtual->filhos[nome]->modificadoEm);
        return;
    }
    // Cria arquivo com permissões 644 (rw-r--r--)
    auto novoArquivo = make_shared<FCB>(nome, tipo, usuarioAtual, grupoAtual, 6, 4, 4, diretorioAtual);
    
    // Aloca 1 bloco inicial vazio (Req 3.4 - Alocação)
    try {
        novoArquivo->indicesBlocos = disco.alocarBlocos(0); 
        diretorioAtual->filhos[nome] = novoArquivo;
        cout << "Arquivo criado: " << nome << " (tipo: " << tipoArquivoString(tipo) << ")\n";
    } catch (exception& e) {
        cout << e.what() << endl;
    }
}

// Escrever no arquivo (Simula: echo "conteudo" > arquivo)
void FileSystem::echo(string nome, string conteudo) {
    if (!diretorioAtual->filhos.count(nome)) {
        touch(nome); // Cria se não existe
    }
    
    auto arquivo = diretorioAtual->filhos[nome];
    if (arquivo->tipo == DIRECTORY) {
        cout << "Erro: Nao pode escrever em um diretorio.\n";
        return;
    }

    // Req 3.3: Checa permissão de Escrita
    if (!verificarPermissao(arquivo, PERM_WRITE)) {
        cout << "Erro: Permissao negada (Write).\n";
        return;
    }

    // Req 3.4: Realocação de blocos
    // 1. Tenta alocar novos blocos antes de liberar os antigos
    vector<int> oldIndices = arquivo->indicesBlocos;
    
    try {
        // 2. Aloca novos blocos baseados no tamanho do conteúdo
        vector<int> newIndices = disco.alocarBlocos(conteudo.size());
        
        // 3. Escreve no "disco"
        disco.escreverDados(newIndices, conteudo);
        
        // 4. Libera blocos antigos e atualiza FCB
        disco.liberarBlocos(oldIndices);
        arquivo->indicesBlocos = newIndices;
        arquivo->tamanho = conteudo.size();
        time(&arquivo->modificadoEm);
        cout << "Gravado com sucesso.\n";
    } catch (exception& e) {
        cout << e.what() << endl;
    }
}

// Ler arquivo (cat)
void FileSystem::cat(string nome) {
    if (!diretorioAtual->filhos.count(nome)) {
        cout << "Erro: Arquivo nao encontrado.\n";
        return;
    }
    auto arquivo = diretorioAtual->filhos[nome];
    
    if (arquivo->tipo == DIRECTORY) {
        cout << "Erro: E um diretorio.\n";
        return;
    }

    // Req 3.3: Checa permissão de Leitura
    if (!verificarPermissao(arquivo, PERM_READ)) {
        cout << "Erro: Permissao negada (Read).\n";
        return;
    }

    // Atualiza data de acesso (Req 3.2)
    time(&arquivo->acessadoEm);

    // Req 3.4: Busca dados dos blocos
    string conteudo = disco.lerDados(arquivo->indicesBlocos, arquivo->tamanho);
    cout << conteudo << endl;
}

void FileSystem::ls() {
    cout << left << setw(12) << "PERM" 
         << setw(10) << "TIPO" 
         << setw(8)  << "TAM" 
         << setw(8)  << "UID"
         << setw(8)  << "GID"
         << setw(18) << "MODIFICADO" 
         << "NOME" << endl;
    
    for (auto const& [chave, val] : diretorioAtual->filhos) {
        // Formato: drwxr-xr-x ou -rw-r--r--
        string strPerm = (val->tipo == DIRECTORY) ? "d" : "-";
        strPerm += permParaStr(val->permProprietario);
        strPerm += permParaStr(val->permGrupo);
        strPerm += permParaStr(val->permOutros);

        cout << left << setw(12) << strPerm
             << setw(10) << tipoArquivoString(val->tipo)
             << setw(8)  << val->tamanho
             << setw(8)  << val->idProprietario
             << setw(8)  << val->idGrupo
             << setw(18) << tempoParaString(val->modificadoEm)
             << val->nome << endl;
    }
}

// chmod no formato octal: 755, 644, 777, etc. (Req 3.3)
void FileSystem::chmod(string nome, int permOctal) {
    if (!diretorioAtual->filhos.count(nome)) {
        cout << "Erro: Arquivo nao encontrado.\n";
        return;
    }
    auto arquivo = diretorioAtual->filhos[nome];
    
    // Apenas o dono pode mudar permissões
    if (arquivo->idProprietario != usuarioAtual) {
        cout << "Erro: Apenas o dono pode mudar permissoes.\n";
        return;
    }
    
    // Extrai dígitos do octal (ex: 755 -> owner=7, group=5, other=5)
    arquivo->permOutros = permOctal % 10;
    arquivo->permGrupo = (permOctal / 10) % 10;
    arquivo->permProprietario = (permOctal / 100) % 10;
    
    cout << "Permissoes alteradas para " << permOctal << " (";
    cout << permParaStr(arquivo->permProprietario) << permParaStr(arquivo->permGrupo) << permParaStr(arquivo->permOutros);
    cout << ")\n";
}

void FileSystem::rm(string nome) {
    if (!diretorioAtual->filhos.count(nome)) {
        cout << "Erro: Nao encontrado.\n";
        return;
    }
    auto alvo = diretorioAtual->filhos[nome];
    
    if (!verificarPermissao(alvo, PERM_WRITE)) {
         cout << "Erro: Permissao negada.\n";
         return;
    }

    // Libera blocos no disco (Req 3.4)
    disco.liberarBlocos(alvo->indicesBlocos);
    
    // Remove da árvore
    diretorioAtual->filhos.erase(nome);
    cout << "Removido: " << nome << endl;
}

// Renomear/Mover (mv)
void FileSystem::mv(string nomeAntigo, string nomeNovo) {
    if (!diretorioAtual->filhos.count(nomeAntigo)) {
        cout << "Erro: Arquivo de origem nao encontrado.\n";
        return;
    }
    if (diretorioAtual->filhos.count(nomeNovo)) {
        cout << "Erro: Destino ja existe.\n";
        return;
    }

    auto arquivo = diretorioAtual->filhos[nomeAntigo];
    
    // Req 3.3: Checa permissão de escrita no diretório pai (conceitualmente)
    // ou no próprio arquivo dependendo da regra. Vamos simplificar:
    if (arquivo->idProprietario != usuarioAtual) {
         cout << "Erro: Permissao negada (apenas dono pode mover).\n";
         return;
    }

    // Renomeia (update key in map)
    arquivo->nome = nomeNovo;
    diretorioAtual->filhos[nomeNovo] = arquivo;
    diretorioAtual->filhos.erase(nomeAntigo);
    
    time(&arquivo->modificadoEm);
    cout << "Movido/Renomeado de " << nomeAntigo << " para " << nomeNovo << endl;
}

// Copiar (cp)
void FileSystem::cp(string nomeOrigem, string nomeDestino) {
    if (!diretorioAtual->filhos.count(nomeOrigem)) {
        cout << "Erro: Arquivo de origem nao encontrado.\n";
        return;
    }
    if (diretorioAtual->filhos.count(nomeDestino)) {
        cout << "Erro: Destino ja existe.\n";
        return;
    }

    auto arquivoOrigem = diretorioAtual->filhos[nomeOrigem];
    if (arquivoOrigem->tipo == DIRECTORY) {
        cout << "Erro: cp nao suporta diretorios recursivamente nesta versao.\n";
        return;
    }

    // Lê dados originais
    string conteudo = disco.lerDados(arquivoOrigem->indicesBlocos, arquivoOrigem->tamanho);

    // Cria novo arquivo
    touch(nomeDestino, arquivoOrigem->tipo);
    
    // Escreve dados no novo arquivo
    echo(nomeDestino, conteudo);
    cout << "Copiado de " << nomeOrigem << " para " << nomeDestino << endl;
}

void FileSystem::stat(string nome) {
    if (!diretorioAtual->filhos.count(nome)) {
        cout << "Erro: Arquivo nao encontrado.\n";
        return;
    }
    auto f = diretorioAtual->filhos[nome];
    
    // Formato similar ao comando stat do Linux
    cout << "  File: " << f->nome << "\n";
    cout << "  Size: " << f->tamanho << " bytes\n";
    cout << " Inode: " << f->inodeId << "\n";
    cout << "  Type: " << tipoArquivoString(f->tipo) << "\n";
    cout << "Blocks: [";
    for(size_t i = 0; i < f->indicesBlocos.size(); i++) {
        cout << f->indicesBlocos[i];
        if (i < f->indicesBlocos.size() - 1) cout << ", ";
    }
    cout << "]\n";
    cout << "Access: (" << f->permProprietario << f->permGrupo << f->permOutros << "/";
    cout << permParaStr(f->permProprietario) << permParaStr(f->permGrupo) << permParaStr(f->permOutros) << ")\n";
    cout << "   Uid: " << f->idProprietario << "  Gid: " << f->idGrupo << "\n";
    cout << "Access: " << tempoParaString(f->acessadoEm) << "\n";
    cout << "Modify: " << tempoParaString(f->modificadoEm) << "\n";
    cout << " Birth: " << tempoParaString(f->criadoEm) << "\n";
}

// Simula troca de usuário e grupo (Req 3.3: testar owner/group/others)
void FileSystem::trocarUsuario(int uid, int gid) {
    usuarioAtual = uid;
    if (gid >= 0) grupoAtual = gid;
    cout << "Usuario alterado para UID: " << usuarioAtual << ", GID: " << grupoAtual << endl;
}

// Retorna info do usuário atual
void FileSystem::quemSou() {
    cout << "UID: " << usuarioAtual << ", GID: " << grupoAtual << endl;
}

string FileSystem::obterCaminho() {
     // Simplificado: não reconstrói path inteiro, mostra apenas atual
     if (diretorioAtual == raiz) return "/";
     return ".../" + diretorioAtual->nome;
}
