#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

int main(void)
{
	unsigned char buffer[] = { 'M', 'O', 'D', 'B', 'U', 'S' };

	//adres docelowy
	struct sockaddr_in remote;
	//wyzerowanie pamieci zajmowanej przez strukturę
	memset(&remote, 0, sizeof(remote));
	//protokol IPv4
	remote.sin_family = AF_INET;
	//binarny adres ip
	inet_pton(AF_INET, "127.0.0.1", &remote.sin_addr);
	remote.sin_port = htons(4444); // standardowy port to 502

	//tworzenie gniazda TCP
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//polacznie
	connect(sock, (void*)&remote, sizeof(remote));
	//wyslanie bufora danych do serwera
	write(sock, buffer, sizeof(buffer));

	printf("wyslano");

	close(sock);
	return 0;
}
