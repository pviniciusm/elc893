#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

enum type{original=1, shared, openmp};

void welcome(){
  printf("---------------------------------------------------------------------\n");
  printf(" _    _      _                          \n"
         "| |  | |    | |\n"
         "| |  | | ___| | ___ ___  _ __ ___   ___\n"
         "| |/\\| |/ _ | |/ __/ _ \\| '_ ` _ \\ / _ \\\n"
         "\\  /\\  |  __| | (_| (_) | | | | | |  __/\n"
         " \\/  \\/ \\___|_|\\___\\___/|_| |_| |_|\\___|\n\n");

  printf("Implementacao do metodo MPI_Alltoallv com OPENMP:\n");
  printf("  1) openmp                  |process|\n\n");
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


void print_final(int rank, int* sharedint_local){
  #pragma omp barrier
  printf("\n");
  #pragma omp barrier

  if(rank == 0 ){
    print_buffer(sharedint_local, rank, 12, openmp);
  }
  #pragma omp barrier
  if(rank == 1){
    print_buffer(sharedint_local, rank, 12, openmp);
  }
  #pragma omp barrier
  if(rank == 2){
    print_buffer(sharedint_local, rank, 12, openmp);
  }
}


void main(int argc, char* argv[]){

  int comm_size, rank, rank1=0, i, j;
  char st[30];

  /* Mensagem de inicializacao */
  welcome();

  /* Buffer compartilhado
     Funciona de forma semelhante ao shm:
      - Cada thread possui seu proprio array compartilhado.
      - Nesse caso, sera representado por uma matriz
      - O resultado do all to all eh armazenado em vetores locais. */
  int **shared_var;
  shared_var = (int**) malloc (3*sizeof(int*));
  for (i=0; i<3; i++) shared_var[i] = (int*) malloc (12*sizeof(int));
  for (j=0; j<3; j++) for(i=0; i<12; i++) shared_var[j][i] = 500;


  #pragma omp parallel num_threads(3) private(comm_size, rank) shared(shared_var)
  {

    rank = omp_get_thread_num();
    comm_size = omp_get_num_threads();

    printf("Hello world from %d of %d!\n", rank, comm_size);

    /* Buffers */
    int *send_countbuf = (int *)malloc(3*sizeof(int));
    int *send_offsetbuf = (int *)malloc(3*sizeof(int));
    int *rec_countbuf = (int *)malloc(3*sizeof(int));
    int *rec_offsetbuf = (int *)malloc(3*sizeof(int));
    int *sharedint;
    int *sharedint_local = (int*)malloc(12*sizeof(int));

    int k;

    /* Atribuicao dos buffers send e receive */
    for(k=0; k<12; k++){
      shared_var[rank][k] = rank;
      sharedint_local[k] = 500;
    }

    /* Barreira de sincronizacao */
    #pragma omp barrier


    /* Inicializacao dos buffers de count e offset */
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

    /* Barreira de sincronizacao */
    #pragma omp barrier

    #pragma omp parallel for ordered
    for(i=0; i<comm_size; i++){
      /* Simula o win shared query do shm */
      sharedint = shared_var[i];

      /* O calculo eh o mesmo, baseado nos counters e offsets */
      for(k=send_offsetbuf[i]; k<send_offsetbuf[i]+send_countbuf[i]; k++){
        sharedint_local[k+rec_offsetbuf[i]] = sharedint[k];
      }
    }

    /* Print ordenado para melhor visualizacao */
    print_final(rank, sharedint_local);


    free(send_countbuf);
    free(send_offsetbuf);
    free(rec_countbuf);
    free(rec_offsetbuf);

    free(sharedint_local);
    free(shared_var[rank]);
  }
  free(shared_var);
}
