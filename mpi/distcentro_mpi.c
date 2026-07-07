#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include <mpi.h> 
 
/*  Entrada: numero de pontos (n)
 *  Saida:   vetor com a distância de cada ponto para a centroide
 *
 *  Estrategia de paralelizacao com MPI
 *  
 *  1) Somente o rank 0 gera os pontos que serao usados no calculo da centroide e distancias
 *  2) Faço um SCATTER do vetor para os nodos do cluster
 *  3) Cada nodo computa o valor parcial da centroide
 *  4) Aplico operacao de REDUCAO para calcular o valor integral da centroide
 *  5) Broadcast do valor da centroide para todos os nodos
 *  OBS: Pode eliminar o passo 5) ao usar REDUCE-ALL no passo 4
 *  6) Cada nodo computa a distancia Euclidiana dos seus pontos para a centroide
 *  7) Aplico GATHER para coletar todos os pontos no meu ROOT
 */

int main(int argc, char **argv) 
{
  int i, j;
  struct timeval start, stop;
  int    comm_sz;               /* Number of processes    */
  int    my_rank;               /* My process rank        */

  if (argc < 2) {
    printf("Necessário informar o número de pontos.\n");
    exit(-1);
  }
  int npontos = atoi(argv[1]);

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_sz); 
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank); 

  // aloca memória para os pontos (x,y) - i-ésimo ponto está na posição i*2 e
  // i*2+1 do vetor

  // usado pelo RANK 0 para a totalidade dos pontos e distancias
  int *g_pvetor = NULL;
  double *g_dist = NULL;

  // usado por todos para os calculos locais (pontos e distancias)
  int *l_pvetor;
  double *l_dist;

  // 1) Somente o rank 0 gera os pontos que serao usados no calculo da centroide e distancias
  if (my_rank == 0) {    
    g_pvetor = (int *)malloc(npontos*2*sizeof(int));
    g_dist = (double *)malloc(npontos*sizeof(double));
    srand(0);
  // gera os pontos de forma aleatória
    for (i=0; i<npontos; i++)
    {
      g_pvetor[i*2] = rand() % 2048;
      g_pvetor[(i*2)+1] = rand() % 2048;
    }
    /*
    for (i=0; i<20; i++)
      printf("[%d] = %d -", i, g_pvetor[i]);
    printf("\n");
    */
  }

  int numPontosDist = npontos/comm_sz;

  // Assumindo que a divisao serah exata
  l_pvetor = (int *)malloc(numPontosDist*2*sizeof(int));
  l_dist = (double *)malloc(numPontosDist*sizeof(double));

  MPI_Barrier(MPI_COMM_WORLD);
  gettimeofday(&start, NULL);

 // 2) Faço um SCATTER do vetor para os nodos do cluster
  MPI_Scatter(g_pvetor, numPontosDist*2, MPI_INT,
                      l_pvetor, numPontosDist*2, MPI_INT,
                      0, MPI_COMM_WORLD);
/*
  for (int ii=0; ii<10; ii++) {
    printf("Rank %d - [%d] = %d -", my_rank, ii, l_pvetor[ii]);
  }
  printf("\n");
  exit(-1);
*/

  // calcula a centroide
 //  3) Cada nodo computa o valor parcial da centroide
  double soma_x_local = 0, soma_y_local = 0;
  for (i = 0; i < numPontosDist; i++) {
    soma_x_local += l_pvetor[i*2];
    soma_y_local += l_pvetor[(i*2)+1];
  }

  double cent_x=0, cent_y=0;

 // 4) Aplico operacao de REDUCAO para calcular o valor integral da centroide
 // 5) Broadcast do valor da centroide para todos os nodos
 // OBS: Pode eliminar o passo 5) ao usar REDUCE-ALL no passo 4
  MPI_Allreduce(&soma_x_local, &cent_x, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  MPI_Allreduce(&soma_y_local, &cent_y, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

  cent_x = cent_x / npontos;
  cent_y = cent_y / npontos;
/*
  if (my_rank == 0)
    printf("Centroide = (%g,%g)\n", cent_x, cent_y);
  exit(-1);
*/
  // calcula as distâncias euclidianas de cada ponto para a centróide
 // 6) Cada nodo computa a distancia Euclidiana dos seus pontos para a centroide
  for (i = 0; i < numPontosDist; i++) {
    double a = l_pvetor[i*2], b = l_pvetor[(i*2)+1];
    l_dist[i] = sqrt((cent_x - a) * (cent_x - a)  +  (cent_y - b) * (cent_y - b));
  }
 
// 7) Aplico GATHER para coletar todos os pontos no meu ROOT
  MPI_Gather(l_dist, numPontosDist, MPI_DOUBLE,
             g_dist, numPontosDist, MPI_DOUBLE,
             0, MPI_COMM_WORLD);

  gettimeofday(&stop, NULL); 
 
  free(l_pvetor); free(l_dist);
  if (my_rank == 0) {

    double t = (((double)(stop.tv_sec)*1000.0  + (double)(stop.tv_usec / 1000.0)) - \
                   ((double)(start.tv_sec)*1000.0 + (double)(start.tv_usec / 1000.0)));
    fprintf(stdout, "Tempo decorrido = %g ms\n", t);

    printf("Vetor com as distâncias para a centroide (%g,%g):\n", cent_x, cent_y);
  
    // imprime o vetor com as distâncias
    for (i=0; i < npontos; i++) {
      printf("%d - %.3f \n", i, g_dist[i]);
    }
    printf("\n");

    free(g_pvetor); free(g_dist);
  }
 


  MPI_Finalize();

  return 0;
}
