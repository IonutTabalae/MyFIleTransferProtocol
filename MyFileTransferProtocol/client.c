#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/wait.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;

#define PORT 2024
#define max 1000

void send_file(FILE *f, int sd)
{
    int n;
    char data[1024];
    bzero(data, 1024);

    while (fgets(data, 1024, f) != NULL)
    {
        if (write(sd, data, sizeof(data)) == -1)
        {
            perror("Eroare la trimiterea fisierului");
            errno;
        }
        bzero(data, 1024);
    }
}

void write_file(int sd)
{
    int n;
    FILE *f;
    char *filename = "download.txt";
    char buffer[1024];

    f = fopen(filename, "w");
    while (1)
    {
        n = read(sd, buffer, 1024);
        if (n <= 0)
        {
            break;
            return;
        }
        fprintf(f, "%s", buffer);
        bzero(buffer, 1024);
    }
    return;
}

int main(int argc, char *argv[])
{
    printf("\nBine ati venit! Folositi comanda /login pentru a va loga.\n\n");
    printf("\nSunt implementate urmatoarele comenzi:\n\n\n/login ......................................... Pentru a va loga\n/logout ........................................ Pentru a va deloga\n/quit .......................................... Pentru a inchide clientul\n/list files .................................... Listeaza fisierele de pe server\n/list files in <directory>...................... Listeaza fisiere din directorul specificat\n/create -d <name> .............................. Creeaza directorul <name> in server\n/create <name> ................................. Creaza fisierul <name> in server\n/delete -d <name> .............................. Sterge directorul <name> din server\n/delete <name> ................................. Sterge fisierul <name> din server\n/rename <old_name> <new_name> .................. Redenumeste directorul/fisierul <old_name> in <new_name>\n/move <path> <new_path> ........................ Muta directorul/fisierul din <path> in <new_path>\n/send <filename> ............................... Se va trimite fisierul <filename> din client in server sub numele de \"send\"\n/download <filename> ........................... Se va descarca <filename> din server in client sub numele de \"download\"\n\n");
    fflush(stdout);
    int sd;                    // descriptorul de socket
    struct sockaddr_in server; // structura folosita pentru conectare
    char msgin[max];
    char msgout[max];

    /* cream socketul */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Eroare la socket().\n");
        return errno;
    }

    server.sin_family = AF_INET;                     // familia socket-ului
    server.sin_addr.s_addr = inet_addr("127.0.0.1"); // adresa IP a serverului
    server.sin_port = htons(PORT);                   // portul de conectare

    /* ne conectam la server */
    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[client]Eroare la connect().\n");
        return errno;
    }

    while (1)
    {
        /*citirea mesajului*/
        bzero(msgout, max);
        read(0, msgout, sizeof(msgout)); // citire tastatura
        msgout[strlen(msgout) - 1] = 0;

        /*trimiterea mesajului la server*/
        if (write(sd, msgout, sizeof(msgout)) <= 0)
        {
            perror("[client]Eroare la write() spre server.\n");
            return errno;
        }

        bzero(msgin, max);
        /*citirea raspunsului dat de server (apel blocant)*/
        if (read(sd, msgin, sizeof(msgin)) <= 0)
        {
            perror("[client]Eroare la read() de la server.\n");
            return errno;
        }

        /*afisam mesajul primit*/
        printf("%s", msgin);

        if (strcmp(msgin, "\nQuitting...\n") == 0)
            break;

        if (strstr(msgin, "Fisierul este in curs de trimitere.\n") != NULL)
        {
            FILE *f;
            char filename[200];
            strcpy(filename, msgin + 36);
            f = fopen(filename, "r");
            if (f == NULL)
            {
                perror("Eroare la citirea din fisier");
                exit(1);
            }

            send_file(f, sd);
            printf("\nFisierul a fost trimis cu succes. Clientul se va inchide...\n");
            break;
        }
        if (strstr(msgin, "Fisierul este in curs de download.\n") != NULL)
        {
            FILE *f;
            write_file(sd);
            printf("\nFisierul a fost descarcat cu succes. Clientul se va inchide...\n");
            break;
        }
    }

    /* inchidem conexiunea, am terminat */
    close(sd);
}