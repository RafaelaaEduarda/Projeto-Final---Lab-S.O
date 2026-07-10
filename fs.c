/*
 * RSFS - Really Simple File System
 *
 * Copyright © 2010,2012,2019 Gustavo Maciel Dias Vieira
 * Copyright © 2010 Rodrigo Rocco Barbieri
 *
 * This file is part of RSFS.
 *
 * RSFS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define CLUSTERSIZE 4096
#define FATCLUSTERS 65536
#define DIRENTRIES 128

unsigned short fat[FATCLUSTERS];

typedef struct {
  char used;
  char name[25];
  unsigned short first_block;
  int size;
} dir_entry;

dir_entry dir[DIRENTRIES];

int fs_init() {
  int sector;
  char* buffer_Fat = (char*)fat;
  char* buffer_Dir = (char*)dir;

  for (sector = 0; sector < 32; sector++) // leitura de cada setor no disco
  {
    if (!bl_read(sector, buffer_Fat + (sector * CLUSTERSIZE)))
      return 0;
  }

  if (!bl_read(32, buffer_Dir))
    return 0;

  if (fat[0] != 3 || fat[32] != 4) {
    printf("Disco não formatado\n");
  }

  return 1;
}

int fs_format() {
  int sector;
  char* buffer_Fat = (char*)fat;
  char* buffer_Dir = (char*)dir;
  int tamBlocos = bl_size();

  if (tamBlocos < 33) // 33 = 32 (FAT) + 1 (DIR), tam min
    return 0;

  for (sector = 0; sector < 32; sector++) // blocos de 0 a 31 são FAT = 3
    fat[sector] = 3;

  fat[32] = 4; // bloco 32 é do diretorio = 4

  for (sector = 33; sector < tamBlocos; sector++)
    fat[sector] = 1;

  for (sector = tamBlocos; sector < FATCLUSTERS; sector++)
    fat[sector] = 0;

  for (sector = 0; sector < DIRENTRIES; sector++)
    dir[sector].used = 0;

  for (sector = 0; sector < 32; sector++) {
    if (!bl_write(sector, buffer_Fat + (sector * CLUSTERSIZE)))
      return 0;
  }

  if (!bl_write(32, buffer_Dir))
    return 0;

  return 1;
}

int fs_free() {
  int i, count;

  if (fat[0] != 3 || fat[32] != 4) {
    printf("Disco não formatado\n");
    return 0;
  }

  count = 0;

  for (i = 0; i < FATCLUSTERS; i++) {
    if (fat[i] == 1)
      count++;
  }

  return count * CLUSTERSIZE;
}

int fs_list(char* buffer, int size) {

  int i, j;
  int offset = 0;
  char tmp[32];

  if (fat[0] != 3 || fat[32] != 4) {
    printf("Disco não formatado\n");
    return 0;
  }

  for (i = 0; i < DIRENTRIES; i++) {
    if (dir[i].used != 0) {
      for (j = 0; j < 24 && dir[i].name[j] != '\0'; j++) {

        if (offset + 30 >= size)
          return 0;

        buffer[offset++] = dir[i].name[j];
      }

      if (offset + 2 >= size)
        return 0;

      buffer[offset++] = '\t';
      buffer[offset++] = '\t';

      int printed = snprintf(tmp, sizeof(tmp), "%d", dir[i].size);

      if (printed < 0)
        return 0;

      if (offset + printed + 1 >= size)
        return 0;

      for (j = 0; j < printed; j++)
        buffer[offset++] = tmp[j];

      buffer[offset++] = '\n';
    }
  }

  if (offset < size)
    buffer[offset] = '\0';
  else
    buffer[size - 1] = '\0';

  return 1;
}

int fs_create(char* file_name) {
  // verifica se o disco está formatado antes de fazer as operações
  if (fat[0] != 3 || fat[32] != 4) {
    printf("Disco não formatado\n");
    return 0;
  }

  // verifica tamanho do nome do arquivo
  if (strlen(file_name) > 24) {
    printf("O nome do arquivo é muito grande (maior que 24 caracteres).\n");
    return 0;
  }

  // verificar se o arq já existe e se o dir está cheio
  int livre = -1;
  int bloco_livre = -1;
  for (int i = 0; i < DIRENTRIES; i++) {
    if (dir[i].used == 1) {
      if (strcmp(dir[i].name, file_name) == 0) { // verifico se esta usado pq o nome pode ser lixo de memoria
        printf("Já existe um arquivo com este nome. :/\n");
        return 0;
      }
    } else if (livre == -1) {
      livre = i;
    }
  }

  if (livre == -1) {
    printf("Diretório cheio.");
    return 0;
  }

  // verificar fat
  for (int i = 33; i < bl_size(); i++) {
    if (fat[i] == 1) {
      bloco_livre = i;
      break;
    }
  }
  // verificação extra pq nao faz mal
  if (bloco_livre == -1) {
    printf("Sem espaço livre no disco.\n");
    return 0;
  }

  // se der tudo certo, crio um arquivo no bl livre
  dir_entry new_archive;
  // inicializar
  memset(&new_archive, 0, sizeof(dir_entry));
  // add dados
  new_archive.first_block = bloco_livre;
  strcpy(new_archive.name, file_name);
  new_archive.size = 0;
  new_archive.used = 1;

  // grava no dir e fat
  dir[livre] = new_archive;
  fat[bloco_livre] = 2;

  // gravar fat e dir
  char* buffer_Fat = (char*)fat;
  char* buffer_Dir = (char*)dir;

  for (int sector = 0; sector < 32; sector++) {
    if (!bl_write(sector, buffer_Fat + (sector * CLUSTERSIZE))) {
      printf("Não foi possível guardar na FAT.");
      return 0;
    }
  }

  if (!bl_write(32, buffer_Dir)) {
    printf("Não foi possível guardar no diretório.");
    return 0;
  }

  return 1;
}

int fs_remove(char* file_name) {

  if (fat[0] != 3 || fat[32] != 4) {
    printf("disco não formatado\n");
    return 0;
  }

  for (int i = 0; i < DIRENTRIES; i++) {
    if (dir[i].used && strcmp(dir[i].name, file_name) == 0) { // Verifica se o arquivo esta sendo usado e se é o que quero remover
      // Começo a remoção me oriantando a partir do primeiro bloco
      unsigned short current_block = dir[i].first_block;

      while (current_block >= 33 && current_block < bl_size() && current_block != 2) { // Ate chegar no ultimo bloco
        unsigned short next = fat[current_block];
        fat[current_block] = 1; // Libero o bloco
        current_block = next;
      }

      // Limpo os dados antigos
      dir[i].used = 0;
      dir[i].name[0] = '\0';
      dir[i].size = 0;
      dir[i].first_block = 0;

      // Grava de volta no disco
      for (int s = 0; s < 32; s++) {
        bl_write(s, (char*)&fat[s * (SECTORSIZE / sizeof(unsigned short))]);
      }
      bl_write(32, (char*)dir);

      return 1;
    }
  }
  printf("Erro: Arquivo '%s' não encontrado.\n", file_name);
  return 0;
}

int fs_open(char* file_name, int mode) {
  printf("Função não implementada: fs_open\n");
  return -1;
}

int fs_close(int file) {
  printf("Função não implementada: fs_close\n");
  return 0;
}

int fs_write(char* buffer, int size, int file) {
  printf("Função não implementada: fs_write\n");
  return -1;
}

int fs_read(char* buffer, int size, int file) {
  if (fat[0] != 3 || fat[32] != 4) {
    printf("disco não formatado\n");
    return -1;
  }
}
