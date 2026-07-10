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
#define MAXOPENFILES 10

unsigned short fat[FATCLUSTERS];

typedef struct {
  char used;
  char name[25];
  unsigned short first_block;
  int size;
} dir_entry;

dir_entry dir[DIRENTRIES];

typedef struct {
  int used;           // used = 1 ou 0 (nao usado) 
  int dir_index;       
  int mode;           //para FS_W = 1, para FS_R = 0
  int current_cluster;  
  int cluster_offset;
} open_file_entry;

open_file_entry openfiles[MAXOPENFILES];


int fs_init() {
  printf("Função não implementada: fs_init\n");
  return 1;
}

int fs_format() {
  printf("Função não implementada: fs_format\n");
  return 0;
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

  //tenta inicialiazar o dir
  if(!fs_init()){
    return 0;
  }

  //verifica tamanho do nome do arquivo 
  if(strlen(file_name)>24){
    printf("O nome do arquivo é muito grande (maior que 24 caracteres).\n");
    return 0;
  }

  //verificar se o arq já existe e se o dir está cheio
  int livre = -1;
  int bloco_livre = -1;
  for (int i = 0; i < DIRENTRIES; i++)
  {
    if (dir[i].used == 1){
      if(strcmp(dir[i].name, file_name) == 0){//verifico se esta usado pq o nome pode ser lixo de memoria
        printf("Já existe um arquivo com este nome. :/\n");
        return 0;
      }
    }else if(livre == -1){
      livre = i;
    }
    
  }
  
  if (livre == -1){
    printf("Diretório cheio.");
    return 0;
  }

  //verificar fat
  for (int i = 33; i < bl_size(); i++)
  {
    if (fat[i] == 1){
      bloco_livre = i;
      break;
    }
  }
  //verificação extra pq nao faz mal
  if (bloco_livre == -1){
    printf("Sem espaço livre no disco.\n");
    return 0;
  }
  
  //se der tudo certo, crio um arquivo no bl livre
  dir_entry new_archive;
    //inicializar
  memset(&new_archive, 0, sizeof(dir_entry));
    //add dados
  new_archive.first_block = bloco_livre;
  strcpy(new_archive.name, file_name);
  new_archive.size = 0;
  new_archive.used = 1;

  //grava no dir e fat
  dir[livre] = new_archive;
  fat[bloco_livre] = 2;

  //gravar fat e dir
  char *buffer_Fat = (char *) fat;
  char *buffer_Dir = (char*) dir;

  for (int sector = 0; sector < 32; sector++)
  {
    if (!bl_write(sector, buffer_Fat + (sector * CLUSTERSIZE))){
      printf("Não foi possível guardar na FAT.");
      return 0;
    }
  }

  if (!bl_write(32, buffer_Dir)){
    printf("Não foi possível guardar no diretório.");
    return 0;
  }

  return 1;
}

int fs_remove(char *file_name) {
  printf("Função não implementada: fs_remove\n");
  return 0;
}

int fs_open(char *file_name, int mode) {
  
  //verificar init do dir
  if(!fs_init()){
    return -1;
  }

  //verificar se o modo eh valido
  if (mode != FS_R && mode != FS_W)
  {
    printf("Modo inválido de abertura.\n");
    return -1;
  }

  //verificar se há espaço para abrir o arquivo
  int livre = -1;
  for(int i = 0; i < MAXOPENFILES; i++){
    if(openfiles[i].used == 0){
      livre = i;
      break;

    }
  }

  if(livre == -1){
    printf("Não há espaço para abrir novos arquivos.\n");
    return -1;
  }

  if(mode == FS_R){//se for FS_R

    //procurar arquivo
    for (int i = 0; i < DIRENTRIES; i++)
    {
      if(dir[i].used == 1 && strcmp(dir[i].name, file_name) == 0){
        openfiles[livre].used = 1;
        openfiles[livre].dir_index = i;
        openfiles[livre].mode = FS_R;
        openfiles[livre].current_cluster = dir[i].first_block;
        openfiles[livre].cluster_offset = 0;
        return livre;
      }
    }
    //percorreu tudo e n achou
    printf("Arquivo não encontrado.\n");
    return -1;
    
  }else if (mode == FS_W){//se for FS_W
    
    //procurar arquivo
    for (int i = 0; i < DIRENTRIES; i++){
      if(dir[i].used == 1 && strcmp(dir[i].name, file_name) == 0){//se achar
        if(!fs_remove(file_name))return -1;//remover
        break;
      }
    }
    if(!fs_create(file_name))return -1;//criar novo
    
    //procurar o novo arquivo
    for (int i = 0; i < DIRENTRIES; i++)
    {
      if(dir[i].used == 1 && strcmp(dir[i].name, file_name) == 0){
        openfiles[livre].used = 1;
        openfiles[livre].dir_index = i;
        openfiles[livre].mode = FS_W;
        openfiles[livre].current_cluster = dir[i].first_block;
        openfiles[livre].cluster_offset = 0;
        return livre;

      }
    }

    //percorreu tudo e n achou
    printf("Arquivo não encontrado.\n");
    return -1;
  }

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

