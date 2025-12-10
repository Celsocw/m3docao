
#include "../header/cliente.h"
#include <iostream>

using namespace std;

void printHelp() {
    cout << "\n=== COMANDOS DISPONIVEIS ===\n";
    cout << "  mkdir <nome>            - Cria diretorio\n";
    cout << "  cd <nome|..|/>          - Navega entre diretorios (req 3.1/3.3)\n";
    cout << "  ls                      - Lista arquivos com metadados (req 3.1/3.3)\n";
    cout << "  touch <nome> [tipo]     - Cria arquivo (tipo: text/num/bin/prog) (req 3.2)\n";
    cout << "  echo <arq> <conteudo>   - Escreve conteudo no arquivo (req 3.2/3.4/3.3)\n";
    cout << "  cat <arq>               - Le conteudo do arquivo (req 3.2/3.3/3.4)\n";
    cout << "  cp <origem> <destino>   - Copia arquivo ou diretorio (req 3.1/3.2/3.3/3.4)\n";
    cout << "  mv <origem> <destino>   - Move/renomeia arquivo (req 3.3)\n";
    cout << "  rm <nome>               - Remove arquivo ou diretorio (req 3.3)\n";
    cout << "  chmod <arq> <perm>      - Altera permissoes (ex: 755, 644) (req 3.3)\n";
    cout << "  stat <arq>              - Mostra metadados detalhados (inode, blocos) (req 3.2/3.4)\n";
    cout << "  exec <arq>              - Executa arquivo (requer permissao x) (req 3.3)\n";
    cout << "  su <uid> [gid]          - Troca usuario/grupo atual (req 3.3)\n";
    cout << "  whoami                  - Mostra usuario/grupo atual (req 3.3)\n";
    cout << "  help                    - Mostra esta ajuda\n";
    cout << "  exit                    - Sai do simulador\n\n";
}
