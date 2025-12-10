
#include <iostream>
#include <sstream>
#include "../header/constantes.h"
#include "../header/disco_virtual.h"
#include "../header/bloco_controle.h"
#include "../header/sistema_arquivos.h"
#include "../header/cliente.h"

using namespace std;


// ==========================================
// PROGRAMA PRINCIPAL (CLI)
// ==========================================

int main() {
    FileSystem fs;
    string comando, arg1, arg2;
    string linha;

    cout << "=== Mini Sistema de Arquivos em Memoria (Simulador) ===\n";
    cout << "Trabalho M3 - Sistemas Operacionais - UNIVALI\n";
    cout << "Digite 'help' para ver os comandos disponiveis.\n\n";

    while (true) {
        cout << "user@" << fs.obterCaminho() << "$ ";
        if (!getline(cin, linha)) break;
        
        stringstream ss(linha);
        ss >> comando;

        if (comando == "exit") break;
        else if (comando == "help") printHelp();
        else if (comando == "ls") fs.ls();
        else if (comando == "whoami") fs.quemSou();
        else if (comando == "mkdir") {
            ss >> arg1;
            if (!arg1.empty()) fs.mkdir(arg1);
        }
        else if (comando == "cd") {
            ss >> arg1;
            if (!arg1.empty()) fs.cd(arg1);
        }
        else if (comando == "touch") {
            ss >> arg1;
            string tipoStr;
            ss >> tipoStr;
            
            FileType tipo = TYPE_TEXT; // Padrão
            if (tipoStr == "num" || tipoStr == "numeric") tipo = TYPE_NUMERIC;
            else if (tipoStr == "bin" || tipoStr == "binary") tipo = TYPE_BINARY;
            else if (tipoStr == "prog" || tipoStr == "program") tipo = TYPE_PROGRAM;
            
            if (!arg1.empty()) fs.touch(arg1, tipo);
        }
        else if (comando == "cat") {
            ss >> arg1;
            if (!arg1.empty()) fs.cat(arg1);
        }
        else if (comando == "rm") {
            ss >> arg1;
            bool recursivo = false;
            if (arg1 == "-r" || arg1 == "-rf") {
                recursivo = true;
                ss >> arg1; // Pega o nome real do arquivo/diretório
            }
            if (!arg1.empty()) fs.rm(arg1, recursivo);
        }
        else if (comando == "mv") {
            ss >> arg1 >> arg2;
            if (!arg1.empty() && !arg2.empty()) fs.mv(arg1, arg2);
        }
        else if (comando == "cp") {
            ss >> arg1 >> arg2;
            if (!arg1.empty() && !arg2.empty()) fs.cp(arg1, arg2);
        }
        else if (comando == "stat") {
            ss >> arg1;
            if (!arg1.empty()) fs.stat(arg1);
        }
        else if (comando == "echo") {
            ss >> arg1; // arquivo
            
            // Pega o resto da linha como conteudo
            string conteudo;
            getline(ss, conteudo);
            
            // Remove espaços iniciais
            size_t first = conteudo.find_first_not_of(' ');
            if (string::npos != first) conteudo = conteudo.substr(first);
            
            if (!arg1.empty()) fs.echo(arg1, conteudo);
        }
        else if (comando == "chmod") {
            int perm;
            ss >> arg1 >> perm;
            if (!arg1.empty()) fs.chmod(arg1, perm);
        }
        else if (comando == "su") {
            int uid, gid = -1;
            ss >> uid;
            ss >> gid; // Opcional
            fs.trocarUsuario(uid, gid);
        }
        else {
            if (!comando.empty()) cout << "Comando desconhecido. Digite 'help'.\n";
        }
    }

    return 0;
}

