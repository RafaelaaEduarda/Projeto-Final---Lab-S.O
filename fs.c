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
  char *buffer_Fat = (char *) fat;
  char *buffer_Dir = (char *) dir;

  for (sector = 0; sector < 32; sector++) // leitura de cada setor no disco
  {
    if (!bl_read(sector, buffer_Fat + (sector * CLUSTERSIZE)))
      return 0;    
  }

  if (!bl_read(32, buffer_Dir))
    return 0;

  if (fat[0] != 3 || fat[32] != 4)
    printf("Disco não formatado\n");
  
  return 1;  
}

int fs_format() {
  int sector;  
  char *buffer_Fat = (char *) fat;
  char *buffer_Dir = (char *) dir;
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

  for (sector = 0; sector < 32; sector++){ 
    if (!bl_write(sector, buffer_Fat + (sector * CLUSTERSIZE)))
      return 0;
  }

  if (!bl_write(32, buffer_Dir))
    return 0;  

  return 1;
}

int fs_free() {
  printf("Função não implementada: fs_free\n");
  return 0;
}

int fs_list(char *buffer, int size) {
  printf("Função não implementada: fs_list\n");
  return 0;
}

int fs_create(char* file_name) {
  printf("Função não implementada: fs_create\n");
  return 0;
}

int fs_remove(char *file_name) {
  printf("Função não implementada: fs_remove\n");
  return 0;
}

int fs_open(char *file_name, int mode) {
  printf("Função não implementada: fs_open\n");
  return -1;
}

int fs_close(int file)  {
  printf("Função não implementada: fs_close\n");
  return 0;
}

int fs_write(char *buffer, int size, int file) {
  printf("Função não implementada: fs_write\n");
  return -1;
}

int fs_read(char *buffer, int size, int file) {
  printf("Função não implementada: fs_read\n");
  return -1;
}

