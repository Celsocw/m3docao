#ifndef DISCO_VIRTUAL_H
#define DISCO_VIRTUAL_H

#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include "constantes.h"

using namespace std;

// ==========================================
// 3.4: SIMULAÇÃO DE ALOCAÇÃO DE BLOCOS
// ==========================================
class VirtualDisk {
private:
    // O "Disco" é um array linear de bytes na memória
    vector<char> dados;
    // Mapa de bits para saber quais blocos estão livres (true = ocupado)
    vector<bool> mapaBits;

public:
    VirtualDisk() {
        dados.resize(DISK_SIZE_BLOCKS * BLOCK_SIZE, '\0');
        mapaBits.resize(DISK_SIZE_BLOCKS, false);
    }

    // Retorna índice de blocos livres
    vector<int> alocarBlocos(int bytesRequeridos) {
        int blocosNecessarios = (int)ceil((double)bytesRequeridos / BLOCK_SIZE);
        if (blocosNecessarios == 0) blocosNecessarios = 1; // Mínimo 1 bloco

        vector<int> indices;
        int encontrados = 0;

        // Estratégia: Alocação indexada (procura quaisquer blocos livres)
        for (int i = 0; i < DISK_SIZE_BLOCKS && encontrados < blocosNecessarios; i++) {
            if (!mapaBits[i]) {
                indices.push_back(i);
                mapaBits[i] = true; // Marca como ocupado
                encontrados++;
            }
        }

        if (encontrados < blocosNecessarios) {
            // Rollback se não houver espaço suficiente
            for (int idx : indices) mapaBits[idx] = false;
            throw runtime_error("Erro: Espaco insuficiente no disco virtual.");
        }
        return indices;
    }

    void liberarBlocos(const vector<int>& indices) {
        for (int idx : indices) {
            if (idx >= 0 && idx < DISK_SIZE_BLOCKS) {
                mapaBits[idx] = false;
                // Opcional: Limpar dados
                fill(dados.begin() + (idx * BLOCK_SIZE), 
                     dados.begin() + ((idx + 1) * BLOCK_SIZE), '\0');
            }
        }
    }

    // Escreve dados nos blocos alocados
    void escreverDados(const vector<int>& indices, const string& conteudo) {
        size_t posConteudo = 0;
        for (int idx : indices) {
            int enderecoInicio = idx * BLOCK_SIZE;
            for (int i = 0; i < BLOCK_SIZE && posConteudo < conteudo.size(); i++) {
                dados[enderecoInicio + i] = conteudo[posConteudo++];
            }
        }
    }

    // Lê dados dos blocos
    string lerDados(const vector<int>& indices, int tamanhoBytes) {
        string conteudo;
        int bytesLidos = 0;
        for (int idx : indices) {
            int enderecoInicio = idx * BLOCK_SIZE;
            for (int i = 0; i < BLOCK_SIZE && bytesLidos < tamanhoBytes; i++) {
                if (dados[enderecoInicio + i] != '\0') {
                    conteudo += dados[enderecoInicio + i];
                }
                bytesLidos++;
            }
        }
        return conteudo;
    }
};

#endif // DISCO_VIRTUAL_H
