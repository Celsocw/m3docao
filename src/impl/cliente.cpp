
#include "../header/cliente.h"
#include <iostream>

using namespace std;

void printHelp() {
    cout << "\n=== COMANDOS DISPONIVEIS ===\n";
    cout << "  mkdir <nome>            - Cria diretorio\n";
    cout << "  cd <nome|..|/>          - Navega entre diretorios\n";
    cout << "  ls                      - Lista arquivos com metadados\n";
    cout << "  touch <nome> [tipo]     - Cria arquivo (tipo: text/num/bin/prog)\n";
    cout << "  echo <arq> <conteudo>   - Escreve conteudo no arquivo\n";
    cout << "  cat <arq>               - Le conteudo do arquivo\n";
    cout << "  cp <origem> <destino>   - Copia arquivo ou diretorio\n";
    cout << "  mv <origem> <destino>   - Move/renomeia arquivo\n";
    cout << "  rm <nome>               - Remove arquivo ou diretorio\n";
    cout << "  chmod <arq> <perm>      - Altera permissoes (ex: 755, 644)\n";
    cout << "  stat <arq>              - Mostra metadados detalhados (inode, blocos)\n";
    cout << "  exec <arq>              - Executa arquivo (requer permissao x)\n";
    cout << "  su <uid> [gid]          - Troca usuario/grupo atual\n";
    cout << "  whoami                  - Mostra usuario/grupo atual\n";
    cout << "  help                    - Mostra esta ajuda\n";
    cout << "  exit                    - Sai do simulador\n\n";
}
