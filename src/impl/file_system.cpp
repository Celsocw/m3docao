#include "../header/sistema_arquivos.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <functional>

using namespace std;

FileSystem::FileSystem() {
    usuarioAtual = 0;
    grupoAtual = 0;
    raiz = make_shared<FCB>("/", DIRECTORY, 0, 0, 7, 5, 5, nullptr);
    raiz->pai = raiz;
    diretorioAtual = raiz;
}

bool FileSystem::verificarPermissao(shared_ptr<FCB> arquivo, int permRequerida) {
    int permEfetiva;
    if (arquivo->idProprietario == usuarioAtual) {
        permEfetiva = arquivo->permProprietario;
    } else if (arquivo->idGrupo == grupoAtual) {
        permEfetiva = arquivo->permGrupo;
    } else {
        permEfetiva = arquivo->permOutros;
    }
    return (permEfetiva & permRequerida) != 0;
}

string FileSystem::tempoParaString(time_t t) {
    struct tm *tm = localtime(&t);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", tm);
    return string(buf);
}

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
    if (usuarioAtual != 0 && !verificarPermissao(diretorioAtual, PERM_WRITE)) {
        cout << "Erro: Permissao negada (Write no diretorio).\n";
        return;
    }
    auto novoDiretorio = make_shared<FCB>(nome, DIRECTORY, usuarioAtual, grupoAtual, 7, 5, 5, diretorioAtual);
    diretorioAtual->filhos[nome] = novoDiretorio;
    cout << "Diretorio criado: " << nome << endl;
}

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
                auto pai = dir->pai.lock();
                if (usuarioAtual != 0 && !verificarPermissao(pai, PERM_EXEC)) {
                    cout << "Erro: Permissao negada (Execute no diretorio pai).\n";
                    return;
                }
                dir = pai;
            }
        } else {
            if (dir->filhos.count(comp)) {
                auto alvo = dir->filhos[comp];
                if (alvo->tipo == DIRECTORY) {
                    if (usuarioAtual != 0 && !verificarPermissao(alvo, PERM_EXEC)) {
                        cout << "Erro: Permissao negada (Execute).\n";
                        return;
                    }
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

void FileSystem::touch(string nome, FileType tipo) {
    if (diretorioAtual->filhos.count(nome)) {
        time(&diretorioAtual->filhos[nome]->modificadoEm);
        return;
    }
    if (usuarioAtual != 0 && !verificarPermissao(diretorioAtual, PERM_WRITE)) {
        cout << "Erro: Permissao negada (Write no diretorio).\n";
        return;
    }
    auto novoArquivo = make_shared<FCB>(nome, tipo, usuarioAtual, grupoAtual, 6, 4, 4, diretorioAtual);
    try {
        novoArquivo->indicesBlocos = disco.alocarBlocos(0); 
        diretorioAtual->filhos[nome] = novoArquivo;
        cout << "Arquivo criado: " << nome << " (tipo: " << tipoArquivoString(tipo) << ")\n";
    } catch (exception& e) {
        cout << e.what() << endl;
    }
}

void FileSystem::echo(string nome, string conteudo) {
    if (!diretorioAtual->filhos.count(nome)) {
        touch(nome);
    }
    auto arquivo = diretorioAtual->filhos[nome];
    if (arquivo->tipo == DIRECTORY) {
        cout << "Erro: Nao pode escrever em um diretorio.\n";
        return;
    }
    if (!verificarPermissao(arquivo, PERM_WRITE)) {
        cout << "Erro: Permissao negada (Write).\n";
        return;
    }
    vector<int> oldIndices = arquivo->indicesBlocos;
    try {
        vector<int> newIndices = disco.alocarBlocos(conteudo.size());
        disco.escreverDados(newIndices, conteudo);
        disco.liberarBlocos(oldIndices);
        arquivo->indicesBlocos = newIndices;
        arquivo->tamanho = conteudo.size();
        time(&arquivo->modificadoEm);
        cout << "Gravado com sucesso.\n";
    } catch (exception& e) {
        cout << e.what() << endl;
    }
}

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
    if (!verificarPermissao(arquivo, PERM_READ)) {
        cout << "Erro: Permissao negada (Read).\n";
        return;
    }
    time(&arquivo->acessadoEm);
    string conteudo = disco.lerDados(arquivo->indicesBlocos, arquivo->tamanho);
    cout << conteudo << endl;
}

void FileSystem::ls() {
    if (usuarioAtual != 0 && !verificarPermissao(diretorioAtual, PERM_READ)) {
        cout << "Erro: Permissao negada (Read).\n";
        return;
    }
    cout << left << setw(12) << "PERM"
         << setw(10) << "TIPO"
         << setw(8)  << "TAM"
         << setw(8)  << "UID"
         << setw(8)  << "GID"
         << setw(18) << "MODIFICADO"
         << "NOME" << endl;
    for (auto const& [chave, val] : diretorioAtual->filhos) {
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

void FileSystem::chmod(string nome, int permOctal) {
    if (!diretorioAtual->filhos.count(nome)) {
        cout << "Erro: Arquivo nao encontrado.\n";
        return;
    }
    auto arquivo = diretorioAtual->filhos[nome];
    if (usuarioAtual != 0 && arquivo->idProprietario != usuarioAtual) {
        cout << "Erro: Apenas o dono pode mudar permissoes.\n";
        return;
    }
    arquivo->permOutros = permOctal % 10;
    arquivo->permGrupo = (permOctal / 10) % 10;
    arquivo->permProprietario = (permOctal / 100) % 10;
    cout << "Permissoes alteradas para " << permOctal << " (";
    cout << permParaStr(arquivo->permProprietario) << permParaStr(arquivo->permGrupo) << permParaStr(arquivo->permOutros);
    cout << ")\n";
}

void FileSystem::removerRecursivo(shared_ptr<FCB> alvo) {
    if (alvo->tipo == DIRECTORY) {
        for (auto& [nome, filho] : alvo->filhos) {
            removerRecursivo(filho);
        }
        alvo->filhos.clear();
    }
    disco.liberarBlocos(alvo->indicesBlocos);
}

void FileSystem::rm(string nome, bool recursivo) {
    if (!diretorioAtual->filhos.count(nome)) {
        cout << "Erro: Nao encontrado.\n";
        return;
    }
    auto alvo = diretorioAtual->filhos[nome];
    if (usuarioAtual != 0 && !verificarPermissao(diretorioAtual, PERM_WRITE)) {
         cout << "Erro: Permissao negada (Write no diretorio).\n";
         return;
    }
    if (alvo->tipo != DIRECTORY && usuarioAtual != 0 && !verificarPermissao(alvo, PERM_WRITE)) {
        cout << "Erro: Permissao negada (Write no arquivo).\n";
        return;
    }
    if (alvo->tipo == DIRECTORY) {
        if (!alvo->filhos.empty() && !recursivo) {
            cout << "Erro: Diretorio nao esta vazio. Use 'rm -r' para remover recursivamente.\n";
            return;
        }
        removerRecursivo(alvo);
    } else {
        disco.liberarBlocos(alvo->indicesBlocos);
    }
    diretorioAtual->filhos.erase(nome);
    cout << "Removido: " << nome << endl;
}

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
    if (usuarioAtual != 0 && !verificarPermissao(diretorioAtual, PERM_WRITE)) {
         cout << "Erro: Permissao negada (Write no diretorio).\n";
         return;
    }
    if (usuarioAtual != 0 && !verificarPermissao(arquivo, PERM_WRITE)) {
        cout << "Erro: Permissao negada (Write no arquivo).\n";
        return;
    }
    arquivo->nome = nomeNovo;
    diretorioAtual->filhos[nomeNovo] = arquivo;
    diretorioAtual->filhos.erase(nomeAntigo);
    time(&arquivo->modificadoEm);
    cout << "Movido/Renomeado de " << nomeAntigo << " para " << nomeNovo << endl;
}

void copiarDiretorioRecursivo(shared_ptr<FCB> origem, shared_ptr<FCB> destino, int usuarioAtual, int grupoAtual,
                              function<bool(shared_ptr<FCB>, int)> verificarPermissao) {
    auto novoDir = make_shared<FCB>(destino->nome, DIRECTORY, usuarioAtual, grupoAtual,
                                   origem->permProprietario, origem->permGrupo, origem->permOutros, destino->pai);
    destino->pai.lock()->filhos[destino->nome] = novoDir;
    for (auto& [nome, filho] : origem->filhos) {
        if (filho->tipo == DIRECTORY) {
            auto novoSubDir = make_shared<FCB>(nome, DIRECTORY, filho->idProprietario, filho->idGrupo,
                                             filho->permProprietario, filho->permGrupo, filho->permOutros, novoDir);
            copiarDiretorioRecursivo(filho, novoSubDir, usuarioAtual, grupoAtual, verificarPermissao);
        } else {
            auto novoArquivo = make_shared<FCB>(nome, filho->tipo, filho->idProprietario, filho->idGrupo,
                                              filho->permProprietario, filho->permGrupo, filho->permOutros, novoDir);
            novoArquivo->tamanho = filho->tamanho;
            novoArquivo->indicesBlocos = filho->indicesBlocos;
            novoDir->filhos[nome] = novoArquivo;
        }
    }
}

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
    if (usuarioAtual != 0 && !verificarPermissao(arquivoOrigem, PERM_READ)) {
        cout << "Erro: Permissao negada (Read).\n";
        return;
    }
    if (usuarioAtual != 0 && !verificarPermissao(diretorioAtual, PERM_WRITE)) {
        cout << "Erro: Permissao negada (Write no diretorio).\n";
        return;
    }
    if (arquivoOrigem->tipo == DIRECTORY) {
        auto novoDir = make_shared<FCB>(nomeDestino, DIRECTORY, usuarioAtual, grupoAtual,
                                       arquivoOrigem->permProprietario, arquivoOrigem->permGrupo,
                                       arquivoOrigem->permOutros, diretorioAtual);
        copiarDiretorioRecursivo(arquivoOrigem, novoDir, usuarioAtual, grupoAtual,
                                [this](shared_ptr<FCB> f, int p) { return verificarPermissao(f, p); });
    } else {
        string conteudo = disco.lerDados(arquivoOrigem->indicesBlocos, arquivoOrigem->tamanho);
        touch(nomeDestino, arquivoOrigem->tipo);
        echo(nomeDestino, conteudo);
    }
    cout << "Copiado de " << nomeOrigem << " para " << nomeDestino << endl;
}

void FileSystem::stat(string nome) {
    if (!diretorioAtual->filhos.count(nome)) {
        cout << "Erro: Arquivo nao encontrado.\n";
        return;
    }
    auto f = diretorioAtual->filhos[nome];
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

void FileSystem::executar(string nome) {
    if (!diretorioAtual->filhos.count(nome)) {
        cout << "Erro: Arquivo nao encontrado.\n";
        return;
    }
    auto arquivo = diretorioAtual->filhos[nome];
    if (arquivo->tipo == DIRECTORY) {
        cout << "Erro: Nao pode executar um diretorio.\n";
        return;
    }
    if (!verificarPermissao(arquivo, PERM_EXEC)) {
        cout << "Erro: Permissao negada (Execute).\n";
        return;
    }
    if (arquivo->tipo == TYPE_PROGRAM) {
        cout << "Executando programa: " << arquivo->nome << "\n";
        cout << "Conteudo do programa seria executado aqui...\n";
    } else {
        cout << "Arquivo '" << arquivo->nome << "' executado (tipo: " << tipoArquivoString(arquivo->tipo) << ")\n";
    }
    time(&arquivo->acessadoEm);
}

void FileSystem::trocarUsuario(int uid, int gid) {
    usuarioAtual = uid;
    if (gid >= 0) grupoAtual = gid;
    cout << "Usuario alterado para UID: " << usuarioAtual << ", GID: " << grupoAtual << endl;
}

void FileSystem::quemSou() {
    cout << "UID: " << usuarioAtual << ", GID: " << grupoAtual << endl;
}

string FileSystem::obterCaminho() {
    if (diretorioAtual == raiz) return "/";
    vector<string> partes;
    shared_ptr<FCB> atual = diretorioAtual;
    while (atual != raiz) {
        partes.push_back(atual->nome);
        auto paiPtr = atual->pai.lock();
        if (!paiPtr || paiPtr == atual) break;
        atual = paiPtr;
    }
    string caminho = "";
    for (int i = partes.size() - 1; i >= 0; i--) {
        caminho += "/" + partes[i];
    }
    return caminho.empty() ? "/" : caminho;
}

#include "../header/sistema_arquivos.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <functional>

using namespace std;

// Constructor
FileSystem::FileSystem() {
    usuarioAtual = 0;  // Usuário inicial é root (UID 0)
    grupoAtual = 0;    // Grupo inicial é root (GID 0)
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
    if (usuarioAtual != 0 && !verificarPermissao(diretorioAtual, PERM_WRITE)) {
        cout << "Erro: Permissao negada (Write no diretorio).\n";
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
                auto pai = dir->pai.lock();
                // Verifica permissão de execução no diretório pai para "atravessar"
                if (usuarioAtual != 0 && !verificarPermissao(pai, PERM_EXEC)) {
                    cout << "Erro: Permissao negada (Execute no diretorio pai).\n";
                    return;
                }
                dir = pai;
            }
        } else {
            if (dir->filhos.count(comp)) {
                auto alvo = dir->filhos[comp];
                if (alvo->tipo == DIRECTORY) {
                    // Verifica permissão de execução no diretório alvo para entrar
                    if (usuarioAtual != 0 && !verificarPermissao(alvo, PERM_EXEC)) {
                        cout << "Erro: Permissao negada (Execute).\n";
                        return;
                    }
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
    // Verifica permissão de escrita no diretório atual (root ignora)
    if (usuarioAtual != 0 && !verificarPermissao(diretorioAtual, PERM_WRITE)) {
        cout << "Erro: Permissao negada (Write no diretorio).\n";
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
    // Verifica permissão de leitura no diretório atual (root ignora)
    if (usuarioAtual != 0 && !verificarPermissao(diretorioAtual, PERM_READ)) {
        cout << "Erro: Permissao negada (Read).\n";
        return;
    }

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
    
    // Apenas o dono ou root (UID 0) pode mudar permissões
    if (usuarioAtual != 0 && arquivo->idProprietario != usuarioAtual) {
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

// Helper: Remove recursivamente um FCB e seus filhos
void FileSystem::removerRecursivo(shared_ptr<FCB> alvo) {
    if (alvo->tipo == DIRECTORY) {
        // Remove todos os filhos recursivamente
        for (auto& [nome, filho] : alvo->filhos) {
            removerRecursivo(filho);
        }
        alvo->filhos.clear();
    }
    // Libera blocos no disco
    disco.liberarBlocos(alvo->indicesBlocos);
}

void FileSystem::rm(string nome, bool recursivo) {
    if (!diretorioAtual->filhos.count(nome)) {
        cout << "Erro: Nao encontrado.\n";
        return;
    }
    auto alvo = diretorioAtual->filhos[nome];

    // Verifica permissão de escrita no diretório pai (root ignora)
    if (usuarioAtual != 0 && !verificarPermissao(diretorioAtual, PERM_WRITE)) {
         cout << "Erro: Permissao negada (Write no diretorio).\n";
         return;
    }

    // Para arquivos, verifica também permissão de escrita no próprio arquivo (root ignora)
    if (alvo->tipo != DIRECTORY && usuarioAtual != 0 && !verificarPermissao(alvo, PERM_WRITE)) {
        cout << "Erro: Permissao negada (Write no arquivo).\n";
        return;
    }

    // Se for diretório, verifica se está vazio ou se -r foi passado
    if (alvo->tipo == DIRECTORY) {
        if (!alvo->filhos.empty() && !recursivo) {
            cout << "Erro: Diretorio nao esta vazio. Use 'rm -r' para remover recursivamente.\n";
            return;
        }
        // Remove recursivamente se necessário
        removerRecursivo(alvo);
    } else {
        // Libera blocos no disco (Req 3.4)
        disco.liberarBlocos(alvo->indicesBlocos);
    }

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

    // Req 3.3: Checa permissão de escrita no diretório atual (root ignora)
    if (usuarioAtual != 0 && !verificarPermissao(diretorioAtual, PERM_WRITE)) {
         cout << "Erro: Permissao negada (Write no diretorio).\n";
         return;
    }

    // Verifica permissão de escrita no próprio arquivo (root ignora)
    if (usuarioAtual != 0 && !verificarPermissao(arquivo, PERM_WRITE)) {
        cout << "Erro: Permissao negada (Write no arquivo).\n";
        return;
    }

    // Renomeia (update key in map)
    arquivo->nome = nomeNovo;
    diretorioAtual->filhos[nomeNovo] = arquivo;
    diretorioAtual->filhos.erase(nomeAntigo);

    time(&arquivo->modificadoEm);
    cout << "Movido/Renomeado de " << nomeAntigo << " para " << nomeNovo << endl;
}

// Helper: Copiar diretório recursivamente
void copiarDiretorioRecursivo(shared_ptr<FCB> origem, shared_ptr<FCB> destino, int usuarioAtual, int grupoAtual,
                              function<bool(shared_ptr<FCB>, int)> verificarPermissao) {
    // Cria o diretório destino
    auto paiDestino = destino->pai.lock();
    auto novoDir = make_shared<FCB>(destino->nome, DIRECTORY, usuarioAtual, grupoAtual,
                                   origem->permProprietario, origem->permGrupo, origem->permOutros, paiDestino);
    paiDestino->filhos[destino->nome] = novoDir;

    // Copia todos os filhos recursivamente
    for (auto& [nome, filho] : origem->filhos) {
        if (filho->tipo == DIRECTORY) {
            // Cria subdiretório e copia recursivamente
            auto novoSubDir = make_shared<FCB>(nome, DIRECTORY, filho->idProprietario, filho->idGrupo,
                                             filho->permProprietario, filho->permGrupo, filho->permOutros, novoDir);
            copiarDiretorioRecursivo(filho, novoSubDir, usuarioAtual, grupoAtual, verificarPermissao);
        } else {
            // Copia arquivo
            auto novoArquivo = make_shared<FCB>(nome, filho->tipo, filho->idProprietario, filho->idGrupo,
                                              filho->permProprietario, filho->permGrupo, filho->permOutros, novoDir);
            novoArquivo->tamanho = filho->tamanho;
            novoArquivo->indicesBlocos = filho->indicesBlocos; // Copia referências aos blocos
            novoDir->filhos[nome] = novoArquivo;
        }
    }
}

// Copiar (cp) - agora suporta cópia recursiva de diretórios
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

    // Verifica permissão de leitura no arquivo/diretório de origem (root ignora)
    if (usuarioAtual != 0 && !verificarPermissao(arquivoOrigem, PERM_READ)) {
        cout << "Erro: Permissao negada (Read).\n";
        return;
    }

    // Verifica permissão de escrita no diretório destino (root ignora)
    if (usuarioAtual != 0 && !verificarPermissao(diretorioAtual, PERM_WRITE)) {
        cout << "Erro: Permissao negada (Write no diretorio).\n";
        return;
    }

    if (arquivoOrigem->tipo == DIRECTORY) {
        // Cópia recursiva de diretório
        auto novoDir = make_shared<FCB>(nomeDestino, DIRECTORY, usuarioAtual, grupoAtual,
                                       arquivoOrigem->permProprietario, arquivoOrigem->permGrupo,
                                       arquivoOrigem->permOutros, diretorioAtual);

        copiarDiretorioRecursivo(arquivoOrigem, novoDir, usuarioAtual, grupoAtual,
                                [this](shared_ptr<FCB> f, int p) { return verificarPermissao(f, p); });
    } else {
        // Cópia de arquivo regular
        // Lê dados originais
        string conteudo = disco.lerDados(arquivoOrigem->indicesBlocos, arquivoOrigem->tamanho);

        // Cria novo arquivo
        touch(nomeDestino, arquivoOrigem->tipo);

        // Escreve dados no novo arquivo
        echo(nomeDestino, conteudo);
    }

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

// Novo comando: executar arquivo (Req 3.3 - testar PERM_EXEC)
void FileSystem::executar(string nome) {
    if (!diretorioAtual->filhos.count(nome)) {
        cout << "Erro: Arquivo nao encontrado.\n";
        return;
    }
    auto arquivo = diretorioAtual->filhos[nome];

    if (arquivo->tipo == DIRECTORY) {
        cout << "Erro: Nao pode executar um diretorio.\n";
        return;
    }

    // Req 3.3: Verifica permissão de execução
    if (!verificarPermissao(arquivo, PERM_EXEC)) {
        cout << "Erro: Permissao negada (Execute).\n";
        return;
    }

    // Simula execução baseada no tipo de arquivo
    if (arquivo->tipo == TYPE_PROGRAM) {
        cout << "Executando programa: " << arquivo->nome << "\n";
        cout << "Conteudo do programa seria executado aqui...\n";
    } else {
        cout << "Arquivo '" << arquivo->nome << "' executado (tipo: " << tipoArquivoString(arquivo->tipo) << ")\n";
    }

    // Atualiza timestamp de acesso
    time(&arquivo->acessadoEm);
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
    // Reconstrói o caminho completo subindo pela árvore até a raiz
    if (diretorioAtual == raiz) return "/";
    
    vector<string> partes;
    shared_ptr<FCB> atual = diretorioAtual;
    
    while (atual != raiz) {
        partes.push_back(atual->nome);
        auto paiPtr = atual->pai.lock();
        if (!paiPtr || paiPtr == atual) break; // Segurança contra loop infinito
        atual = paiPtr;
    }
    
    // Monta o caminho na ordem correta (de raiz para atual)
    string caminho = "";
    for (int i = partes.size() - 1; i >= 0; i--) {
        caminho += "/" + partes[i];
    }
    
    return caminho.empty() ? "/" : caminho;
}
