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
  int used;            
  int dir_index;       
  int mode;             
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
  for(int i = 0; i < DIRENTRIES; i++){
    if (dir[i].used && strcmp(dir[i].name, file_name) == 0){// Verifica se o arquivo esta sendo usado e se é o que quero remover
        // Começo a remoção me oriantando a partir do primeiro bloco
        unsigned short current_block = dir[i].first_block;

        while(current_block != 2){// Ate chegar no ultimo bloco
          unsigned short next = fat[current_block];
          fat[current_block] = 1; // Libero o bloco
          current_block = next;
        }
        fat[current_block] = 1;

        // Limpo os dados antigos
        dir[i].used = 0;
        dir[i].name[0] = '\0';
        dir[i].size = 0;
        dir[i].first_block = 0;
      
        // Grava de volta no disco
        for (int s = 0; s < 32; s++) {
          bl_write(s, (char *)&fat[s * (SECTORSIZE / sizeof(unsigned short))]);
        }
        bl_write(32, (char *)dir);

        return 1;
    }
  }
  printf("Erro: Arquivo '%s' não encontrado.\n", file_name);
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

int fs_close(int file) {
  //  Verifica se o número do arquivo existe e se ele está realmente aberto
  if (file < 0 || file >= MAXOPENFILES || openfiles[file].used == 0) {
    printf("Erro: identificador de arquivo invalido ou arquivo ja esta fechado (%d)\n", file); 
    return 0; // 0 significa ERRO
  }
  openfiles[file].used = 0;

  return 1;
}

int fs_write(char *buffer, int size, int file) {
  int i;
  int escrito = 0;
  char buf_temp[4096];

  // Verifica se o arquivo é válido e está aberto para escrita
  if (file < 0 || file > 9 || openfiles[file].used == 0) {
    printf("Erro: arquivo fechado ou invalido\n");
    return -1;
  }
  if (openfiles[file].mode != 1) { // 1 = FS_W
    printf("Erro: arquivo nao ta aberto para escrita\n");
    return -1;
  }
  if (size <= 0) {
    return 0;
  }

  int id_dir = openfiles[file].dir_index;

  // Loop para escrever os dados aos poucos
  while (escrito < size) {
    
    // Se o arquivo é novo ou o bloco de 4096 bytes já encheu
    if (openfiles[file].current_cluster == -1 || openfiles[file].cluster_offset == 4096) {
      
      // Procura um bloco livre na FAT
      int novo_bloco = -1;
      for (i = 33; i < 65536; i++) {
        if (fat[i] == 1) {
          novo_bloco = i;
          break;
        }
      }

      if (novo_bloco == -1) {
        printf("Erro: disco cheio!\n");
        break; // Sai do loop se não tiver espaço
      }

      // Atualiza a FAT
      if (openfiles[file].current_cluster == -1) {
        dir[id_dir].first_block = novo_bloco; // Primeiro bloco do arquivo
      } else {
        fat[openfiles[file].current_cluster] = novo_bloco; // Liga o bloco velho ao novo
      }
      
      fat[novo_bloco] = 2;
      
      // Atualiza o arquivo aberto
      openfiles[file].current_cluster = novo_bloco;
      openfiles[file].cluster_offset = 0;
    }

    // Le o que já está no disco para o buffer temporario para não apagar coisas sem querer
    bl_read(openfiles[file].current_cluster, buf_temp);

    // Calcula quanto espaço tem e quanto ainda falta escrever
    int espaco = 4096 - openfiles[file].cluster_offset;
    int falta = size - escrito;
    int copiar;
    
    if (falta < espaco) {
      copiar = falta;
    } else {
      copiar = espaco;
    }

    // Copia os dados do buffer do utilizador para o nosso buffer
    memcpy(buf_temp + openfiles[file].cluster_offset, buffer + escrito, copiar);

    // Grava o bloco modificado de volta no disco
    bl_write(openfiles[file].current_cluster, buf_temp);

    // Avança os contadores
    openfiles[file].cluster_offset += copiar;
    escrito += copiar;
  }

  //Atualiza o tamanho do arquivo no diretorio somando o que foi escrito
  dir[id_dir].size += escrito;
  
  // Salva os 32 blocos da FAT
  char *p_fat = (char *)fat;
  for (i = 0; i < 32; i++) {
    bl_write(i, p_fat + (i * 4096));
  }

  // Salva o bloco 32 que é o Diretorio
  bl_write(32, (char *)dir);

  return escrito; // Retorna o total de bytes que conseguiu escrever
}

int fs_read(char *buffer, int size, int file) {
  printf("Função não implementada: fs_read\n");
  return -1;
}

