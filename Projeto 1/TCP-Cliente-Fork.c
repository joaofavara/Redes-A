/*
Herick Valsecchi Carlsen 15159619
João Pedro Favara 16061921
Raissa Furlan Davinha 15032006
Leonardo Blanco Natis 15296858
Kaíque Ferreira Fávero 15118698
*/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>


/*
 * Cliente TCP
 */
 
#define COMMAND 200
#define SEGMENT 5000 //approximate target size of small file
#define FILESIZE 4096

int s;

char command[COMMAND];

long file_size(char *name) {
    FILE *fp = fopen(name, "rb"); //must be binary read to get bytes

    long size;
    if(fp) {
        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fclose(fp);
    }
    return size;
}

void create_socket(int *s_enviar, int *ns_enviar) {
    // int *s_enviar;                     /* Socket para aceitar conexoes       */
	// int *ns_enviar = 0;                /* Socket conectado ao cliente        */
    unsigned short port;
	struct sockaddr_in client_enviar; 
	struct sockaddr_in server_enviar; 
	int namelen_enviar;
	FILE *fp;

    /*
     * O primeiro argumento (argv[1]) e a porta
     * onde o servidor aguardara por conexoes
     */

    port = (unsigned short) 21;

    /*
     * Cria um socket TCP (stream) para aguardar conexoes
     */
    if ((*s_enviar = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
	  perror("Socket()");
	  exit(2);
    }

   /*
    * Define a qual endereco IP e porta o servidor estara ligado.
    * IP = INADDDR_ANY -> faz com que o servidor se ligue em todos
    * os enderecos IP
    */
    server_enviar.sin_family = AF_INET;   
    server_enviar.sin_port   = htons(port);       
    server_enviar.sin_addr.s_addr = INADDR_ANY;

	/* Imprime qual porta E IP foram utilizados. */
    printf("\nPorta utilizada (enviar): %d", ntohs(server_enviar.sin_port));
    printf("\nIP utilizado (enviar): %d\n", ntohs(server_enviar.sin_addr.s_addr));

	/*
     * Liga o servidor a porta definida anteriormente.
     */
    if (bind(*s_enviar, (struct sockaddr *)&server_enviar, sizeof(server_enviar)) < 0)
    {
		perror("Bind() aqui ... ");
		exit(3);
    }

    /*
     * Prepara o socket para aguardar por conexoes e
     * cria uma fila de conexoes pendentes.
     */
    if (listen(*s_enviar, 1) != 0)
    {
	  perror("Listen()");
	  exit(4);
    }


     while(*ns_enviar == 0)
    {
	  /*
	  * Aceita uma conexao e cria um novo socket atraves do qual
	  * ocorrera a comunicacao com o cliente.
	  */
      printf("\n>>> while(1) - %i\n", *ns_enviar == 0);
	  namelen_enviar = sizeof(client_enviar);
	  if ((*ns_enviar = accept(*s_enviar, (struct sockaddr *) &client_enviar, (socklen_t *) &namelen_enviar)) == -1) {
		perror("Accept()");
		exit(5);
	  }
	}
}

void enviar(char comando[], char nome_local[], char nome_remoto[]) {
    FILE *fp;
    char str_aux[COMMAND];
    char bufsize[100];
    char file_name[strlen(nome_local)+1];
    int i, len, accum;
    long sizeFile = file_size(nome_local);
    char line[FILESIZE];
    int s_file = 0, ns_file = 0;

    strcpy(str_aux, comando);
    // enviar o comando enviar ...
    if (send(s, &str_aux, (sizeof(str_aux)), 0) < 0) {
        perror("Send() 1");
        exit(5);
    }

    // envia o nome que o arquivo sera salvo
    strcpy(str_aux, nome_remoto);
    if (send(s, &str_aux, (sizeof(str_aux)), 0) < 0) {
        perror("Send() 2");
        exit(5);
    }

    //cria socket e espera a conexao
    create_socket(&s_file, &ns_file);
    printf("s_file: %i - ns_file: %i\n", s_file, ns_file);

    // le o nome do arquivo
    snprintf(file_name, strlen(nome_local)+1, "%s", nome_local);
    fp = fopen(file_name, "rb");
    printf("file_name: %s\n", file_name);

    // enviar qual o tamanho do arquivo
    if (send(ns_file, &sizeFile, (sizeof(sizeFile)), 0) < 0) {
        perror("Send() 3");
        exit(5);
    }

    // strcpy(str_aux, "PUDIM");
    // if (send(ns_file, &str_aux, (sizeof(str_aux)), 0) < 0) {
    //     perror("Send() 3");
    //     exit(5);
    // }

    if(fp) {
        int sent_bytes = 0;
        while((sent_bytes = fread(line, 1, FILESIZE, fp)) && (sizeFile > 0)) {
            printf("sizeFile: %li - %i\n", sizeFile, sent_bytes);
            sizeFile -= sent_bytes;

            if (send(ns_file, &line, sent_bytes, 0) < 0) {
                perror("Send()");
                exit(5);
            }
            memset(line, 0, sizeof(line));
        }
        printf("Finalizou o processo de envio ... \n");
    }

    fclose(fp);
    close(s_file);
    close(ns_file);
    printf("Fechou tudo!\n");
}

void encerrar(const char list_command[]) {
    char comando_encerrar[COMMAND], nomeFile[256];
    strcpy(comando_encerrar, list_command);
    if (send(s, &comando_encerrar, (sizeof(comando_encerrar)), 0) < 0)
    {
        perror("Send()");
        exit(5);
    }
    close(s);
    printf("Finalizando o cliente ...\n");
    exit(0);
}

void listar(const char list_command[]) {
    char comando_listar[COMMAND], nomeFile[256];
    strcpy(comando_listar, list_command);
    DIR *dir;
    struct dirent *dp;
    int count = 0;

    if (send(s, &comando_listar, (strlen(comando_listar)), 0) < 0)
    {
        perror("Send()");
        exit(5);
    }

    // Recebe a lista de arquivos que estao no servidor...
    while((strcmp(nomeFile, "stop")) != 0) {
        if (recv(s, &nomeFile, (sizeof(nomeFile)), 0) < 0)
        {
            perror("Recv()");
            exit(6);
        }

        printf("> %s\n", nomeFile);
    };
}

void conectar(char hostname[], char porta[]) {
    unsigned short port;
    struct hostent *hostnm;
    struct sockaddr_in server;

    printf("hostname: %s\n", hostname);
    printf("porta: %s\n", porta);

    /*
     * Obtendo o endereco IP do servidor
     */
    hostnm = gethostbyname(hostname);
    if (hostnm == (struct hostent *) 0)
    {
        fprintf(stderr, "Gethostbyname failed\n");
        exit(2);
    }
    port = (unsigned short) atoi(porta);

    /*
     * Define o endereco IP e a porta do servidor
     */
    server.sin_family      = AF_INET;
    server.sin_port        = htons(port);
    server.sin_addr.s_addr = *((unsigned long *)hostnm->h_addr);

    /*
     * Cria um socket TCP (stream)
     */
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket()");
        exit(3);
    }

    /* Estabelece conexao com o servidor */
    if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Connect()");
        exit(4);
    }
}

// MAIN FUNCTION
int main(int argc, char **argv){
    bool variavelLoop = true;

    do {
        variavelLoop = false;
        printf("Insira um comando:\n");

        memset(command, 0, sizeof(command));
        fpurge(stdin);
        fgets(command,sizeof(command),stdin);

        char comando[strlen(command)]; 
        strcpy(comando, command);

        char *token = strtok(command, " ");

        int i = 0;
        char value[3][COMMAND];

        while( token != NULL ) {
            strcpy(value[i],token);
            printf("Token[%i] = %s\n", i, token);
            token = strtok(NULL, " ");
            i++;
        }

        printf("command: %s\n", value[0]);
        strtok(value[0],"\n");

        if ((strcmp(value[0], "conectar")) == 0) {
            conectar(value[1], value[2]);
        }

        if ((strcmp(value[0], "listar")) == 0) {
            printf("Listar foi chamado\n");
            listar(value[0]);
        }

        if ((strcmp(value[0], "encerrar")) == 0) {
            encerrar(value[0]);
        }

        if ((strcmp(value[0], "enviar")) == 0) {
            enviar(value[0], value[1], value[2]);
        }

    } while(!variavelLoop);

    /* Fecha o socket */
    close(s);

    printf("Cliente terminou com sucesso.\n");
    exit(0);

}


