#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>     /* -lsqlite3 */
#include <openssl/sha.h> /*libssl-dev    -lcrypto*/

#define PORT 2024
#define max 2000

extern int errno;
sqlite3 *db;

void write_file(int client)
{
    int n;
    FILE *f;
    char *filename = "./SERVER/send.txt";
    char buffer[1024];

    f = fopen(filename, "w");
    while (1)
    {
        n = read(client, buffer, 1024);
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

static int callback(void *str, int argc, char **argv, char **azColName) // pentru baza de date
{
    int i;
    char *data = (char *)str;

    for (i = 0; i < argc; i++)
    {
        strcat(data, azColName[i]);
        strcat(data, " = ");
        if (argv[i])
            strcat(data, argv[i]);
        else
            strcat(data, "NULL");
        strcat(data, "\n");
    }

    strcat(data, "\n");
    return 0;
}

int main(int argc, char *argv[])
{
    char msgin[max];  // mesajul primit de la client
    char msgout[max]; // mesaj de raspuns pentru client

    // comenzi fara login
    char login[] = "/login";
    char quit[] = "/quit";
    char help[] = "/help";

    // comenzi cu login
    char logout[] = "/logout";
    char ls[] = "/list files";
    char mkdir[] = "/create -d ";
    char touch[] = "/create ";
    char rmdir[] = "/delete -d ";
    char rm[] = "/delete ";
    char cd[] = "/list files in ";
    char rename[] = "/rename ";
    char move[] = "/move ";
    char send[] = "/send ";
    char download[] = "/download ";

    char message[max], username[200], password[max], result[max];

    // database
    char *zErrMsg;
    char str[max];
    char sql[max];
    int rc = sqlite3_open("MyFTP.db", &db); // stabilire conexiune baza date

    if (rc) // verificare conexiune cu DB
    {
        fprintf(stderr, "DB error: %s\n", sqlite3_errmsg(db));
        return (1);
    }

    int sd; // descriptor de socket

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) // creare socket
    {
        perror("[server]Eroare la socket ().\n");
        return errno;
    }

    int opt = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (void *)&opt, sizeof(opt));

    struct sockaddr_in server; // structura folosita de server
    struct sockaddr_in from;

    /*pregatirea structurilor de date*/
    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    server.sin_family = AF_INET;                // stabilirea familiei de socket-uri
    server.sin_addr.s_addr = htonl(INADDR_ANY); // acceptam orice adresa
    server.sin_port = htons(PORT);              // utilizam un port utilizator

    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) // atasare socket
    {
        perror("[server]Eroare la bind ().\n");
        return errno;
    }

    if (listen(sd, 1) == -1) // punem serverul sa asculte daca vin clienti sa se conecteze
    {
        perror("[server]Eroare la listen ().\n");
        return errno;
    }

    /*servim in mod concurent clientii*/
    while (1)
    {
        int client;
        int length = sizeof(from);

        printf("[server]Asteptam la portul: %d...\n\n", PORT);
        fflush(stdout);

        client = accept(sd, (struct sockaddr *)&from, &length); // acceptam un client (stare blocanta pina la realizarea conexiunii)

        if (client < 0) // eroare la acceptarea conexiunii de la un client
        {
            perror("[server]Eroare la accept().\n");
            continue;
        }

        // start children
        int pid;
        if ((pid = fork()) == -1)
        {
            close(client);
            continue;
        }
        else if (pid > 0)
        {
            // parinte
            close(client);
            while (waitpid(-1, NULL, WNOHANG))
                ;
            continue;
        }
        else if (pid == 0)
        {
            // copil
            close(sd);
            int login_status = 0;
            int closing = 0;
            int sending = 0;
            int downloading = 0;
            /*s-a realizat conexiunea, se asteapta mesajele*/

            while (1)
            {
                if (sending == 1)
                {
                    write_file(client);
                    sending = 0;
                    break;
                }
                if (downloading == 1)
                {
                    FILE *f;
                    char filename[100];
                    char path[300];
                    strcpy(path, "./SERVER/");
                    strcat(path, msgin + 10);
                    f = fopen(path, "r");
                    if (f == NULL)
                    {
                        perror("Eroare la citirea din fisier");
                        exit(1);
                    }
                    downloading = 0;
                    send_file(f, client);
                    bzero(filename, 100);
                    bzero(path, 300);
                    break;
                }

                // reading the message
                bzero(msgin, max);
                bzero(msgout, max);
                bzero(message, max);

                printf("[server]Asteptam mesajul...\n");
                fflush(stdout);

                if (read(client, msgin, max) <= 0) // citire mesaj
                {
                    perror("[server]Eroare la read() de la client.\n");
                    close(client); // inchidem conexiunea cu clientul
                    continue;      // continuam sa ascultam
                }

                printf("[server]Mesajul a fost receptionat.\n");

                strcpy(message, msgin);

                // COMENZI

                // no login required
                //  login user
                if (strcmp(message, login) == 0 && login_status == 0)
                {
                    bzero(result, max);
                    printf("[server]Incercare de logare...\n");
                    fflush(stdout);

                    char *zErrMsg;
                    char str[max];
                    char sql[max];
                    bzero(str, max);
                    bzero(sql, max);
                    int rc = sqlite3_open("MyFTP.db", &db); // stabilire conexiune baza date

                    if (rc) // verificare conexiune
                    {
                        fprintf(stderr, "DB error: %s\n", sqlite3_errmsg(db));
                        return 1;
                    }

                    write(client, "\nUSERNAME:\n", 63);
                    bzero(username, 200);
                    read(client, username, max);

                    write(client, "\nPASSWORD:\n", 33);
                    bzero(password, max);
                    // TODO: getpass() -> ca sa nu mai dea echo cand scriu parola
                    read(client, password, max);

                    unsigned char md[SHA_DIGEST_LENGTH];
                    // bzero(md, SHA256_DIGEST_LENGTH);
                    SHA1(password, strlen(password), md); // hashing cu SHA1

                    char h_pass[500];
                    for (int jj = 0; jj < sizeof(md); jj++) // transformarea din binary in hexa ca sa putem vedea mai bine
                    {
                        sprintf(h_pass + (jj * 2), "%02x", md[jj]);
                    }

                    bzero(str, max);
                    sprintf(sql, "SELECT * FROM users WHERE username = '%s' AND password = '%s';", username, h_pass);
                    rc = sqlite3_exec(db, sql, callback, str, &zErrMsg);

                    if (rc != SQLITE_OK)
                    {
                        login_status = 0;
                        printf("SQL error: %s\n", zErrMsg);
                        fflush(stdout);
                        sqlite3_free(zErrMsg);
                    }
                    else if (strstr(str, username) && strstr(str, h_pass) && strstr(str, "blacklist = 0"))
                    {
                        sprintf(result, "Bine ai venit %s. Ati fost logat cu succes.\n\n", username);
                        login_status = 1;
                        bzero(str, max);
                    }
                    else if (strstr(str, username) && strstr(str, h_pass) && strstr(str, "blacklist = 1"))
                    {
                        login_status = 0;
                        sprintf(result, "Ne pare rau, esti in blacklist. Pentru a-ti reactiva contul contacteaza adminul.\n\n");
                        bzero(str, max);
                    }
                    else
                    {
                        login_status = 0;
                        sprintf(result, "Username sau parola incorecta. Incercati din nou folosind comanda login!\n\n");
                        bzero(str, max);
                    }
                }
                else if (strcmp(message, login) == 0 && login_status == 1)
                {
                    bzero(result, max);
                    strcpy(result, "Esti deja logat.Foloseste comanda /logout pentru a te deloga.");
                }

                // quit
                else if (strcmp(message, quit) == 0)
                {
                    bzero(result, max);
                    strcpy(result, "\nQuitting...\n");
                    closing = 1;
                }

                // help
                /*else if (strcmp(message, help) == 0)
                {
                    bzero(result, max);
                    strcpy(result, "Help!"); // TO DO: De adaugat toate comenzile
                }*/

                // login required
                // logout
                else if (login_status == 1 && strcmp(message, logout) == 0)
                {
                    bzero(message, max);
                    bzero(msgin, max);
                    bzero(msgout, max);
                    bzero(str, max);
                    bzero(sql, max);
                    bzero(result, max);
                    strcpy(result, "Ati fost delogat cu succes!Puteti sa va logati din nou, cu aceasi comanda /login.\n\n");
                    login_status = 0;
                }

                // show files on server
                else if (login_status == 1 && strcmp(message, ls) == 0)
                {
                    bzero(result, max);
                    FILE *fp;
                    fp = popen("ls SERVER -xm --color=always", "r"); // facem un pipe care ruleaza comanda data
                    fgets(result, max, fp);                          // primim de la stream (fp adica pipe) un string de max ch
                    pclose(fp);
                }

                // create directory on server
                else if (login_status == 1 && strstr(message, mkdir) != NULL)
                {
                    bzero(result, max);

                    char comanda[max]; // comanda din terminal
                    bzero(comanda, max);

                    strcpy(comanda, "mkdir ./SERVER/");
                    strcat(comanda, message + 11); // atasam la comanda numele folderului care dorim sa il creem

                    if (system(comanda) != 0)
                        strcpy(result, "\nFolderul este deja existent pe server!\n\n");
                    else
                        strcpy(result, "\nFolderul a fost creat pe server cu succes!\n\n");
                }

                // create files on server
                else if (login_status == 1 && strstr(message, touch) != NULL)
                {
                    bzero(result, max);

                    char comanda[max]; // comanda din terminal
                    bzero(comanda, max);

                    strcpy(comanda, "touch ./SERVER/");
                    strcat(comanda, message + 8); // atasam la comanda numele folderului care dorim sa il creem

                    if (system(comanda) != 0)
                        strcpy(result, "\nFisierul este deja existent pe server!\n\n");
                    else
                        sprintf(result, "\nFisierul %s a fost creat pe server cu succes!\n\n", message + 8);
                }

                // remove directory from server
                else if (login_status == 1 && strstr(message, rmdir) != NULL)
                {
                    bzero(result, max);

                    char comanda[max];
                    bzero(comanda, max);

                    strcpy(comanda, "rm -rf ./SERVER/");
                    strcat(comanda, message + 11); // atasam la comanda numele folderului care dorim sa il stergem

                    if (system(comanda) != 0)
                        strcpy(result, "\nFolderul nu exista pe server!\n\n");
                    else
                        strcpy(result, "\nFolderul a fost sters cu succes de pe server!\n\n");
                }

                // remove files from server
                else if (login_status == 1 && strstr(message, rm) != NULL)
                {
                    bzero(result, max);

                    char comanda[max];
                    bzero(comanda, max);

                    strcpy(comanda, "rm ./SERVER/");
                    strcat(comanda, message + 8); // atasam la comanda numele folderului care dorim sa il stergem

                    if (system(comanda) != 0)
                        sprintf(result, "\nFisierul %s nu exista pe server!\n\n", message + 8);
                    else
                        strcpy(result, "\nFisierul a fost sters cu succes de pe server!\n\n");
                }

                // cd like command
                else if (login_status == 1 && strstr(message, cd) != NULL)
                {
                    bzero(result, max);

                    char comanda[max];
                    bzero(comanda, max);

                    strcpy(comanda, "ls SERVER/");
                    strcat(comanda, message + 15);
                    strcat(comanda, " -xm --color=always");

                    FILE *fp;
                    fp = popen(comanda, "r"); // facem un pipe care ruleaza comanda data
                    fgets(result, max, fp);   // primim de la stream (fp adica pipe) un string de max ch

                    if (strcmp(result, "") == 0)
                        strcpy(result, "\nFolderul ales este gol sau nu exista!\n\n");
                    else if (strstr(result, "SERVER") != 0)
                        strcpy(result, "\nNu poate fi deschis deoarece nu este un folder!\n\n");

                    pclose(fp);
                }

                // rename
                else if (login_status == 1 && strstr(message, rename) != NULL)
                {
                    bzero(result, max);
                    char d1[300];
                    char d2[100];
                    bzero(d1, 300);
                    bzero(d2, 100);

                    char comanda[max]; // comanda din terminal
                    bzero(comanda, max);

                    strcpy(message, message + 8);
                    int i = 0;
                    while (i <= strlen(message))
                    {
                        if (message[i] == ' ')
                        {
                            strcpy(d2, message + i + 1);
                            strncpy(d1, message, i);
                            break;
                        }
                        i++;
                    }
                    sprintf(comanda, "mv ./SERVER/%s ./SERVER/%s", d1, d2);

                    if (system(comanda) != 0)
                        strcpy(result, "\nFisierul cu numele specificat nu exista sau nu poate fi redenumit!\n\n");
                    else
                        strcpy(result, "\nFisierul specificat a fost redenumit cu succes!\n\n");
                }

                // move
                else if (login_status == 1 && strstr(message, move) != NULL)
                {
                    bzero(result, max);
                    char d1[100];
                    char d2[300];
                    bzero(d1, 100);
                    bzero(d2, 300);

                    char comanda[max]; // comanda din terminal
                    bzero(comanda, max);

                    strcpy(message, message + 6);
                    int i = 0;
                    while (i <= strlen(message))
                    {
                        if (message[i] == ' ')
                        {
                            strcpy(d2, message + i + 1);
                            strncpy(d1, message, i);
                            break;
                        }
                        i++;
                    }
                    sprintf(comanda, "mv ./SERVER/%s ./SERVER/%s", d1, d2);

                    if (system(comanda) != 0)
                        strcpy(result, "\nFisierul nu a putut fi mutat!\n\n");
                    else
                        strcpy(result, "\nFisierul a fost mutat cu succes!\n\n");
                }

                // send
                else if (login_status == 1 && strstr(message, send) != NULL)
                {
                    bzero(result, max);
                    char filename[100];
                    strcpy(filename, message + 6);
                    sprintf(result, "Fisierul este in curs de trimitere.\n%s", filename);
                    sending = 1;
                }

                // download
                else if (login_status == 1 && strstr(message, download) != NULL)
                {
                    bzero(result, max);

                    strcpy(result, "Fisierul este in curs de download.\n");
                    downloading = 1;
                }

                // eroare fara login
                else if (login_status == 0)
                {
                    bzero(result, max);
                    strcpy(result, "\nComanda gresita! Ati tastat gresit comanda sau nu aveti acces la comanda fara login!\n\n");
                }

                // eroare cu login
                else if (login_status == 1)
                {
                    bzero(result, max);
                    strcpy(result, "Comanda gresita!\n\n");
                }

                // eroare (nu se poate reproduce in rulare)
                else
                {
                    bzero(result, max);
                    strcpy(result, "\nEroare neasteptata! Incearca sa inchizi si sa deschizi din nou clientul.\n\n");
                }

                // sending the message back
                bzero(msgout, max);
                strcpy(msgout, result);
                int len_msgout = strlen(msgout);

                if (write(client, msgout, len_msgout) <= 0) // returnam mesajul clientului si continuam sa ascultam
                {
                    perror("[server]Eroare la write() catre client.\n");
                    continue;
                }

                if (closing == 1) // inchidere conexiune cu client
                {
                    close(client);
                    break;
                }
            }
            /*am terminat cu acest client, inchidem conexiunea*/
            close(client);
            exit(0);
        }
    } /*while*/

    sqlite3_close(db); // inchidere baza de date
} /*main*/