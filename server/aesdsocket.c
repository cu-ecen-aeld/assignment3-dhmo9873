//CODE USED FROM - https://beej.us/guide/bgnet/html/#acceptthank-you-for-calling-port-3490.
//https://www.devzery.com/post/your-ultimate-guide-to-c-handlers
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <pthread.h>
#include <stdbool.h>

#define MYPORT "9000"
#define BACKLOG 10

volatile sig_atomic_t exit_flag = 0;

pthread_mutex_t mutex; 

struct thread_node {
	pthread_t thread;
	int  conn_id;
	bool thread_complete_success;
	struct sockaddr_storage *their_addr;
	struct thread_node *next_node;
};

void handle_signal(int sig) {
    syslog(LOG_INFO, "Caught signal, exiting");
	exit_flag = 1;
}

void* handle_connections(void *arg){
	struct thread_node *conn = (struct thread_node *)arg;

	/* Extract and log client IP address */
	char ipstr[INET6_ADDRSTRLEN];
	void *addr;

	if ((*(conn->their_addr)).ss_family == AF_INET) {
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)conn->their_addr;
		addr = &(ipv4->sin_addr);
	} else {
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)conn->their_addr;
		addr = &(ipv6->sin6_addr);
	}

	inet_ntop((*(conn->their_addr)).ss_family, addr, ipstr, sizeof(ipstr));
	syslog(LOG_INFO, "Accepted connection from %s", ipstr);

	char buffer[1024];
	char *packet = NULL;
	int packet_len = 0;

	/*
	 * Receive data until newline is detected.
	 * Each newline-terminated packet is appended to file.
	 */
	while (!exit_flag) {
		int bytes = recv(conn->conn_id, buffer, sizeof(buffer), 0);
		if (bytes <= 0)
			break;

		char *temp = realloc(packet, packet_len + bytes);
		if (!temp){
			free(packet);
			packet = NULL;
			break;
		}
		packet = temp;
		memcpy(packet + packet_len, buffer, bytes);
		packet_len += bytes;

		// Check if packet contains newline (packet complete)
		if (memchr(packet, '\n', packet_len) != NULL)
		{
			pthread_mutex_lock(&mutex);
			int fd = open("/var/tmp/aesdsocketdata", O_WRONLY | O_CREAT | O_APPEND, 0644);
			if (fd >= 0) {
				write(fd, packet, packet_len);
				close(fd);
			}

			// Send entire file contents back to client
			fd = open("/var/tmp/aesdsocketdata", O_RDONLY);
			if (fd >= 0) {
				ssize_t r;
				while ((r = read(fd, buffer, sizeof(buffer))) > 0) {
					send(conn->conn_id, buffer, r, 0);
				}
				close(fd);
			}

			pthread_mutex_unlock(&mutex);
			free(packet);
			packet = NULL;
			packet_len = 0;
		}
	}
	free(packet);
	close(conn->conn_id);

	syslog(LOG_INFO, "Closed connection from %s", ipstr);

	return NULL;
}


int main(int argc, char* argv[])
{
	pthread_mutex_init(&mutex, NULL);
	struct sigaction sa;

    // Configure signal handler
	sa.sa_handler = handle_signal;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);

    openlog("aesdsocket", 0, LOG_USER);

    // Register SIGINT handler
	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("sigaction SIGINT");
        syslog(LOG_ERR, "sigaction SIGINT failed");
		exit(EXIT_FAILURE);
	}

    // Register SIGTERM handler
	if (sigaction(SIGTERM, &sa, NULL) == -1) {
		perror("sigaction SIGTERM");
        syslog(LOG_ERR, "sigaction SIGTERM failed");
		exit(EXIT_FAILURE);
	}

    struct sockaddr_storage their_addr;   // Storage for client address
	socklen_t addr_size;
	struct addrinfo hints, *res;
	int sockfd;
	int daemon_mode = 0;
	struct thread_node *head = NULL;
	int ret;

    /* Check if -d argument is provided (daemon mode) */
	if (argc == 2 && strcmp(argv[1], "-d") == 0) {
		daemon_mode = 1;
	}

    // Prepare hints structure for getaddrinfo()
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;  
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;     

    // Get address information
	int status = getaddrinfo(NULL, MYPORT, &hints, &res);
	if (status != 0){
		fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(status));
        syslog(LOG_ERR, "getaddrinfo: %s", gai_strerror(status));
		return -1;
	}

    // Create socket
	sockfd = socket(res->ai_family, res->ai_socktype,res->ai_protocol);
	if (sockfd < 0){
		perror("socket");
        syslog(LOG_ERR, "socket error");
		return -1;
	}

    // Bind socket to port 9000
	if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0){
		perror("bind");
        syslog(LOG_ERR, "bind error");
		return -1;
	}

    // Start listening for connections
	if (listen(sockfd, BACKLOG) < 0){
		perror("listen");
        syslog(LOG_ERR, "listen error");
		return -1;
	}
	freeaddrinfo(res);

    if (daemon_mode) {
        pid_t pid = fork();
        if (pid < 0) {
            exit(EXIT_FAILURE);
        }
        if (pid > 0) {
            exit(EXIT_SUCCESS);   // Parent exits
        }

        umask(0);

        if (setsid() < 0) {
            exit(EXIT_FAILURE);
        }

        if (chdir("/") < 0) {
            exit(EXIT_FAILURE);
        }

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    /*
     * Main server loop:
     * Accept connections until signal received
     */
	while (!exit_flag) {

		addr_size = sizeof (their_addr);
		int new_fd = accept(sockfd, (struct sockaddr *)&their_addr,&addr_size);

		if (new_fd < 0) {
			if (errno == EINTR && exit_flag)
				break;
			perror("accept");
			continue;
		}

		struct thread_node *new_node = malloc(sizeof(struct thread_node));
		
		if (new_node == NULL){
			syslog(LOG_INFO, "Malloc for new thread_node creation failed");
			continue;
		}

		new_node->thread_complete_success = false;
		new_node->conn_id = new_fd;
		new_node->their_addr = &their_addr;
		new_node->next_node = head;
		head = new_node;

		ret = pthread_create(&new_node->thread, NULL, handle_connections, (void *)new_node);
     
		if (ret != 0 ) {
			free(new_node);
			continue;
		}
	

		struct thread_node* current;
		current = head;
		while(current != NULL){
			while (head != NULL && head->thread_complete_success){
				pthread_join(head->thread, NULL);
				struct thread_node *tmp = head;
				head = head->next_node;
				free(tmp);
			}
			current = head ;
			while(current != NULL && current->next_node != NULL){
				if (current->next_node->thread_complete_success){
					pthread_join(current->next_node->thread, NULL);
					struct thread_node *tmp = current->next_node;
					current->next_node = current->next_node->next_node;
					free(tmp);
				} else {
					current = current->next_node;
				}
			}

		}

	}
	close(sockfd);
	if (remove("/var/tmp/aesdsocketdata") == 0) {
        printf("File '%s' deleted successfully.\n", "/var/tmp/aesdsocketdata");
    } else {
        fprintf(stderr, "Error deleting file: ");
        perror(""); 
    }
    closelog();

	return 0;
}	
