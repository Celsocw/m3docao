#ifndef SISTEMA_ARQUIVOS_H
#define SISTEMA_ARQUIVOS_H

#include <memory>
#include <string>
#include "disco_virtual.h"
#include "bloco_controle.h"
#include "constantes.h"

using namespace std;

// ==========================================
// SISTEMA DE ARQUIVOS (Lógica Principal)
// ==========================================
class FileSystem {
private:
    VirtualDisk disco;
    shared_ptr<FCB> raiz;
    shared_ptr<FCB> diretorioAtual;
    int usuarioAtual;  // ID do usuário atual logado
    int grupoAtual; // ID do grupo atual

    // Helper: Verifica permissão (Req 3.3 - owner/group/others)
    bool verificarPermissao(shared_ptr<FCB> arquivo, int permRequerida);
    
    // Helper: Converte FileType para string
    string tipoArquivoString(FileType t);
    
    // Helper: Converte permissão numérica (0-7) para string rwx
    string permParaStr(int p);
    
    // Utilitário para formatar tempo
    string tempoParaString(time_t t);
    
    // Helper: Remove recursivamente um FCB e seus filhos
    void removerRecursivo(shared_ptr<FCB> alvo);

public:
    FileSystem();

    // --- Comandos (Req 3.1 e 3.2) ---
    void mkdir(string nome);
    void cd(string nome);
    void touch(string nome, FileType tipo = TYPE_TEXT);
    void echo(string nome, string conteudo);
    void cat(string nome);
    void ls();
    void chmod(string nome, int permOctal);
    void rm(string nome, bool recursivo = false);
    void mv(string nomeAntigo, string nomeNovo);
    void cp(string nomeOrigem, string nomeDestino);
    void stat(string nome);
    void trocarUsuario(int uid, int gid = -1);
    void quemSou();
    string obterCaminho();
};

#endif // SISTEMA_ARQUIVOS_H
