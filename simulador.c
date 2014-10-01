///////////////////////////////////////////////////////////////////////////////
//  Prototipo de simulador de discos magneticos v0.1alfa
///////////////////////////////////////////////
//
//  Autor: Daniel Correa Lobato a partir de esqueleto gerado pelo ASiA
//  Data: abril, 2000
//
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "sss.h"

#define MAX_ZONES 50
#define MAX_SEEK  200

#define ARRIVL 1
#define STARTA 2
#define ENDACT 3

char* LeLinha (FILE* InStream, FILE* OutStream)
{
   char ENTRADA[100];
   char COMENT[1] = "#";
   do {
      fgets(ENTRADA, 100, InStream);
      strcat(ENTRADA, "\0");
      fprintf(OutStream, "%s %s", "Input>", ENTRADA);
   } while ((strncmp(ENTRADA,COMENT,1) == 0) && !feof(InStream));
   if (feof(InStream)) {
      printf("EOF do arquivo de configuracao antes do tempo!");
      exit(1);
   }
   return(ENTRADA);
}

/////////////////////////////////////////////////////////////////
//
// INCIO DA ROTINA PRINCIPAL
//
int main(int argc, char* argv[]) {
////////////////////////////////////////////////////////////////
// Variaveis do simulador
   long int event = 0;
   int busy = 0;
// Tempo de execucao da simulacao e numero maximo de requisicoes
   double Te, NumReq;
// Arquivos que serao lidos/escritos e de tabela de tempos de busca
   FILE *arq_entrada, *saida, *debug, *tabbusca;
// Setor que vai ser lido e cilindro anterior
   unsigned long int setor_atual, cilindro_ant;
// Cilindro atual e cabeca de leitura anterior
   unsigned long int cilindro_atual, cabeca_ant;
// Cabeca atual e deslocamento
   unsigned long int cabeca_atual, x;
// Tamanho da requisicao, zona, contador de I/Os
   unsigned long int s, zona, contador;
// Semente do gerador de numeros aleatorios
   int s1;
// Tipos de distribuicao
   int tipodistribcliente, tipodistribsetor, tipodistribtr;
// Parametros das distribuicoes
   double parC1, parC2, parS1, parS2, parT1, parT2;
// Tempo do proximo evento
   double proximo_tempo;

////////////////////////////////////////////////////////////////
// VARIAVEIS COM CARACTERISTICAS DO DISCO
// Tempo de revolucao e Tempo de busca
   double tr, tb;
// Constantes da equacao de Lee
   double a, b, c;
// Tempo medio, de full stroke e de troca de trilha do disco
   double avseek, fullstroke, trocatrilha;
// Total de blocos do disco e de blocos por superficie
   unsigned long int totalblocos;
   unsigned long int totalblocossuperficie;
// Atraso imposto pelo disco para escrita (nao utilizado na simulacao)
   double write_delay;
// Tempo de busca por 1..10 trilhas
   double seek[11];
// Tempo de troca de cabeca e de transferencia de um setor
   double trocacabeca, sectortransfer;
// Numero de RPMs, de superficies de dados, cilindros e numero de zonas
   int RPM, datasurface, cilindros, numband;
// Como calcula o tempo de busca?
   int comocalcula;
// Estrutura utilizada para armazenar as zonas do disco
   struct s_Zona {
      unsigned long int inicio, fim, sectrilha, setores, acumuladoV;
      unsigned long int acumuladoH;
   } zonas[MAX_ZONES];
// Estrutura utilizada para armazenar a tabela de tempos de busca
   struct s_Tempo {
      unsigned long int distancia;
      double tempo;
   } tempos[MAX_SEEK];
   unsigned long int tabela;

////////////////////////////////////////////////////////////////
// VARIAVEIS TEMPORARIAS
// Buffer de entrada
   char TMP[100];
// Variaveis auxiliares nos calculos
   unsigned long int offset_zona, sec_cil, resto;
   double passo;
// Contador eventual
   int i;

////////////////////////////////////////////////////////////////
//
// IDENTIFICACAO DOS ARQUIVOS DE ENTRADA, SAIDA E DEBUG
//
// Obtem o arquivo de entrada de dados
   printf("///////////////////////////////////////////////////////////////\n");
   printf("// Prototipo de simulador de discos magneticos v0.1alfa\n");
   printf("///////////////////////////////////////////////\n");
   printf("//\n");
   printf("// Autor: Daniel Correa Lobato\n");
   printf("// Data: abril, 2000\n");
   printf("//\n");
// Checa se todos os parametros estao presentes
   // O simulador vai ser utilizado apenas para calcular as constantes da
   //    equacao de Lee.
   if (argc == 5) {
      printf("// Modulo de calculo das constantes da equacao de Lee\n");
      printf("//\n");
      printf("// miSeek: %9.6f\n", atof(argv[1]));
      printf("// avSeek: %9.6f\n", atof(argv[2]));
      printf("// maSeek: %9.6f\n", atof(argv[3]));
      printf("// numCyl: %9.0f\n",   atof(argv[4]));
      printf("//\n");
   // A, B e C da equacao de Lee
      trocatrilha = atof(argv[1]);
      avseek      = atof(argv[2]);
      fullstroke  = atof(argv[3]);
      cilindros   = atof(argv[4]);
      a = (-10*trocatrilha + 15*avseek - 5*fullstroke) / (3*sqrt(cilindros));
      b = (7*trocatrilha - 15*avseek + 8*fullstroke) / (3*cilindros);
      printf("// Constantes da equacao de Lee\n");
      printf("//\n");
      printf("// a: %9.6f\tb: %9.6f\tc: %9.6f\n", a, b, trocatrilha);
      printf("//\n");
      printf("///////////////////////////////////////////////\n");
      exit(0);
   }
   if (argc < 6) {
      printf("// Forma de uso:\n");
      printf("// C:\\>Simulador <input> <seed> <output> <seek?> <debug> ");
      printf("[seekfile]\n");
      printf("//\n");
      printf("//   <input>.... Arquivo de configuracao\n");
      printf("//   <seed>..... Semente do gerador de numeros aleatorios\n");
      printf("//   <output>... Arquivo de saida dos resultados\n");
      printf("//   <seek?>.... Tempos de busca calculados por (1) Lee ou\n");
      printf("//               (2) tabela externa\n");
      printf("//   <debug>.... Arquivo para debug da simulacao\n");
      printf("//   [seekfile]. Se <seek?>=2, arquivo com a tabela de tempo ");
      printf("de busca\n");
      printf("//\n");
      printf("// C:\\>Simulador <miSeek> <avSeek> <maSeek> <numCyl>\n");
      printf("//\n");
      printf("//   Calcula as constantes da equacao de Lee\n");
      printf("//\n");
      printf("///////////////////////////////////////////////\n");
      exit(1);
   }
   printf("// arquivo com configuracao do disco> ");

// Abre o arquivo de configuracao somente para leitura
   if ((arq_entrada = fopen(argv[1], "r")) == NULL) {
      printf("** ERRO: impossivel abrir arquivo de configuracao desejado\n");
      exit(1);
   }
   printf("%s\n", argv[1]);

// Qual a semente?
   printf("//                semente do gerador> ");
   s1 = atoi(argv[2]);
   printf("%d\n", s1);

// Descobrir o nome do arquivo de saida e abri-lo
   printf("//     arquivo de saida da simulacao> ");
   if ((saida = fopen(argv[3], "w")) == NULL) {
      printf("** ERRO: impossivel abrir arquivo de saida\n");
      fclose(arq_entrada);
      exit(1);
   }
   printf("%s\n", argv[3]);

// Como vai ser o calculo do tempo de busca???
   printf("//         calculo do tempo de busca> ");
   comocalcula = atoi(argv[4]);
   if      (comocalcula == 1) printf("Lee\n");
   else if (comocalcula == 2) printf("Tabela\n");
   else {
      printf("ERRO!!!");
      fclose(arq_entrada);
      fclose(saida);
   }

// Descobrir o nome do arquivo de debug e abri-lo
   printf("//     arquivo de debug da simulacao> ");
   if ((debug = fopen(argv[5], "w")) == NULL) {
      printf("** ERRO: impossivel abrir arquivo de debug\n");
      fclose(arq_entrada);
      fclose(saida);
      exit(1);
   }
   printf("%s\n", argv[5]);

// Vai buscar em tabela??? Qual o nome?
   if (comocalcula == 2) {
      if (argc == 7) {
         printf("//       arquivo com tempos de busca> %s\n",argv[6]);
      }
      else {
         printf("** ERRO: arquivo com tempos de busca nao fornecido\n");
         fclose(arq_entrada);
         fclose(saida);
         fclose(debug);
         exit(1);
      }
   }

////////////////////////////////////////////////////////////////
//
// LEITURA DOS PARAMETROS DA SIMULACAO
//
   printf("// Lendo configuracao ");
   fprintf(debug,"*********************************************************\n");
   fprintf(debug,"*** Lendo configuracoes a partir do arquivo %s\n", argv[1]);
   fprintf(debug,"*********************************************************\n");
   fprintf(debug,"\n");

// Leitura dos parametros da simulacao
   strcpy(TMP,LeLinha(arq_entrada, saida)); Te = atof(TMP);
   fprintf(debug,"Tempo de simulacao (ms)> %f\n", Te);
   strcpy(TMP,LeLinha(arq_entrada, saida)); NumReq = atof(TMP);
   fprintf(debug,"Numero maximo de requisicoes> %f\n", NumReq);
   printf("\b-");

// Leitura dos parametros da distribuicao de chegada de clientes
   strcpy(TMP,LeLinha(arq_entrada, saida)); tipodistribcliente = atoi(TMP);
   if ((tipodistribcliente < 1) || (tipodistribcliente > 7)) {
      fprintf(debug,"Tipo de distribuicao de chegada de cliente ");
      fprintf(debug,"(%d) invalido!", tipodistribcliente);
      fclose(arq_entrada); fclose(debug); fclose(saida);
      exit(1);
   }
   strcpy(TMP,LeLinha(arq_entrada, saida)); parC1 = atof(TMP);
   strcpy(TMP,LeLinha(arq_entrada, saida)); parC2 = atof(TMP);
   fprintf(debug,"Distribuicao de chegada e parametros> ");
   fprintf(debug,"%d - %8.3f - %8.3f\n", tipodistribcliente, parC1, parC2);
   printf("\b\\");

// Leitura dos parametros da distribuicao do setor a ser lido
   strcpy(TMP,LeLinha(arq_entrada, saida)); tipodistribsetor = atoi(TMP);
   if ((tipodistribsetor < 1) || (tipodistribsetor > 7)) {
      fprintf(debug,"Tipo de distribuicao do setor a ser lido ");
      fprintf(debug,"(%d) invalido!", tipodistribsetor);
      fclose(arq_entrada); fclose(debug); fclose(saida);
      exit(1);
   }
   strcpy(TMP,LeLinha(arq_entrada, saida)); parS1 = atof(TMP);
   strcpy(TMP,LeLinha(arq_entrada, saida)); parS2 = atof(TMP);
   fprintf(debug,"Distribuicao do setor e parametros> ");
   fprintf(debug,"%d - %8.3f - %8.3f\n", tipodistribsetor, parS1, parS2);
   printf("\b|");

// Leitura dos parametros da distribuicao do tamanho da requisicao
   strcpy(TMP,LeLinha(arq_entrada, saida)); tipodistribtr = atoi(TMP);
   if ((tipodistribtr < 1) || (tipodistribtr > 7)) {
      fprintf(debug,"Tipo de distribuicao do tamanho da requisicao ");
      fprintf(debug,"(%d) invalido!", tipodistribtr);
      fclose(arq_entrada); fclose(debug); fclose(saida);
      exit(1);
   }
   strcpy(TMP,LeLinha(arq_entrada, saida)); parT1 = atof(TMP);
   strcpy(TMP,LeLinha(arq_entrada, saida)); parT2 = atof(TMP);
   fprintf(debug,"Distribuicao do tamanho da requisicao e parametros> ");
   fprintf(debug,"%d - %8.3f - %8.3f\n", tipodistribtr, parT1, parT2);
   printf("\b/");

////////////////////////////////////////////////////////////////
//
// LEITURA DOS PARAMETROS DO DISCO
//
// Tempo de troca de trilha
   strcpy(TMP,LeLinha(arq_entrada, saida)); trocatrilha = atof(TMP);
   fprintf(debug,"Valor assumido para trocatrilha> %f\n", trocatrilha);
   printf("\b-");

// Tempo medio de busca
   strcpy(TMP,LeLinha(arq_entrada, saida)); avseek = atof(TMP);
   fprintf(debug,"Valor assumido para avseek> %f\n", avseek);
   printf("\b\\");

// Tempo de fullstroke
   strcpy(TMP,LeLinha(arq_entrada, saida)); fullstroke = atof(TMP);
   fprintf(debug,"Valor assumido para fullstoke> %f\n", fullstroke);
   printf("\b|");

// Atraso de escrita
   strcpy(TMP,LeLinha(arq_entrada, saida)); write_delay = atof(TMP);
   fprintf(debug,"Valor assumido para write_delay> %f\n", write_delay);
   printf("\b/");

// Se for o caso, ler os tempos de busca a partir da tabela
   if (comocalcula == 2) {
      if ((tabbusca = fopen(argv[6], "r")) == NULL) {
         printf("** ERRO: impossivel abrir %s com tempos de busca\n", TMP);
         fclose(arq_entrada); fclose(debug); fclose(saida);
         exit(1);
      }
      strcpy(TMP,LeLinha(tabbusca, saida));
   // Ha, tabelado, tempo de busca para <tabela> distancias. Vamos le-los...
      tabela = atoi(TMP);
      if (tabela > MAX_SEEK){
         printf("** ERRO: Excedido o limite de linhas na tabela de tempos ");
         printf(" de busca. Limite atual: %d\n", MAX_SEEK);
         fclose(arq_entrada); fclose(debug); fclose(saida);
         exit(1);
      }
      for(i=1;i<=tabela;i++) {
         strcpy(TMP,LeLinha(tabbusca, saida));
      // Para a distancia tempos[i].distancia...
         tempos[i].distancia = atoi(TMP);
         strcpy(TMP,LeLinha(tabbusca, saida));
      // ...o tempo de busca eh de tempos[i].tempo
         tempos[i].tempo = atof(TMP);
      // E tenho o dito!
         fprintf(debug,"   Busca por %5lu cilindros",tempos[i].distancia);
         fprintf(debug," demora %3.6fms\n", tempos[i].tempo);
         if (i % 4 == 0) printf("\b-");
         if (i % 4 == 1) printf("\b\\");
         if (i % 4 == 2) printf("\b|");
         if (i % 4 == 3) printf("\b/");
      }
      fclose(tabbusca);
   }

// Tempo de busca por 1..10 trilhas
   for (i=1;i<=10;i++) {
      strcpy(TMP,LeLinha(arq_entrada, saida)); seek[i] = atof(TMP);
      fprintf(debug,"Valor assumido para seek[%d]> %f\n", i, seek[i]);
      if (i % 4 == 0) printf("\b-");
      if (i % 4 == 1) printf("\b\\");
      if (i % 4 == 2) printf("\b|");
      if (i % 4 == 3) printf("\b/");
   }

// Tempo de troca de cabeca
   strcpy(TMP,LeLinha(arq_entrada, saida)); trocacabeca = atof(TMP);
   fprintf(debug,"Valor assumido para trocacabeca> %f\n", trocacabeca);
   printf("\b/");

// RPM
   strcpy(TMP,LeLinha(arq_entrada, saida)); RPM = atoi(TMP);
   fprintf(debug,"Valor assumido para RPM> %d\n", RPM);
   printf("\b-");

// Superficies de dados
   strcpy(TMP,LeLinha(arq_entrada, saida)); datasurface = atoi(TMP);
   fprintf(debug,"Valor assumido para datasurface> %d\n", datasurface);
   printf("\b\\");

// Cilindros
   strcpy(TMP,LeLinha(arq_entrada, saida)); cilindros = atoi(TMP);
   fprintf(debug,"Valor assumido para cilindros> %d\n", cilindros);
   printf("\b|");

// Tempo de transferencia de um setor
   strcpy(TMP,LeLinha(arq_entrada, saida)); sectortransfer = atof(TMP);
   fprintf(debug,"Valor assumido para sectortransfer> %f\n", sectortransfer);
   printf("\b/");

// Numero de zonas
   strcpy(TMP,LeLinha(arq_entrada, saida)); numband = atoi(TMP);
   fprintf(debug,"Valor assumido para numband> %d\n", numband);
   if (numband > MAX_ZONES){
      printf("** ERRO: Excedido o limite de zonas no disco. ");
      printf("Limite atual: %d\n", MAX_ZONES);
      fclose(arq_entrada); fclose(debug); fclose(saida);
      exit(1);
   }
   printf("\b-");

// Agora, popular a estrutura de zonas numband vezes
   for (i=1;i<=numband;i++) {
      fprintf(debug,"   Dados da banda %d\n", i);
      strcpy(TMP,LeLinha(arq_entrada, saida)); zonas[i].inicio = atoi(TMP);
      fprintf(debug,"      inicio....... %lu\n", zonas[i].inicio);

      strcpy(TMP,LeLinha(arq_entrada, saida)); zonas[i].fim = atoi(TMP);
      fprintf(debug,"      fim.......... %lu\n", zonas[i].fim);

      strcpy(TMP,LeLinha(arq_entrada, saida)); zonas[i].sectrilha = atoi(TMP);
      fprintf(debug,"      sectrilha.... %lu\n", zonas[i].sectrilha);

      zonas[i].setores = (zonas[i].sectrilha * \
                         (zonas[i].fim - zonas[i].inicio + 1));
      fprintf(debug,"      setores...... %lu\n", zonas[i].setores);
      if (i == 1) {
         zonas[i].acumuladoH = zonas[i].setores;
         zonas[i].acumuladoV = datasurface * zonas[i].acumuladoH;
      }
      else {
         zonas[i].acumuladoH = zonas[i-1].acumuladoH + zonas[i].setores;
         zonas[i].acumuladoV = datasurface * zonas[i].acumuladoH;
      }
      fprintf(debug,"      acumuladosH.. %lu\n", zonas[i].acumuladoH);
      fprintf(debug,"      acumuladosV.. %lu\n", zonas[i].acumuladoV);
      if (i % 4 == 0) printf("\b-");
      if (i % 4 == 1) printf("\b\\");
      if (i % 4 == 2) printf("\b|");
      if (i % 4 == 3) printf("\b/");
  }

// Fecha o arquivo de entrada
   fclose(arq_entrada);
   printf("\n");

// Calcula o numero de blocos no disco e por superficie
   printf("// Calculando variaveis dependentes -\n");
   totalblocossuperficie = zonas[numband].acumuladoH;
   totalblocos = totalblocossuperficie * datasurface;

   fprintf(debug,"\nTotal de blocos por superficie...: ");
   fprintf(debug,"%lu setores\n", totalblocossuperficie);
   fprintf(debug,"Total de blocos no disco.........: ");
   fprintf(debug,"%lu setores\n", totalblocos);

// Se algum dos parametros de distribuicao de setor a ser lido for 1919, trocar
//    pelo numero total de blocos
   if (parS1 == 1919) {
      parS1 = (double)(totalblocos-1);
      fprintf(debug,"!! Reajustando o parametro parS1 para o ultimo ");
      fprintf(debug,"bloco (%10.0f) !!\n", parS1);
   }
   if (parS2 == 1919) {
      parS2 = (double)(totalblocos-1);
      fprintf(debug,"!! Reajustando o parametro parS2 para o ultimo ");
      fprintf(debug,"bloco (%10.0f) !!\n", parS2);
   }

// Se algum dos parametros de distribuicao do tamanho da requisicao for 1919,
//    trocar pelo numero total de blocos
   if (parT1 == 1919) {
      parT1 = (double)(totalblocos-1);
      fprintf(debug,"!! Reajustando o parametro parT1 para o ultimo ");
      fprintf(debug,"bloco (%10.0f) !!\n", parT1);
   }
   if (parT2 == 1919) {
      parT2 = (double)(totalblocos-1);
      fprintf(debug,"!! Reajustando o parametro parT2 para o ultimo ");
      fprintf(debug,"bloco (%10.0f) !!\n", parT2);
   }

////////////////////////////////////////////////////////////////
//
// CALCULO DE VARIAVEIS QUE DEPENDEM DOS PARAMETROS LIDOS
//
// Tempo de revolucao
   tr = (double)(1*60*1000)/(double)RPM;
   fprintf(debug,"Tempo de revolucao: %fms\n", tr);

// A, B e C da equacao de Lee
   a = (-10*trocatrilha + 15*avseek - 5*fullstroke) / (3*sqrt(cilindros));
   b = (7*trocatrilha - 15*avseek + 8*fullstroke) / (3*cilindros);
   c = trocatrilha;
   fprintf(debug,"\nConstantes da equacao de Lee\n");
   fprintf(debug,"a: %f\tb: %f\tc: %f\n", a, b, c);

   if ((a<0) || (b<0) || (c<0)) {
      fprintf(debug,"ATENCAO! Constantes da equacao de Lee negativas!\n");
      fprintf(debug,"~~~~~~~~ De acordo com (Lee, 2000) prefira calcular o ");
      fprintf(debug,"tempo de busca por interpolacao!!!\n");
   }

   fprintf(debug,"\n===========================[ Debug da simulacao ");
   fprintf(debug,"]===========================\n");

   fprintf(debug,"  cont    sector      size !  Z    Cyl SF Sec !      X");
   fprintf(debug,"  SeekTime !  ServTime\n");

////////////////////////////////////////////////////////////////
//
// INICIALIZACAO DA SIMULACAO
//
// prepara o sistema de simulacao e da nome ao modelo
   printf("//\n");
   printf("// Simulacao iniciada!\n");
   printf("//\n");

   INIQUE(1,1,5);
   SETSEE(s1);
   CREATE(0.0,0);
   INISTA(2,"TempoBusca", 0, 30, 1, (double)(fullstroke/30));
   INISTA(3,"TempoServico", 0, 30, (double)(0.5*tr), \
                                   (double)((double)(tr+fullstroke)/30));
   INISTA(4,"TempoResposta", 0, 30, 14, 2);
   INISTA(5,"Cilindro",0, 30, 0, totalblocos/29);

// Inicializa contadores auxiliares
   cilindro_ant = 0;
   cabeca_ant = 0;
   contador = 0;

   fprintf(saida,"\n\n=============================[ Saida da Simulacao ");
   fprintf(saida,"]=======================\n");

////////////////////////////////////////////////////////////////
//
// LACO PRINCIPAL DA SIMULACAO
//
   printf("// Executando E/Ss  ");
   do {
   	if (contador % 100 == 0) printf("\b-");
   	if (contador % 100 == 25) printf("\b\\");
   	if (contador % 100 == 50) printf("\b|");
   	if (contador % 100 == 75) printf("\b/");
      event = NEXTEV();
      if (event) switch (event) {
      ////////////////////////////////////////////////////////////////
      //
      // Hora de escalonar novo cliente
      //
      //
      case ARRIVL:
         SETA(1, T());
         switch(tipodistribcliente) {
         case 1: CREATE(BE(parC1,parC2), IDE() + 1); break;
         case 2: CREATE(ER(parC1,parC2), IDE() + 1); break;
         case 3: CREATE(EX(parC1), IDE() + 1); break;
         case 4: CREATE(GA(parC1,parC2), IDE() + 1); break;
         case 5: CREATE(RN(parC1,parC2), IDE() + 1); break;
         case 6: CREATE(NP(parC1), IDE() + 1); break;
         case 7: CREATE(UN(parC1,parC2), IDE() + 1); break;
         }
         if (busy) QUEUE(1,0);
         else SCHED(0.0, STARTA, IDE());
         break;
      ////////////////////////////////////////////////////////////////
      //
      // Atender um cliente
      //
      case STARTA:
         busy = 1;
         proximo_tempo = 0.00;
         contador++;
         fprintf(debug,"%06lX ",contador);
      // QUAL setor eu vou ler???
         switch(tipodistribsetor) {
         case 1: setor_atual = (unsigned long int)BE(parS1,parS2); break;
         case 2: setor_atual = (unsigned long int)ER(parS1,parS2); break;
         case 3: setor_atual = (unsigned long int)EX(parS1); break;
         case 4: setor_atual = (unsigned long int)GA(parS1,parS2); break;
         case 5: setor_atual = (unsigned long int)RN(parS1,parS2); break;
         case 6: setor_atual = (unsigned long int)NP(parS1); break;
         case 7: setor_atual = (unsigned long int)UN(parS1,parS2); break;
         }
      // Se solicitar um setor alem do limite do disco, solicita o maior
      //    possivel...
         if (setor_atual  > (totalblocos - 1))
            setor_atual = totalblocos - 1;
         fprintf(debug,"%09ld ",setor_atual);
	 TALLY(5, setor_atual);

      // e QUANTOS setores a partir dele
         switch(tipodistribtr) {
         case 1: s = (unsigned long int)BE(parT1,parT2); break;
         case 2: s = (unsigned long int)ER(parT1,parT2); break;
         case 3: s = (unsigned long int)EX(parT1); break;
         case 4: s = (unsigned long int)GA(parT1,parT2); break;
         case 5: s = (unsigned long int)RN(parT1,parT2); break;
         case 6: s = (unsigned long int)NP(parT1); break;
         case 7: s = (unsigned long int)UN(parT1,parT2); break;
         }
      // Se estiver lendo mais setores do que ha no disco, limita no maximo
      //    possivel...
         if (setor_atual + s > (totalblocos - 1))
            s = totalblocos - setor_atual - 1;
         fprintf(debug,"%09ld ! ", s);

      //////////////////////////////////////////////////////////////////////
      // DESMONTAR O SETOR EM ZONA, CILINDRO, SUPERFICIE, SETOR
      //
      // Em qual zona esta o setor desejado?                          (Passo 1)
         for (i=1;i<=numband;i++) {
            if (setor_atual < zonas[i].acumuladoV) {
               zona = i; break;
            }
         }

      // Temos quantos setores por cilindro para essa zona?           (Passo 2)
         sec_cil = zonas[zona].sectrilha * datasurface;

      // Quantos setores ha antes do inicio da zona?                  (Passo 3)
         if (zona == 1)
            offset_zona = setor_atual - 0;
         else
            offset_zona = setor_atual - zonas[zona-1].acumuladoV;

      // Sabendo a zona, descobre o cilindro...                       (Passo 4)
         cilindro_atual = (offset_zona / sec_cil) + zonas[zona].inicio;
         resto = offset_zona % sec_cil;

      // ... e o deslocamento em relacao a requisicao anterior
         x = abs(cilindro_atual - cilindro_ant);

      // Calcular em qual superficie esta o dado                      (Passo 5)
         cabeca_atual = (resto / zonas[zona].sectrilha);
         resto = resto % zonas[zona].sectrilha;

      // Achou a cabeca... Se mudou, acrescenta ao tempo...
         if (cabeca_atual != cabeca_ant)
            proximo_tempo += trocacabeca;

      // Anotar no debug o setor desmontado...
         fprintf(debug,"%02ld %06ld %02ld %03ld ! ", zona, cilindro_atual, \
                                                 cabeca_atual, resto);
      // ... e o deslocamento
         fprintf(debug,"%06ld ", x);
      //
      // DESMONTAR O SETOR EM ZONA, CILINDRO, SUPERFICIE, SETOR
      //////////////////////////////////////////////////////////////////////

      //////////////////////////////////////////////////////////////////////
      // CALCULAR O TEMPO DE BUSCA PARA O DESLOCAMENTO EM QUESTAO
      //
      // Se o deslocamento eh 0, nao tem tempo de busca
         if (x == 0)
            tb = (double)0.00;

      // Eh para usar Lee?
         else if (comocalcula == 1)
         // Se for pequena, usa a tabela de pequenas distancias
            if ((x >= 1) && (x <= 10)) tb = seek[x];
         // senao, usa Lee
            else tb = a * (double)sqrt(x - 1) + b * (double)(x - 1) + c;

      // Eh para usar interpolacao
         else if (comocalcula == 2) {
            for (i=1;i<=tabela;i++) {
               if (tempos[i].distancia == x) {
                  tb = tempos[i].tempo;
                  break;
               }
               if (tempos[i].distancia > x) {
                  passo = (tempos[i].tempo - tempos[i-1].tempo)/\
                          (double)(tempos[i].distancia - tempos[i-1].distancia);
                  tb = (double)(tempos[i-1].tempo + \
                       ((double)(x - tempos[i-1].distancia) * passo));
                  break;
               }
            }
         }
         else fprintf(debug,"@@@");

      // Isso aqui eh o SeekTime
         fprintf(debug,"%09.5f ! ",tb);
         proximo_tempo += tb;
         TALLY(2, proximo_tempo);

      //
      // CALCULAR O TEMPO DE BUSCA PARA O DESLOCAMENTO EM QUESTAO
      //////////////////////////////////////////////////////////////////////

      //////////////////////////////////////////////////////////////////////
      // CALCULAR O ATRASO ROTACIONAL
      // E o atraso rotacional???

      // Distribuição uniforme entre 0 e o tempo de rotação
         proximo_tempo += UN((double)0.00,tr);

      // CALCULAR O ATRASO ROTACIONAL
      //////////////////////////////////////////////////////////////////////

      //////////////////////////////////////////////////////////////////////
      // TEMPO DE TRANSFERENCIA DOS SETORES DESEJADOS

         proximo_tempo += (double)(sectortransfer * (double)s);
         fprintf(debug,"%09.5f\n",proximo_tempo);

      // TEMPO DE TRANSFERENCIA DOS SETORES DESEJADOS
      //////////////////////////////////////////////////////////////////////

      // Escalona o fim de servico e liberacao do disco
         SCHED(proximo_tempo, ENDACT, IDE());
         TALLY(3, proximo_tempo);

         cilindro_ant = cilindro_atual;
         cabeca_ant = cabeca_atual;
         break;
      ////////////////////////////////////////////////////////////////
      //
      // Liberar o disco para o proximo
      //
      //
      case ENDACT:
         TALLY(4, T() - A(1));
         if ((IDE() >= NumReq) || (T() >= Te)) SIMEND(0);
         DISPOS();
         busy = 0;
         if (NQ(1)) {
            REMVFQ(1,1);
            SCHED(0.0, STARTA, IDE());
         }
         break;
      }
   } while (event);

// Gera o relatorio da simulacao
   printf("\n//\n");
   printf("// Fim da simulacao!\n");
   printf("//\n");
   printf("// Numero de E/S: %ld\n", contador);
   printf("// Tempo de simulacao: %12.5fms\n",T());
   printf("///////////////////////////////////////////////\n");

   fprintf(saida,"Numero de ESs: ");
   fprintf(saida,"%ld\n",contador);
   fprintf(saida,"ES por segundo: ");
   fprintf(saida,"%10.5f\n",(double)contador/((double)T()/1000));
   fclose(saida);

   fprintf(debug,"Numero de ESs: ");
   fprintf(debug,"%ld\t",contador);
   fprintf(debug,"ES por segundo: ");
   fprintf(debug,"%10.5f\n",(double)contador/((double)T()/1000));
   fclose(debug);

   SUMRY(argv[3]);

   return (EXIT_SUCCESS);
}

