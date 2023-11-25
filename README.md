# MyFileTransferProtocol: A Comprehensive TCP Client-Server Application

**_For a detailed LaTeX description of the project and diagrams, please refer to this [document](MyFTP.pdf)._**

MyFileTransferProtocol is a robust **TCP client-server application** developed in **C**, facilitating file transfer between clients and server. It also enables various actions with files/directories in a local file system. Each new connected client is handled by a child process, created from the main process using `fork()`. Clients are identified by IP and port.

## Key Features

- **Secure Login**: Users are authenticated using a username and password stored in a SQLite database on the server. Passwords are transmitted securely via SHA1 hashing function.
- **Reliable File Transfer**: Users can send or receive files to/from the server using the `/send` and `/receive` commands. File transfer is facilitated through TCP sockets, ensuring accurate and reliable data transmission.
- **Local File Management**: Users can perform various actions on local files/directories, such as creating, renaming, deleting, moving, or listing the contents of the current or another directory, using the commands `/create`, `/rename`, `/delete`, `/move`, or `/list files`.

## How it Works

For each new connected client, a child process is created from the main process by `fork()`. The client sends a command to the server (like `/login`, `/list files`), and the server checks the syntax, interprets it, executes it, and sends a response back to the client. Clients are identified by IP and port.

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
This project showcases skills in **network programming**, **multi-process architecture**, **database management** with SQLite, and **security practices** with SHA1 hashing. It's an excellent demonstration of **system programming** capabilities.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details. 


