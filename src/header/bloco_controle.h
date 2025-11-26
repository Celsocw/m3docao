#ifndef BLOCO_CONTROLE_H
#define BLOCO_CONTROLE_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <ctime>

using namespace std;

// ==========================================
// FILE TYPES (Req 3.2)
// ==========================================
enum FileType { DIRECTORY, TYPE_TEXT, TYPE_NUMERIC, TYPE_BINARY, TYPE_PROGRAM };

// ==========================================
// 3.2: FILE CONTROL BLOCK (FCB / Inode)
// ==========================================
struct FCB {
    int inodeId;          // ID único (Req 3.2: simula inode)
    string nome;
    FileType tipo;
    int tamanho;
    int idProprietario;
    int idGrupo;          // Req 3.3: para permissões de grupo
    
    // Permissões no formato octal (Req 3.3: owner/group/others)
    // Ex: 755 = rwxr-xr-x, 644 = rw-r--r--
    int permProprietario;        // 0-7 (rwx)
    int permGrupo;        // 0-7 (rwx)
    int permOutros;        // 0-7 (rwx)
    
    time_t criadoEm;
    time_t modificadoEm;
    time_t acessadoEm;    // Req 3.2: data de acesso
    
    // Simulação de Inode: Lista de índices de blocos onde o conteúdo vive
    vector<int> indicesBlocos; 

    // Para diretórios: mantemos referências aos filhos em memória
    // (Em um FS real, isso estaria dentro do bloco de dados, 
    // mas para o trabalho M3, ponteiros facilitam a estrutura de árvore do Req 3.1)
    map<string, shared_ptr<FCB>> filhos;
    weak_ptr<FCB> pai; // Para 'cd ..'

    FCB(string n, FileType t, int uid, int gid, int oPerm, int gPerm, int pubPerm, shared_ptr<FCB> par);
};

// Global inode counter
extern int nextInodeId;

#endif // BLOCO_CONTROLE_H
