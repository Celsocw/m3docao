#ifndef CONSTANTS_H
#define CONSTANTS_H

// ==========================================
// CONSTANTES E DEFINIÇÕES (Req 3.3 e 3.4)
// ==========================================

// Block size and disk configuration
const int BLOCK_SIZE = 64;         // Tamanho pequeno para demonstrar alocação de múltiplos blocos
const int DISK_SIZE_BLOCKS = 100;  // Disco simula 100 blocos

// Permission masks (RWX) - Req 3.3
const int PERM_READ  = 4;  // 100 (binary)
const int PERM_WRITE = 2;  // 010 (binary)
const int PERM_EXEC  = 1;  // 001 (binary)

#endif // CONSTANTS_H
  
