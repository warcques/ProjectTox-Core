/* Messenger test
 *
 * This program adds a friend and accepts all friend requests with the proper message.
 *
 * It tries sending a message to the added friend.
 *
 * If it recieves a message from a friend it replies back.
 *
 *
 * This is how I compile it: gcc -O2 -Wall -D VANILLA_NACL -o test ../core/Lossless_UDP.c ../core/network.c ../core/net_crypto.c ../core/Messenger.c ../core/DHT.c ../nacl/build/${HOSTNAME%.*}/lib/amd64/{cpucycles.o,libnacl.a,randombytes.o} Messenger_test.c
 *
 *
 * Command line arguments are the ip, port and public_key of a node (for bootstrapping).
 *
 * EX: ./test 127.0.0.1 33445 CDCFD319CE3460824B33BE58FD86B8941C9585181D8FBD7C79C5721D7C2E9F7C
 *
 * Or the argument can be the path to the save file.
 *
 * EX: ./test Save.bak
 *
 */

#include "../core/Messenger.h"

#ifdef WIN32

#define c_sleep(x) Sleep(1*x)

#else
#include <unistd.h>
#include <arpa/inet.h>
#define c_sleep(x) usleep(1000*x)

#endif

static Messenger *messenger;

//horrible function from one of my first C programs.
//only here because I was too lazy to write a proper one.
unsigned char * hex_string_to_bin(char *hex_string)
{
    int len = strlen(hex_string);
    unsigned char * val = malloc(len);
    char *iter = hex_string;

    int i;
    for(i = 0; i < len; i++) {
        sscanf(iter, "%2hhx", &val[i]);
        iter += 2;
        i++;
    }

    return val;
}


void print_request(Messenger *m, uint8_t * public_key, char * data, uint16_t length)
{
    printf("Friend request recieved from: \n");
    printf("ClientID: ");
    uint32_t j;
    for(j = 0; j < 32; j++)
    {
        if(public_key[j] < 16)
            printf("0");
        printf("%hhX", public_key[j]);
    }
    printf("\nOf length: %u with data: %s \n", length, data);

    if(length != sizeof("Install Gentoo"))
    {
        return;
    }
    if(memcmp(data , "Install Gentoo", sizeof("Install Gentoo")) == 0 )
    //if the request contained the message of peace the person is obviously a friend so we add him.
    {
        printf("Friend request accepted.\n");
        m_addfriend_norequest(messenger, public_key);
    }
}


void print_message(Messenger *m, int friendnumber, char *string, uint16_t length)
{
    printf("Message with length %u recieved from %u: %s \n", length, friendnumber, string);
    m_sendmessage(messenger, friendnumber, "Test1", 5);
}


int main(int argc, char *argv[])
{
    if (argc < 4 && argc != 2) {
        printf("usage %s ip port public_key (of the DHT bootstrap node)\n or\n %s Save.bak\n", argv[0], argv[0]);
        exit(0);
    }
    messenger = initMessenger();
	if( ! messenger ){
		fputs("called to initMessenger failed in Messenger_test::main\n", stderr);
		return 1;
	}

    if(argc > 3)
    {
        IP_Port bootstrap_ip_port;
        bootstrap_ip_port.port = htons(atoi(argv[2]));
        bootstrap_ip_port.ip.i = inet_addr(argv[1]);
        DHT_bootstrap(bootstrap_ip_port, hex_string_to_bin(argv[3]));
    }
    else
    {
        FILE *file = fopen(argv[1], "rb");
        if ( file==NULL ){return 1;}
        int read;
        uint8_t buffer[128000];
        read = fread(buffer, 1, 128000, file);
        printf("Messenger loaded: %i\n", Messenger_load(messenger, buffer, read));
        fclose(file);

    }
    m_callback_friendrequest(messenger, print_request);
    m_callback_friendmessage(messenger, print_message);

    printf("OUR ID: ");
    uint32_t i;
    for(i = 0; i < 32; i++)
    {
        if(self_public_key[i] < 16)
            printf("0");
        printf("%hhX",self_public_key[i]);
    }

    setname(messenger, "Anon", 4);

    char temp_id[128];
    printf("\nEnter the client_id of the friend you wish to add (32 bytes HEX format):\n");
    if(scanf("%s", temp_id) != 1)
    {
        return 1;
    }
    int num = m_addfriend(messenger, hex_string_to_bin(temp_id), "Install Gentoo", strlen("Install Gentoo"));

    perror("Initialization");

    while(1)
    {
        char name[128];
        getname(messenger, num, name);
        printf("%s\n", name);

        m_sendmessage(messenger, num, "Test", 4);
        doMessenger(messenger);
        c_sleep(30);
        FILE *file = fopen("Save.bak", "wb");
        if ( file==NULL ){return 1;}
        uint8_t * buffer = calloc(1, Messenger_size(messenger));
        Messenger_save(messenger, buffer);
        fwrite(buffer, 1, Messenger_size(messenger), file);
        free(buffer);
        fclose(file);
    }

}
