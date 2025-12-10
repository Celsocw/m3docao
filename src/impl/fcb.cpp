#include "../header/bloco_controle.h"

int nextInodeId = 1;

FCB::FCB(string n, FileType t, int uid, int gid, int oPerm, int gPerm, int pubPerm, shared_ptr<FCB> par) 
    : nome(n), tipo(t), tamanho(0), idProprietario(uid), idGrupo(gid),
      permProprietario(oPerm), permGrupo(gPerm), permOutros(pubPerm), pai(par) {
    inodeId = nextInodeId++;
    time(&criadoEm);
    modificadoEm = criadoEm;
    acessadoEm = criadoEm;
}

#include "../header/bloco_controle.h"

// Global inode counter definition
int nextInodeId = 1;

// FCB Constructor implementation
FCB::FCB(string n, FileType t, int uid, int gid, int oPerm, int gPerm, int pubPerm, shared_ptr<FCB> par) 
    : nome(n), tipo(t), tamanho(0), idProprietario(uid), idGrupo(gid),
      permProprietario(oPerm), permGrupo(gPerm), permOutros(pubPerm), pai(par) {
    inodeId = nextInodeId++;
    time(&criadoEm);
    modificadoEm = criadoEm;
    acessadoEm = criadoEm;
}
