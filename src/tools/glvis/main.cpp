#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include "glvis.h"

// ________________________________________________________________________________ 
// errors and warnings

void Error(const char *error, ...)
{
	va_list valist;
	char buffer[2048];

	va_start(valist, error);
	vsprintf(buffer, error, valist);
	va_end(valist);

	fprintf(stderr, "\x1b[31m");
	fprintf(stderr, "Error: %s", buffer);
	fprintf(stderr, "\x1b[0m");
	exit(1);
}

void Warning(const char *warning, ...)
{
	va_list valist;
	char buffer[2048];

	va_start(valist, warning);
	vsprintf(buffer, warning, valist);
	va_end(valist);

	fprintf(stdout, "\x1b[33m");
	fprintf(stdout, "Warning: %s", buffer);
	fprintf(stdout, "\x1b[0m");
}

static void ProcessEnviromentVariables(int argc, char *argv[])
{
	char *buffersizestr = getenv("GLVIEW_BUFFER_SIZE");
}

static void ProcessCommandLine(int argc, char *argv[])
{}

static void PrintUsage()
{
	printf("--fifo read from a fifo instead of stdin or input files\n");
}

#if 0
static void ReadBinaryImage(FILE *fp)
{
	int c;
	while ((c = fgetc(fp)) != EOF)
	{
		printf("%x\n", c);
		BufferWriteBytes(&c, 1);
	}
	BufferCommit();
	printf("committed buffer\n");
}
#endif

#if 0
// this is for stdin which may not be eof terminated (eg driven by netcat or fifo)
static void ReadBinaryImageStream(FILE *fp)
{
	int c;

	while ((c = fgetc(fp)) != EOF)
	{
		printf("%x\n", c);
		BufferWriteBytes(&c, 1);
		BufferCommit();
	}
	printf("got eof\n");
}
#endif

static int gargc;
static char **gargv;

static bool readkeepalive = true;

// ________________________________________________________________________________ 
// Net Read loop

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#if 0
int MakeListenSocket()
{
	int listensocket = socket(AF_INET, SOCK_STREAM, 0);
	//int ipv6_socket = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP);

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	//sin.sin_len = sizeof(sin);
	sin.sin_family = AF_INET; // or AF_INET6 (address family)
	sin.sin_port = htons(1234);
	sin.sin_addr.s_addr= INADDR_ANY;

	if (bind(listensocket, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		Error("Socket bind failed\n");

	listen(listensocket, 1);
	
	return listensocket;
}

static void NetReadLoop()
{
	//while(1)
	{
		int listensocket = MakeListenSocket();

		{
			printf("listening for connection\n");
			int readsocket = accept(listensocket, NULL, NULL);
			printf("connected\n");
			FILE *fp = fdopen(readsocket, "rb");
			setbuf(fp, NULL);
			if (!fp)
				Error("Failed to create fp\n");

			Read(fp);
			fclose(fp);
		}

		//printf("closing listen socket\n");
		//close(listensocket);

	}
}
#endif

static int NetMakeListenSocket()
{
	int listensocket = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET; // or AF_INET6 (address family)
	sin.sin_port = htons(1234);
	sin.sin_addr.s_addr= INADDR_ANY;

	if (bind(listensocket, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		Error("Socket bind failed\n");

	if (listen(listensocket, 1) < 0)
		Error("Socket listen call failed\n");
	
	return listensocket;
}

// Non-blocking accept call
static int NetAccept(int listensocket)
{
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(listensocket, &rfds);
	
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 25 * 1000; // 25 milliseconds between read checks

	//printf("calling select\n");
	int selret = select(listensocket + 1, &rfds, NULL, NULL, &tv);
	if (selret == -1)
	{
		Error("error in select\n");
		return 0;
	}
	else if (selret == 0)
		return 0;
	else
		return accept(listensocket, NULL, NULL);
}

static FILE *NetFilePointer(int fd)
{
	if (!fd)
		return NULL;

	FILE *fp = fdopen(fd, "rb");
	if (!fp)
		Error("Failed to create fp\n");

	setbuf(fp, NULL);

	return fp;
}

static void NetReadLoop()
{
	int listensocket = NetMakeListenSocket();

	do
	{
		int readsocket = 0;
		while (!readsocket)
			readsocket =  NetAccept(listensocket);

		FILE *fp = NetFilePointer(readsocket);

		Read(fp);

		fclose(fp);
	}
	while(readkeepalive);
}

// ________________________________________________________________________________ 
// Fifo read loop

static void FifoReadLoop()
{
	do
	{
		static const char *fifoname = "/tmp/f";
		FILE *fp = fopen(fifoname, "rb");
		if(!fp)
			Error("couldn't open fifo \"%s\"\n", fifoname);

		Read(fp);
	}
	while(readkeepalive);
}

// ________________________________________________________________________________ 
// Read and Draw threads

static void *ReadThread(void *args)
{
	int argc = gargc;
	char **argv = gargv;

	int i = 1;
	for (; i < argc && argv[i][0] == '-'; i++)
	{
		if (!strcmp(argv[i], "-k"))
			readkeepalive = true;
		else if (!strcmp(argv[i], "--fifo"))
		{
			FifoReadLoop();
			return EXIT_SUCCESS;
		}
		else if (!strcmp(argv[1], "--net"))
		{
			NetReadLoop();	
			return EXIT_SUCCESS;
		}
		else if (!strcmp(argv[i], "--help"))
		{
			PrintUsage();
			return EXIT_SUCCESS;	
		}
	}

	if (i == argc)
	{
		Read(stdin);
	}
	else
	{
		for (; i < argc; i++)
		{
			char *filename = argv[i];
			FILE *fp = fopen(filename, "rb");
			if(!fp)
				Error("couldn't open file \"%s\"", filename);
			
			Read(fp);
		}
	}

	return EXIT_SUCCESS;
}

static void *DrawThread(void *args)
{
	int argc = gargc;
	char **argv = gargv;

	DrawMain(argc, argv);
	return EXIT_SUCCESS;
}

static void SetupBuffer(int argc, char *argv[])
{
	// default size to 32 megs
	int numbytes = 32 * 1024 * 1024;
	for (int i = 1; i < argc && argv[i][0] == '-'; i++)
	{
		if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--buffer-size"))
			numbytes = atoi(argv[i + 1]);
	}

	BufferInit(numbytes);

	BufferFlush();
}

int main(int argc, char *argv[])
{
	gargc = argc;
	gargv = argv;

	SetupBuffer(argc, argv);

	pthread_t front, draw;
	pthread_create(&front, NULL, ReadThread, NULL);
	pthread_create(&draw, NULL, DrawThread, NULL);

	pthread_join(front, NULL);
	pthread_join(draw, NULL);

	return 0;
}

