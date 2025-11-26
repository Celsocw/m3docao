#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <memory>
#include <algorithm>
#include <cmath>

using namespace std;

// ==========================================
// CONSTANTES E DEFINIÇÕES (Req 3.3 e 3.4)
// ==========================================
const int BLOCK_SIZE = 64;       // Tamanho pequeno para demonstrar alocação de múltiplos blocos
const int DISK_SIZE_BLOCKS = 100; // Disco simula 100 blocos

// Máscaras de Permissão (RWX) - Req 3.3
const int PERM_READ  = 4; // 100
const int PERM_WRITE = 2; // 010
const int PERM_EXEC  = 1; // 001

// Tipos de Arquivo (Req 3.2: numérico, caractere, binário, programa)
enum FileType { DIRECTORY, TYPE_TEXT, TYPE_NUMERIC, TYPE_BINARY, TYPE_PROGRAM };

// Contador global para gerar IDs únicos (simula inode)
int nextInodeId = 1;

// ==========================================
// 3.4: SIMULAÇÃO DE ALOCAÇÃO DE BLOCOS
// ==========================================
class VirtualDisk {
private:
    // O "Disco" é um array linear de bytes na memória
    vector<char> rawData;
    // Mapa de bits para saber quais blocos estão livres (true = ocupado)
    vector<bool> blockBitmap;

public:
    VirtualDisk() {
        rawData.resize(DISK_SIZE_BLOCKS * BLOCK_SIZE, '\0');
        blockBitmap.resize(DISK_SIZE_BLOCKS, false);
    }

    // Retorna índice de blocos livres
    vector<int> allocateBlocks(int requiredBytes) {
        int blocksNeeded = (int)ceil((double)requiredBytes / BLOCK_SIZE);
        if (blocksNeeded == 0) blocksNeeded = 1; // Mínimo 1 bloco

        vector<int> indices;
        int found = 0;

        // Estratégia: Alocação indexada (procura quaisquer blocos livres)
        for (int i = 0; i < DISK_SIZE_BLOCKS && found < blocksNeeded; i++) {
            if (!blockBitmap[i]) {
                indices.push_back(i);
                blockBitmap[i] = true; // Marca como ocupado
                found++;
            }
        }

        if (found < blocksNeeded) {
            // Rollback se não houver espaço suficiente
            for (int idx : indices) blockBitmap[idx] = false;
            throw runtime_error("Erro: Espaco insuficiente no disco virtual.");
        }
        return indices;
    }

    void freeBlocks(const vector<int>& indices) {
        for (int idx : indices) {
            if (idx >= 0 && idx < DISK_SIZE_BLOCKS) {
                blockBitmap[idx] = false;
                // Opcional: Limpar dados
                fill(rawData.begin() + (idx * BLOCK_SIZE), 
                     rawData.begin() + ((idx + 1) * BLOCK_SIZE), '\0');
            }
        }
    }

    // Escreve dados nos blocos alocados
    void writeData(const vector<int>& indices, const string& content) {
        size_t contentPos = 0;
        for (int idx : indices) {
            int startAddr = idx * BLOCK_SIZE;
            for (int i = 0; i < BLOCK_SIZE && contentPos < content.size(); i++) {
                rawData[startAddr + i] = content[contentPos++];
            }
        }
    }

    // Lê dados dos blocos
    string readData(const vector<int>& indices, int sizeBytes) {
        string content;
        int bytesRead = 0;
        for (int idx : indices) {
            int startAddr = idx * BLOCK_SIZE;
            for (int i = 0; i < BLOCK_SIZE && bytesRead < sizeBytes; i++) {
                if (rawData[startAddr + i] != '\0') {
                    content += rawData[startAddr + i];
                }
                bytesRead++;
            }
        }
        return content;
    }
};

// ==========================================
// 3.2: FILE CONTROL BLOCK (FCB / Inode)
// ==========================================
struct FCB {
    int inodeId;          // ID único (Req 3.2: simula inode)
    string name;
    FileType type;
    int size;
    int ownerId;
    int groupId;          // Req 3.3: para permissões de grupo
    
    // Permissões no formato octal (Req 3.3: owner/group/others)
    // Ex: 755 = rwxr-xr-x, 644 = rw-r--r--
    int ownerPerm;        // 0-7 (rwx)
    int groupPerm;        // 0-7 (rwx)
    int otherPerm;        // 0-7 (rwx)
    
    time_t createdAt;
    time_t modifiedAt;
    time_t accessedAt;    // Req 3.2: data de acesso
    
    // Simulação de Inode: Lista de índices de blocos onde o conteúdo vive
    vector<int> blockIndices; 

    // Para diretórios: mantemos referências aos filhos em memória
    // (Em um FS real, isso estaria dentro do bloco de dados, 
    // mas para o trabalho M3, ponteiros facilitam a estrutura de árvore do Req 3.1)
    map<string, shared_ptr<FCB>> children;
    weak_ptr<FCB> parent; // Para 'cd ..'

    FCB(string n, FileType t, int uid, int gid, int oPerm, int gPerm, int pubPerm, shared_ptr<FCB> par) 
        : name(n), type(t), size(0), ownerId(uid), groupId(gid),
          ownerPerm(oPerm), groupPerm(gPerm), otherPerm(pubPerm), parent(par) {
        inodeId = nextInodeId++;
        time(&createdAt);
        modifiedAt = createdAt;
        accessedAt = createdAt;
    }
};

// ==========================================
// SISTEMA DE ARQUIVOS (Lógica Principal)
// ==========================================
class FileSystem {
private:
    VirtualDisk disk;
    shared_ptr<FCB> root;
    shared_ptr<FCB> currentDir;
    int currentUser;  // ID do usuário atual logado
    int currentGroup; // ID do grupo atual

    // Helper: Verifica permissão (Req 3.3 - owner/group/others)
    bool checkPermission(shared_ptr<FCB> file, int requiredPerm) {
        int effectivePerm;
        
        // Determina qual conjunto de permissões usar
        if (file->ownerId == currentUser) {
            effectivePerm = file->ownerPerm;  // Owner
        } else if (file->groupId == currentGroup) {
            effectivePerm = file->groupPerm;  // Group
        } else {
            effectivePerm = file->otherPerm;  // Others (public)
        }
        
        // Verifica se a permissão requerida está no bitmask
        return (effectivePerm & requiredPerm) != 0;
    }

public:
    FileSystem() {
        currentUser = 1;  // Usuário inicial (simula root)
        currentGroup = 1; // Grupo inicial
        // Cria diretório raiz com permissões 755 (rwxr-xr-x)
        root = make_shared<FCB>("/", DIRECTORY, 0, 0, 7, 5, 5, nullptr);
        root->parent = root; // Pai do root é ele mesmo
        currentDir = root;
    }

    // Utilitário para formatar tempo
    string timeToString(time_t t) {
        struct tm *tm = localtime(&t);
        char buf[20];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", tm);
        return string(buf);
    }

    // --- Comandos (Req 3.1 e 3.2) ---

    void mkdir(string name) {
        if (currentDir->children.count(name)) {
            cout << "Erro: Diretorio ja existe.\n";
            return;
        }
        // Cria novo FCB do tipo Directory com permissões 755 (rwxr-xr-x)
        auto newDir = make_shared<FCB>(name, DIRECTORY, currentUser, currentGroup, 7, 5, 5, currentDir);
        currentDir->children[name] = newDir;
        cout << "Diretorio criado: " << name << endl;
    }

    void cd(string name) {
        if (name == "..") {
            if (currentDir != root)
                currentDir = currentDir->parent.lock();
            return;
        }
        if (name == "/") {
            currentDir = root;
            return;
        }

        if (currentDir->children.count(name)) {
            auto target = currentDir->children[name];
            if (target->type == DIRECTORY) {
                currentDir = target;
            } else {
                cout << "Erro: '" << name << "' nao e um diretorio.\n";
            }
        } else {
            cout << "Erro: Diretorio nao encontrado.\n";
        }
    }

    // Cria arquivo com tipo especificado (Req 3.2: numérico, caractere, binário, programa)
    void touch(string name, FileType ftype = TYPE_TEXT) {
        if (currentDir->children.count(name)) {
            // Atualiza timestamp se já existe
            time(&currentDir->children[name]->modifiedAt);
            return;
        }
        // Cria arquivo com permissões 644 (rw-r--r--)
        auto newFile = make_shared<FCB>(name, ftype, currentUser, currentGroup, 6, 4, 4, currentDir);
        
        // Aloca 1 bloco inicial vazio (Req 3.4 - Alocação)
        try {
            newFile->blockIndices = disk.allocateBlocks(0); 
            currentDir->children[name] = newFile;
            cout << "Arquivo criado: " << name << " (tipo: " << fileTypeToString(ftype) << ")\n";
        } catch (exception& e) {
            cout << e.what() << endl;
        }
    }
    
    // Helper: Converte FileType para string
    string fileTypeToString(FileType t) {
        switch(t) {
            case DIRECTORY: return "DIR";
            case TYPE_TEXT: return "TEXT";
            case TYPE_NUMERIC: return "NUMERIC";
            case TYPE_BINARY: return "BINARY";
            case TYPE_PROGRAM: return "PROGRAM";
            default: return "UNKNOWN";
        }
    }

    // Escrever no arquivo (Simula: echo "conteudo" > arquivo)
    void echo(string name, string content) {
        if (!currentDir->children.count(name)) {
            touch(name); // Cria se não existe
        }
        
        auto file = currentDir->children[name];
        if (file->type == DIRECTORY) {
            cout << "Erro: Nao pode escrever em um diretorio.\n";
            return;
        }

        // Req 3.3: Checa permissão de Escrita
        if (!checkPermission(file, PERM_WRITE)) {
            cout << "Erro: Permissao negada (Write).\n";
            return;
        }

        // Req 3.4: Realocação de blocos
        // 1. Libera blocos antigos
        disk.freeBlocks(file->blockIndices);
        
        try {
            // 2. Aloca novos blocos baseados no tamanho do conteúdo
            file->blockIndices = disk.allocateBlocks(content.size());
            
            // 3. Escreve no "disco"
            disk.writeData(file->blockIndices, content);
            
            // 4. Atualiza metadados FCB
            file->size = content.size();
            time(&file->modifiedAt);
            cout << "Gravado com sucesso.\n";
        } catch (exception& e) {
            cout << e.what() << endl;
        }
    }

    // Ler arquivo (cat)
    void cat(string name) {
        if (!currentDir->children.count(name)) {
            cout << "Erro: Arquivo nao encontrado.\n";
            return;
        }
        auto file = currentDir->children[name];
        
        if (file->type == DIRECTORY) {
            cout << "Erro: E um diretorio.\n";
            return;
        }

        // Req 3.3: Checa permissão de Leitura
        if (!checkPermission(file, PERM_READ)) {
            cout << "Erro: Permissao negada (Read).\n";
            return;
        }

        // Atualiza data de acesso (Req 3.2)
        time(&file->accessedAt);

        // Req 3.4: Busca dados dos blocos
        string content = disk.readData(file->blockIndices, file->size);
        cout << content << endl;
    }

    // Helper: Converte permissão numérica (0-7) para string rwx
    string permToStr(int p) {
        string s = "";
        s += (p & PERM_READ)  ? "r" : "-";
        s += (p & PERM_WRITE) ? "w" : "-";
        s += (p & PERM_EXEC)  ? "x" : "-";
        return s;
    }

    void ls() {
        cout << left << setw(12) << "PERM" 
             << setw(10) << "TIPO" 
             << setw(8)  << "TAM" 
             << setw(8)  << "UID"
             << setw(8)  << "GID"
             << setw(18) << "MODIFICADO" 
             << "NOME" << endl;
        
        for (auto const& [key, val] : currentDir->children) {
            // Formato: drwxr-xr-x ou -rw-r--r--
            string pStr = (val->type == DIRECTORY) ? "d" : "-";
            pStr += permToStr(val->ownerPerm);
            pStr += permToStr(val->groupPerm);
            pStr += permToStr(val->otherPerm);

            cout << left << setw(12) << pStr
                 << setw(10) << fileTypeToString(val->type)
                 << setw(8)  << val->size
                 << setw(8)  << val->ownerId
                 << setw(8)  << val->groupId
                 << setw(18) << timeToString(val->modifiedAt)
                 << val->name << endl;
        }
    }

    // chmod no formato octal: 755, 644, 777, etc. (Req 3.3)
    void chmod(string name, int octalPerm) {
        if (!currentDir->children.count(name)) {
            cout << "Erro: Arquivo nao encontrado.\n";
            return;
        }
        auto file = currentDir->children[name];
        
        // Apenas o dono pode mudar permissões
        if (file->ownerId != currentUser) {
            cout << "Erro: Apenas o dono pode mudar permissoes.\n";
            return;
        }
        
        // Extrai dígitos do octal (ex: 755 -> owner=7, group=5, other=5)
        file->otherPerm = octalPerm % 10;
        file->groupPerm = (octalPerm / 10) % 10;
        file->ownerPerm = (octalPerm / 100) % 10;
        
        cout << "Permissoes alteradas para " << octalPerm << " (";
        cout << permToStr(file->ownerPerm) << permToStr(file->groupPerm) << permToStr(file->otherPerm);
        cout << ")\n";
    }

    void rm(string name) {
        if (!currentDir->children.count(name)) {
            cout << "Erro: Nao encontrado.\n";
            return;
        }
        auto target = currentDir->children[name];
        
        if (!checkPermission(target, PERM_WRITE)) {
             cout << "Erro: Permissao negada.\n";
             return;
        }

        // Libera blocos no disco (Req 3.4)
        disk.freeBlocks(target->blockIndices);
        
        // Remove da árvore
        currentDir->children.erase(name);
        cout << "Removido: " << name << endl;
    }
    
    // Renomear/Mover (mv)
    void mv(string oldName, string newName) {
        if (!currentDir->children.count(oldName)) {
            cout << "Erro: Arquivo de origem nao encontrado.\n";
            return;
        }
        if (currentDir->children.count(newName)) {
            cout << "Erro: Destino ja existe.\n";
            return;
        }

        auto file = currentDir->children[oldName];
        
        // Req 3.3: Checa permissão de escrita no diretório pai (conceitualmente)
        // ou no próprio arquivo dependendo da regra. Vamos simplificar:
        if (file->ownerId != currentUser) {
             cout << "Erro: Permissao negada (apenas dono pode mover).\n";
             return;
        }

        // Renomeia (update key in map)
        file->name = newName;
        currentDir->children[newName] = file;
        currentDir->children.erase(oldName);
        
        time(&file->modifiedAt);
        cout << "Movido/Renomeado de " << oldName << " para " << newName << endl;
    }

    // Copiar (cp)
    void cp(string srcName, string destName) {
        if (!currentDir->children.count(srcName)) {
            cout << "Erro: Arquivo de origem nao encontrado.\n";
            return;
        }
        if (currentDir->children.count(destName)) {
            cout << "Erro: Destino ja existe.\n";
            return;
        }

        auto srcFile = currentDir->children[srcName];
        if (srcFile->type == DIRECTORY) {
            cout << "Erro: cp nao suporta diretorios recursivamente nesta versao.\n";
            return;
        }

        // Lê dados originais
        string content = disk.readData(srcFile->blockIndices, srcFile->size);

        // Cria novo arquivo
        touch(destName);
        
        // Escreve dados no novo arquivo
        echo(destName, content);
        cout << "Copiado de " << srcName << " para " << destName << endl;
    }
    
    void stat(string name) {
        if (!currentDir->children.count(name)) {
            cout << "Erro: Arquivo nao encontrado.\n";
            return;
        }
        auto f = currentDir->children[name];
        
        // Formato similar ao comando stat do Linux
        cout << "  File: " << f->name << "\n";
        cout << "  Size: " << f->size << " bytes\n";
        cout << " Inode: " << f->inodeId << "\n";
        cout << "  Type: " << fileTypeToString(f->type) << "\n";
        cout << "Blocks: [";
        for(size_t i = 0; i < f->blockIndices.size(); i++) {
            cout << f->blockIndices[i];
            if (i < f->blockIndices.size() - 1) cout << ", ";
        }
        cout << "]\n";
        cout << "Access: (" << f->ownerPerm << f->groupPerm << f->otherPerm << "/";
        cout << permToStr(f->ownerPerm) << permToStr(f->groupPerm) << permToStr(f->otherPerm) << ")\n";
        cout << "   Uid: " << f->ownerId << "  Gid: " << f->groupId << "\n";
        cout << "Access: " << timeToString(f->accessedAt) << "\n";
        cout << "Modify: " << timeToString(f->modifiedAt) << "\n";
        cout << " Birth: " << timeToString(f->createdAt) << "\n";
    }
    
    // Simula troca de usuário e grupo (Req 3.3: testar owner/group/others)
    void switchUser(int uid, int gid = -1) {
        currentUser = uid;
        if (gid >= 0) currentGroup = gid;
        cout << "Usuario alterado para UID: " << currentUser << ", GID: " << currentGroup << endl;
    }
    
    // Retorna info do usuário atual
    void whoami() {
        cout << "UID: " << currentUser << ", GID: " << currentGroup << endl;
    }
    
    string getPath() {
         // Simplificado: não reconstrói path inteiro, mostra apenas atual
         if (currentDir == root) return "/";
         return ".../" + currentDir->name;
    }
};

// ==========================================
// PROGRAMA PRINCIPAL (CLI)
// ==========================================
void printHelp() {
    cout << "\n=== COMANDOS DISPONIVEIS ===\n";
    cout << "  mkdir <nome>            - Cria diretorio\n";
    cout << "  cd <nome|..|/>          - Navega entre diretorios\n";
    cout << "  ls                      - Lista arquivos com metadados\n";
    cout << "  touch <nome> [tipo]     - Cria arquivo (tipo: text/num/bin/prog)\n";
    cout << "  echo <arq> <conteudo>   - Escreve conteudo no arquivo\n";
    cout << "  cat <arq>               - Le conteudo do arquivo\n";
    cout << "  cp <origem> <destino>   - Copia arquivo\n";
    cout << "  mv <origem> <destino>   - Move/renomeia arquivo\n";
    cout << "  rm <nome>               - Remove arquivo ou diretorio\n";
    cout << "  chmod <arq> <perm>      - Altera permissoes (ex: 755, 644)\n";
    cout << "  stat <arq>              - Mostra metadados detalhados (inode, blocos)\n";
    cout << "  su <uid> [gid]          - Troca usuario/grupo atual\n";
    cout << "  whoami                  - Mostra usuario/grupo atual\n";
    cout << "  help                    - Mostra esta ajuda\n";
    cout << "  exit                    - Sai do simulador\n\n";
}

int main() {
    FileSystem fs;
    string command, arg1, arg2;
    string line;

    cout << "=== Mini Sistema de Arquivos em Memoria (Simulador) ===\n";
    cout << "Trabalho M3 - Sistemas Operacionais - UNIVALI\n";
    cout << "Digite 'help' para ver os comandos disponiveis.\n\n";

    while (true) {
        cout << "user@" << fs.getPath() << "$ ";
        if (!getline(cin, line)) break;
        
        stringstream ss(line);
        ss >> command;

        if (command == "exit") break;
        else if (command == "help") printHelp();
        else if (command == "ls") fs.ls();
        else if (command == "whoami") fs.whoami();
        else if (command == "mkdir") {
            ss >> arg1;
            if (!arg1.empty()) fs.mkdir(arg1);
        }
        else if (command == "cd") {
            ss >> arg1;
            if (!arg1.empty()) fs.cd(arg1);
        }
        else if (command == "touch") {
            ss >> arg1;
            string typeStr;
            ss >> typeStr;
            
            FileType ftype = TYPE_TEXT; // Padrão
            if (typeStr == "num" || typeStr == "numeric") ftype = TYPE_NUMERIC;
            else if (typeStr == "bin" || typeStr == "binary") ftype = TYPE_BINARY;
            else if (typeStr == "prog" || typeStr == "program") ftype = TYPE_PROGRAM;
            
            if (!arg1.empty()) fs.touch(arg1, ftype);
        }
        else if (command == "cat") {
            ss >> arg1;
            if (!arg1.empty()) fs.cat(arg1);
        }
        else if (command == "rm") {
            ss >> arg1;
            if (!arg1.empty()) fs.rm(arg1);
        }
        else if (command == "mv") {
            ss >> arg1 >> arg2;
            if (!arg1.empty() && !arg2.empty()) fs.mv(arg1, arg2);
        }
        else if (command == "cp") {
            ss >> arg1 >> arg2;
            if (!arg1.empty() && !arg2.empty()) fs.cp(arg1, arg2);
        }
        else if (command == "stat") {
            ss >> arg1;
            if (!arg1.empty()) fs.stat(arg1);
        }
        else if (command == "echo") {
            ss >> arg1; // arquivo
            
            // Pega o resto da linha como conteudo
            string content;
            getline(ss, content);
            
            // Remove espaços iniciais
            size_t first = content.find_first_not_of(' ');
            if (string::npos != first) content = content.substr(first);
            
            if (!arg1.empty()) fs.echo(arg1, content);
        }
        else if (command == "chmod") {
            int perm;
            ss >> arg1 >> perm;
            if (!arg1.empty()) fs.chmod(arg1, perm);
        }
        else if (command == "su") {
            int uid, gid = -1;
            ss >> uid;
            ss >> gid; // Opcional
            fs.switchUser(uid, gid);
        }
        else {
            if (!command.empty()) cout << "Comando desconhecido. Digite 'help'.\n";
        }
    }

    return 0;
}

