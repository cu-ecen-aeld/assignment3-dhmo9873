//CODE USED FROM - https://beej.us/guide/bgnet/html/#acceptthank-you-for-calling-port-3490.
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

#define MYPORT "9000"  // the port users will be connecting to
#define BACKLOG 10     // how many pending connections queue holds

int main(void)
{
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints, *res;
    int sockfd, new_fd, write_fd;

    // !! don't forget your error checking for these calls !!

    // first, load up address structs with getaddrinfo():

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;  // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    int status = getaddrinfo(NULL, MYPORT, &hints, &res);
	if (status != 0){
		fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(status));
		return -1;
	}
    // make a socket, bind it, and listen on it:

    sockfd = socket(res->ai_family, res->ai_socktype,res->ai_protocol);
	if (sockfd < 0){
		perror("socket");
		return -1;
	}
	
	if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0){
		perror("bind");
		return -1;
	}
	
	if (listen(sockfd, BACKLOG) < 0){
		perror("socket");
		return -1;
	}
	freeaddrinfo(res);

    // now accept an incoming connection:

    addr_size = sizeof (their_addr);
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr,&addr_size);
	
	if (new_fd < 0) {
		perror("accept");
        return -1;
    }
	
	char buffer[1024];
	char packet[4096];
	int packet_len = 0;

	while (1) {
		int bytes = recv(new_fd, buffer, sizeof(buffer), 0);
		if (bytes <= 0)
			break;

		memcpy(packet + packet_len, buffer, bytes);
		packet_len += bytes;

		if (memchr(buffer, '\n', bytes)) {
			break;   
		}
	}

	int fd = open("/var/tmp/aesdsocketdata", O_WRONLY | O_CREAT | O_APPEND, 0644);
	write(fd, packet, packet_len);
	close(fd);

	fd = open("/var/tmp/aesdsocketdata", O_RDONLY);
	int r;
	while ((r = read(fd, buffer, sizeof(buffer))) > 0) {
		send(new_fd, buffer, r, 0);
	}
	close(fd);

	close(new_fd);
	close(sockfd);


   	return 0;
}	
