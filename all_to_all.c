#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"

enum type{original=1, shared, openmp};

void welcome(){
  printf("---------------------------------------------------------------------\n");
  printf(" _    _      _                          \n"
         "| |  | |    | |\n"
         "| |  | | ___| | ___ ___  _ __ ___   ___\n"
         "| |/\\| |/ _ | |/ __/ _ \\| '_ ` _ \\ / _ \\\n"
         "\\  /\\  |  __| | (_| (_) | | | | | |  __/\n"
         " \\/  \\/ \\___|_|\\___\\___/|_| |_| |_|\\___|\n\n");

  printf("Implementacao do metodo MPI_Alltoallv com MPI:\n");
  printf("  1) metodo original         [process]\n");
  printf("  2) local shared memory     (process)\n");
  printf("---------------------------------------------------------------------\n\n");
}

void update(int *buf, int x1, int x2, int x3){
  buf[0] = x1;
  buf[1] = x2;
  buf[2] = x3;
}

void do_work(){
  printf("Estou trabalhando!\n");
}

void work_as_root(){
  printf("Estou no root!\n");
}

int array_size(int *buf){
  return sizeof(buf)/sizeof(buf[0]);
}

void print_buffer(int *buf, int rank, int size, int type){
  int j;
  switch (type) {
    case original:
      printf("\n[%d]: ", rank);
      break;
    case shared:
      printf("\n(%d): ", rank);
      break;
    case openmp:
      printf("\n|%d|: ", rank);
      break;
  }

  for (j=0; j<size; j++) printf("%d, ",buf[j]);
  printf("\n");
}



void shm(int *sendbuf, int *send_countbuf, int *send_offsetbuf, int *rec_countbuf, int *rec_offsetbuf, int *recbuf){
  MPI_Aint aint = sizeof(int);
  int dspint = sizeof(int);

  int rank,size,i;

  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  /* Divisao dos processos em uma "comunidade" */
  MPI_Comm sharedcomm;
  MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &sharedcomm);

  /* Variavel a ser compartilhada em memoria */
  int *sharedint;

  /* Criacao de uma janela para compartilhamento */
  MPI_Win sm_win;      // Janela
  MPI_Info win_info;   // Informacoes
  MPI_Info_create(&win_info);
  MPI_Info_set(win_info, "alloc_shared_noncontig", "true");
  MPI_Win_allocate_shared(11*sizeof(int), sizeof(int), win_info, sharedcomm, &sharedint, &sm_win);
  MPI_Info_free(&win_info);


  int *sharedint_local = (int *)malloc(11*sizeof(int));


  /* Inicializacao dos vetores */
  for(i=0; i<11; i++){
    sharedint[i] = sendbuf[i];
    sharedint_local[i]=500;
  }

  MPI_Barrier(sharedcomm);

  int k;
  int init_rec;
  for(i=0; i<size; i++){
    MPI_Win_shared_query(sm_win, i, &aint, &dspint, &sharedint);
    for(k=send_offsetbuf[i]; k<send_offsetbuf[i]+send_countbuf[i]; k++){
      sharedint_local[k+rec_offsetbuf[i]] = sharedint[k];
    }
  }

  recbuf = sharedint_local;
}




void main(int argc, char* argv[]){

  char message[20];
  int  i, rank, size, type = 99, j;
  MPI_Status status;

  /* Inicializacao do MPI */
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if( rank == 0 ){
    welcome();
  }


  /* Message buffers */
  int *sendbuf, *recbuf;
  sendbuf = (int*)malloc(12*sizeof(int));
  recbuf = (int*)malloc(12*sizeof(int));

  for (j=0; j<12; j++) sendbuf[j]=rank;
  for (j=0; j<12; j++) recbuf[j]=500;

  /* All to all buffers */
  int *send_countbuf = (int *)malloc(3*sizeof(int));
  int *send_offsetbuf = (int *)malloc(3*sizeof(int));
  int *rec_countbuf = (int *)malloc(3*sizeof(int));
  int *rec_offsetbuf = (int *)malloc(3*sizeof(int));


  /* Buffer intializations */
  if(rank == 0){
    update(send_countbuf,4,2,2);
    update(send_offsetbuf,0,0,0);
    update(rec_countbuf,4,2,2);
    update(rec_offsetbuf,0,4,6);
  }else{
    update(send_countbuf,2,2,2);
    update(send_offsetbuf,0,0,0);
    update(rec_countbuf,2,2,2);
    update(rec_offsetbuf,0,2,4);
  }


  /* All to all call */
  MPI_Alltoallv(sendbuf, send_countbuf, send_offsetbuf, MPI_INT, recbuf, rec_countbuf, rec_offsetbuf, MPI_INT, MPI_COMM_WORLD);
  print_buffer(recbuf, rank, 12, 1);

  MPI_Barrier(MPI_COMM_WORLD);
  if( rank == 0 ){
    printf("\n-------------------------\n");
  }

  /* Shared memory call */
  shm(sendbuf, send_countbuf, send_offsetbuf, rec_countbuf, rec_offsetbuf, recbuf);
  print_buffer(recbuf, rank, 12, 2);

  MPI_Barrier(MPI_COMM_WORLD);
  if( rank == 0 ){
    printf("\n-------------------------\n");
  }


  free(send_countbuf);
  free(send_offsetbuf);
  free(rec_countbuf);
  free(rec_offsetbuf);
  free(sendbuf);
  free(recbuf);

  MPI_Finalize();

}
