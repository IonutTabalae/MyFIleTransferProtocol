# MyFileTransferProtocol 

**_For a full comprehensive LaTeX description of the project and diagrams, go to this [document](MyFTP.pdf)._**

This is a TCP client-server application that allows file transfer between clients and server, as well as other actions with files/directories in a local file system.
A child process is created from the main process for each new connected client, by fork(). The client sends a command to the server (/login, /list files), and the server checks the syntax, interprets it, executes it and sends a response back to the client. Clients are identified by IP and port.

The language used is C.

## Features

- **Login**: The user must log in with a username and password that are stored in a SQLite database on the server. The password is transmitted securely, using SHA1 hashing function.
- **File Transfer**: The user can send or receive files to or from the server, using the commands `/send` and `/receive`. The files are transferred using TCP sockets, ensuring accurate and reliable data transmission.
- **File Management**: The user can perform various actions on the local files/directories, such as creating, renaming, deleting, moving, or listing the contents of the current or another directory, using the commands `/create`, `/rename`, `/delete`, `/move`, or `/list files`.
- 

## How it Works

For each new connected client, a child process is created from the main process by `fork()`. The client sends a command to the server (like `/login`, `/list files`), and the server checks the syntax, interprets it, executes it, and sends a response back to the client. Clients are identified by IP and port. The language used for this application is C.

## Installation

To run the application, you need to have a C compiler and SQLite installed on your system. You can download SQLite from [here](https://www.sqlite.org/download.html).

To compile the source code, use the following commands:

```bash
gcc -o server server.c -lsqlite3
gcc -o client client.c
```



## Usage

To start the server, use the following command:

```bash
./server <port>
```

where `<port>` is the port number that the server will listen on.

To start the client, use the following command:

```bash
./client <ip> <port>
```

where `<ip>` is the IP address of the server and `<port>` is the port number that the server is listening on.

To log in, use the following command:

```bash
/login
```

and then enter your username and password.

To see the available commands, use the following command:

```bash
/help
```

To exit the application, use the following command:

```bash
/quit
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
