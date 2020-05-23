/*
Herick Valsecchi Carlsen 15159619
João Pedro Favara 16061921
Raissa Furlan Davinha 15032006
Leonardo Blanco Natis 15296858
Kaíque Ferreira Fávero 15118698
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <pthread.h>
#include <limits.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/syscall.h>
#include <netdb.h>
#include <sys/stat.h>


/*
 * Servidor TCP
 */

#define commandSizes 200
#define MaxArray 10
#define FILESIZE 4096
#define DIGITOSTELEFONE 8

struct args{
	int ns;
	int s;
	int thread_id;
};

struct cliente {
	char telefone[DIGITOSTELEFONE];
	int porta;
	int thread_id;
};

struct args parameters;
pthread_t thread_id[MaxArray];
struct cliente contatos[MaxArray];
unsigned short port;
int countClients = 0;
int countContatos = 0;
int s_file, ns_file;

char command[commandSizes];

int conectar_file(char hostname[], int porta) {
    unsigned short port;
    struct hostent *hostnm;
    struct sockaddr_in server;

    /*
     * Obtendo o endereco IP do servidor
     */
    hostnm = gethostbyname(hostname);
    if (hostnm == (struct hostent *) 0)
    {
        fprintf(stderr, "Gethostbyname failed\n");
        exit(2);
    }
    port = (unsigned short) (porta);

    /*
     * Define o endereco IP e a porta do servidor
     */
    server.sin_family      = AF_INET;
    server.sin_port        = htons(port);
    server.sin_addr.s_addr = *((unsigned long *)hostnm->h_addr);

    /*
     * Cria um socket TCP (stream)
     */
    if ((s_file = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket()");
        exit(3);
    }

    /* Estabelece conexao com o servidor */
    if ((connect(s_file, (struct sockaddr *)&server, sizeof(server))) < 0)
    {
        perror("Connect()");
        exit(4);
    }

	printf("hostname: %s\n", hostname);
    printf("porta: %i\n", porta);


    return s_file;
}

long file_size(char *name) {
    FILE *fp = fopen(name, "rb"); //must be binary read to get bytes

    long size= 0 ;
    if(fp)
    {
        fseek (fp, 0, SEEK_END);
        size = ftell(fp);
        fclose(fp);
    }
    return size;
}

void enviar(int ns, int s) {
	FILE *fp;
	char buf[200], file_name[200];
	char line[FILESIZE];
	char path[PATH_MAX] = "dados/";
	long size_file;
	int port = 0;
	struct stat st = {0};

	// cria o diretorio de dados
	if (stat("/dados", &st) == -1) {
		mkdir("dados", 0777);
	}

	// recebe o nome do arquivo que sera salvo
	if (recv(ns, &buf, sizeof(buf), 0) == -1) {
		perror("Recv()");
		exit(6);
	}
	strcat(path, buf);
	printf("path: %s\n", path);

	// recebe o numero da porta que sera feito a transferencia dos dados
	if (recv(ns, &port, sizeof(port), 0) == -1)
	{
		perror("Recv()");
		exit(6);
	}

	//conecta com o cliente na porta
	int s_file;
	s_file = conectar_file("localhost", port);

	//recebe o tamanho do arquivo a ser recebido
	if (recv(s_file, &size_file, sizeof(size_file), 0) == -1) {
		perror("Recv()");
		exit(6);
	}

	snprintf(file_name, strlen(path), "%s", path);// pega o nome do arquivo de um uma variavel
	fp = fopen(file_name, "wb");// abre o arquivo

	if(fp) {
		int accum = 0; // quantidade de dados acumuladods
		int sent_bytes = 0; // quantidade de dados recebidos
		while(accum < size_file) {
			if ((sent_bytes = recv(s_file, &line, (sizeof(line)), 0)) < 0) {
				perror("Send()");
				exit(5);
			}
			accum += sent_bytes;//track size of growing file

			fwrite(line, sent_bytes, 1, fp);
		}
	}

	fclose(fp);
	close(s_file);
	printf("Fechou tudo!\n");
}

void receber(int ns, int s) {
	FILE *fp;
	char buf[200], file_name[200];
	char line[FILESIZE];
	char path[PATH_MAX] = "dados/";
	long size_file = 0;
	int port = 0;
	struct stat st = {0};

	// cria o diretorio de dados
	if (stat("/dados", &st) == -1) {
		mkdir("dados", 0777);
	}

	// recebe o nome do arquivo que sera enviado ao cliente
	if (recv(ns, &buf, sizeof(buf), 0) == -1) {
		perror("Recv()");
		exit(6);
	}
	strcat(path, buf);
	// printf("path: %s\n", path);

	// receber o numero da porta o cliente espera a conexao
	if (recv(ns, &port, (sizeof(port)), 0) == -1)
	{
		perror("Recv()");
		exit(6);
	}
	printf("port: %i\n", port);

	//conecta com o cliente na porta
	int s_file = conectar_file("localhost", port);

	//envia o tamanho do arquivo a ser enviado 
	size_file = file_size(path);
	printf("size_file: %li\n", size_file);
	if (send(s_file, &size_file, sizeof(size_file), 0) == -1) {
		perror("Recv()");
		exit(6);
	}

	snprintf(file_name, strlen(path)+1, "%s", path);// pega o nome do arquivo de um uma variavel
	fp = fopen(file_name, "rb");// abre o arquivo
	printf("file_name: %s\n", file_name);

	if(fp) {
        int sent_bytes = 0;
        while((sent_bytes = fread(line, 1, (sizeof(line)), fp)) && (size_file > 0)) {
            printf("sizeFile: %s - %i\n", line, sent_bytes);
            size_file -= sent_bytes;

            if (send(s_file, &line, sent_bytes, 0) < 0) {
                perror("Send()");
                exit(5);
            }
            memset(line, 0, sizeof(line));
        }
        printf("Finalizou o processo de envio ... \n");
    }

	fclose(fp);
	close(s_file);
	printf("Fechou tudo!\n");
}

void encerrar(int ns, int s, int thread_id) {

	for (int i = 0; i < countClients; i++) {
		if (contatos[i].thread_id == thread_id) {
			contatos[i].porta = 0;
			contatos[i].thread_id = NULL;
			printf("Thread[%i] esta na porta %i e possui o telefone %s\n", contatos[i].thread_id, contatos[i].porta, contatos[i].telefone);
		}
	};

	close(ns);
	printf("Thread[%d] has finished ...\n", thread_id);
	pthread_exit(NULL);
}

void listar(int ns) {
	DIR *dir;
    struct dirent *dp;
	char cwd[PATH_MAX];
	char stop[] = "stop";
	char copy[256];
	int port = 0;
	struct stat st = {0};

	if (stat("/dados", &st) == -1) {
		mkdir("dados", 0777);
	}

	// recebe a porta do socket criado pelo cliente ...
	if (recv(ns, &port, sizeof(port), 0) == -1)
	{
		perror("Recv()");
		exit(6);
	}
	printf("port: %i\n", port);

	//conecta com o cliente na porta
	conectar_file("localhost", port);

	// procura o diretorio
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		strcat(cwd, "/dados");
		printf("Current working dir: %s\n", cwd);
	}

	// le o diretorio
    if ((dir = opendir(cwd)) == NULL) {
        perror ("Cannot open .");
        exit (1);
    }

	// le os nomes de cada arquivo do diretorio
	memset(&command, 0, sizeof(command));
	while (dir) {
		if ((dp = readdir(dir)) != NULL) {
			strcpy(copy, dp->d_name);
			// printf("copy: %s\n", copy);
			if (send(s_file, &copy, (sizeof(copy)), 0) < 0) {
				perror("Send() 3");
				exit(7);
			}
		} else {
			if (send(s_file, &stop, (sizeof(stop)), 0) < 0) {
				perror("Send()");
				exit(7);
			}
			printf("Fechou a leitura do diretorio ... \n");
			closedir(dir);
			break;
		}
	}

	close(s_file);
}

void listar_contatos(int ns, int thread_id) {
	struct contatos {
		char telefone[DIGITOSTELEFONE];
		char status[10];
	};

	struct contatos contatos_enviar;

	//manda a quantidade de contatos ha no servidor ...
	int aux_countContatos = countContatos - 1;
	if (send(ns, &aux_countContatos, (sizeof(aux_countContatos)), 0) < 0) {
		perror("Send() 3");
		exit(7);
	}

	for(int i = 0; i < countContatos; i++) {
		printf("Telefone: %s - Porta: %d - Thread_id: %i\n", contatos[i].telefone, contatos[i].porta, contatos[i].thread_id);

		if (contatos[i].thread_id != thread_id) {
			strcpy(contatos_enviar.telefone, contatos[i].telefone);

			if (contatos[i].porta != 0) {
				strcpy(contatos_enviar.status, "Online");
			} else {
				strcpy(contatos_enviar.status, "Offline");
			}

			printf("Telefone: %s - Status: %s\n", contatos_enviar.telefone, contatos_enviar.status);

			if (send(ns, &contatos_enviar, (sizeof(contatos_enviar)), 0) < 0) {
				perror("Send() 3");
				exit(7);
			}
		}
	}
};

void adicionar_contato(int ns) {
	struct cliente aux_cliente;
	bool alteracao = false;

	// recebe a as infomacoes do cliente como a porta e o telefone
	if (recv(ns, &aux_cliente, sizeof(aux_cliente), 0) == -1)
	{
		perror("Recv()");
		exit(6);
	}

	strcpy(contatos[countContatos].telefone, aux_cliente.telefone);
	contatos[countContatos].porta = 0;
	contatos[countContatos].thread_id = 11;
	countContatos++;

};

void setup_contato(int ns, int thread_id) {

	struct cliente aux_cliente;
	bool alteracao = false;

	// recebe a as infomacoes do cliente como a porta e o telefone
	if (recv(ns, &aux_cliente, sizeof(aux_cliente), 0) == -1)
	{
		perror("Recv()");
		exit(6);
	}

	// verifica se nao possui nenhum contato com esse telefone
	for (int i = 0; i < countContatos; i++) {
		if ((strcmp(contatos[i].telefone, aux_cliente.telefone)) == 0 && (contatos[i].porta == 0)) {
			strcpy(contatos[thread_id].telefone, aux_cliente.telefone);
			contatos[thread_id].porta = aux_cliente.porta;
			contatos[thread_id].thread_id = thread_id;
			alteracao = true;
		}
	}

	/* caso nao haja nenhum contato com 
	esse telefone tem que ser criado um novo contato */
	if (!alteracao) {
		strcpy(contatos[thread_id].telefone, aux_cliente.telefone);
		contatos[thread_id].porta = aux_cliente.porta;
		contatos[thread_id].thread_id = thread_id;
		countContatos++;
	}

	printf("Thread[%i] esta na porta %i e possui o telefone %s\n", thread_id, aux_cliente.porta, aux_cliente.telefone);
};

void *recebe_comando(void* parameters){
	struct args args = *((struct args*) parameters);
	setup_contato(args.ns, args.thread_id);
    bool variavelLoop = false;
	int comando;

    do {
        /* Recebe uma mensagem do cliente atraves do novo socket conectado */
		// memset(&comando, 0, sizeof(comando));
		if (recv(args.ns, &comando, sizeof(comando), 0) == -1)
        {
            perror("Recv()");
            exit(6);
        }

		switch (comando) {
        case 1:
            adicionar_contato(args.ns);
            break;

        case 2:
            listar_contatos(args.ns, args.thread_id);
            break;
        
        case 3:
            printf("Comando 3 ...\n");
            break;

        case 4:
			encerrar(args.ns, args.s, args.thread_id);
            variavelLoop = false;
            break;

        default:
            printf("Comando invalido ... Por favor insira um novo comando\n");
            break;
        }

    } while(!variavelLoop);
	return NULL;
}

int main(int argc, char **argv){
	int s;                     /* Socket para aceitar conexoes       */
	int ns;                    /* Socket conectado ao cliente        */
	struct sockaddr_in client; 
	struct sockaddr_in server; 
	int namelen;
    /*
     * O primeiro argumento (argv[1]) e a porta
     * onde o servidor aguardara por conexoes
     */
    if (argc != 2)
    {
	  fprintf(stderr, "\nUse: %s porta", argv[0]);
	  exit(1);
    }

    port = (unsigned short) atoi(argv[1]);

    /*
     * Cria um socket TCP (stream) para aguardar conexoes
     */
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
	  perror("Socket()");
	  exit(2);
    }

   /*
    * Define a qual endereco IP e porta o servidor estara ligado.
    * IP = INADDDR_ANY -> faz com que o servidor se ligue em todos
    * os enderecos IP
    */
    server.sin_family = AF_INET;   
    server.sin_port   = htons(port);       
    server.sin_addr.s_addr = INADDR_ANY;

/* Imprime qual porta E IP foram utilizados. */
    printf("\nPorta utilizada: %d", ntohs(server.sin_port));
    printf("\nIP utilizado: %d\n", ntohs(server.sin_addr.s_addr));

	/*
     * Liga o servidor a porta definida anteriormente.
     */
    if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
	perror("Bind()");
	exit(3);
    }

    /*
     * Prepara o socket para aguardar por conexoes e
     * cria uma fila de conexoes pendentes.
     */
    if (listen(s, 1) != 0)
    {
	  perror("Listen()");
	  exit(4);
    }

    while(1)
    {
	  /*
	  * Aceita uma conexao e cria um novo socket atraves do qual
	  * ocorrera a comunicacao com o cliente.
	  */
	  namelen = sizeof(client);
	  if ((ns = accept(s, (struct sockaddr *) &client, (socklen_t *) &namelen)) == -1)
	  {
		perror("Accept()");
		exit(5);
	  }
		parameters.ns = ns;
		parameters.s = s;
		parameters.thread_id = countClients;

		if (pthread_create(&thread_id[countClients], NULL, recebe_comando, (void* )&parameters)) {
            printf("ERRO: impossivel criar uma thread\n");
            exit(-1);
        }
		printf("Thread[%i] criada\n", countClients);
		countClients++;
        // pthread_detach(thread_id[countClients]);
	}
}
