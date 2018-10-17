//Chris Shorter
//cws68
//proj3

//This is the main file for project three

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define ERROR 1
#define PROTOCOL "tcp"
#define QLEN 1
#define BUFLEN 1024
#define END_LINE "\0"
#define METHOD_POS 0
#define FILE_POS 1
#define VERSION_POS 2
#define REQUEST_ARRAY_LEN 3
#define HTTP_START "HTTP/"


//If there's any sort of error the program exits immediately.
int errexit (char *format, char *arg){
    fprintf (stdout,format,arg);
    fprintf (stdout,"\n");
    exit (ERROR);
}

//Just takes a string and turns it into an integer. But unlike the stupid atoi() method, it returns -1 for non-integers
int getPortFromString(char *port_string){
	int port = 0;
	int place = 1;
	int digit = 0;
	bool valid = true;
	
    int i=strlen(port_string)-1;
	for(; i>-1; i--){
		digit = ((int) port_string[i])-48;
		if(digit > 9 || digit < 0){
			valid = false;
		}
		else{
		port += place*digit;
		place = place*10;
		}
	}
	if(valid && port>1024 && port<65535)
		return port;
	else
		return -1;
}

//Does the socket setup steps and returns a socket descriptor
int setupSocket(int port){
	struct sockaddr_in sin;
    struct protoent *protoinfo;
	
	/* determine protocol */
    if ((protoinfo = getprotobyname (PROTOCOL)) == NULL)
        errexit ("cannot find protocol information for %s", PROTOCOL);
	
    memset ((char *)&sin,0x0,sizeof (sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);
	
    /* allocate a socket */
    /*   would be SOCK_DGRAM for UDP */
    int sd = socket(PF_INET, SOCK_STREAM, protoinfo->p_proto);
    if (sd < 0)
        errexit("cannot create socket", NULL);

    /* bind the socket */
    if (bind (sd, (struct sockaddr *)&sin, sizeof(sin)) < 0){
        errexit ("cannot bind to port", NULL);
	}

    /* listen for incoming connections */
    if(listen (sd, QLEN) < 0){
    	errexit ("cannot listen on port", NULL);
	}
	
	return sd;
}

//Does the accepting and connection to the client and returns another socket descriptor
int setupConnection(int sd){
    struct sockaddr addr;
	
	socklen_t addrlen = sizeof(addr); // Address struct length
	int socket_connection = accept (sd,&addr,&addrlen);
    if (socket_connection < 0){
        errexit ("error accepting connection", NULL);
	}
	return socket_connection;
}

//Reads first line from socket and checks rest for validity. Returns "invalid" if malformed
char* readRequestFromSocket(int sd){
	char buffer [BUFLEN];
	bzero(buffer, BUFLEN);
    int ret = read (sd,buffer,BUFLEN - 1);
    if (ret < 0){
        errexit ("reading error",NULL);
	}
	if(strstr(buffer, "\r\n\r\n") == NULL)
		return "invalid";
	
	char line[sizeof(buffer)];
	char* return_value = malloc(ret);
	int buffer_index = 0;
	int line_index = 0;
	
	while(buffer_index < ret){
		bzero(line, sizeof(line));
		int character_index = 0;
		while(buffer[buffer_index] != '\n' && buffer_index < ret){
			line[character_index] = buffer[buffer_index];
			buffer_index++;
			character_index++;
		}
		if(buffer[buffer_index] == '\n' && buffer[buffer_index-1] != '\r'){
			return "invalid";
		}
		line[character_index] = '\0';
		if(line_index == 0){
			sprintf(return_value, "%s", line);
		}
		line_index++;
		buffer_index++;
	}
	return return_value;
}

//Takes a code and an optional char* for a file and writes the response to the socket
//Is NOT to be used for responses that include files
void writeResponseToSocket(int sd, int code){
	char* response;
	switch(code){
		case 400:
			response = "HTTP/1.1 400 Malformed Request\r\n\r\n";
			break;
		case 501:
			response =  "HTTP/1.1 501 Protocol Not Implemented\r\n\r\n";
			break;
		case 405:
			response = "HTTP/1.1 405 Unsupported Mehtod\r\n\r\n";
			break;
		case 403:
			response =  "HTTP/1.1 403 Operation Forbidden\r\n\r\n";
			break;
		case 406:
			response =  "HTTP/1.1 406 Invalid Filename\r\n\r\n";
			break;
		case 404:
			response =  "HTTP/1.1 404 File Not Found\r\n\r\n";
			break;
		case 200:
			response =  "HTTP/1.1 200 Server Shutting Down\r\n\r\n";
			break;
		default:
			errexit("ERROR: Invalid Response Code Used", NULL);
			break;
	}
	if (write (sd, response, strlen (response)) < 0){
        errexit ("error writing message: %s", NULL);
    if(close (sd)<0)
        errexit("ERROR: Error closing socket", NULL);
    }
}	

int fileToSocket(int sd, char* filename){
    FILE* file = fopen(filename, "r");
    if(file == NULL)
        return -1;
    
    char* response =  "HTTP/1.1 200 OK\r\n\r\n";
    int size = strlen(response);
    if (write (sd, response, size) < 0){
       	 	errexit ("error writing response", NULL); 
    }
    unsigned char buffer[BUFLEN];
    bzero(buffer, BUFLEN);
    int total = 0;
    bool done = false;
    while(!done){
        bzero(buffer, BUFLEN);
        
        int i=0;
        int current = fgetc(file);
        while(i<BUFLEN-1 && current > 0){
            buffer[i] = (unsigned char) current;
            current = fgetc(file);
            i++;
        }
        total += i;
        if(current < 0){
            done = true;
        }
        if (write (sd, buffer, i) < 0){
            errexit ("error writing content", NULL); 
        }
    }
    fclose(file);
    return total;
}


int main(int argc, char *argv[]){
	//booleans to record which flags are present
    bool port_present = false;        //-p is present, and is followed by a port number
	bool directory_present = false;		//-d is present, and a directory is specified
	bool auth_token_present = false;		//-a is present and is followed by an auth token
	
	//Strings for output file location and url
	char *root = "";
	char *server_token;
	
	int port = 0;
	
	int i=1;
	for(; argv[i] != NULL; i++){
		if(argv[i][0] == '-'){
			switch(argv[i][1]){
                case 'p':
					i++;
					port = getPortFromString(argv[i]);
					if(port == -1)
						errexit("ERROR: Invalid port", NULL);
					port_present = true;
					break;
				case 'd':
					i++;
					if(argv[i] != NULL && argv[i][0] != '-')
						root = argv[i];
					else
						errexit("ERROR: Enter a directory following the -d flag", NULL);
					directory_present = true;
					break;
				case 'a':
					i++;
					if(argv[i] != NULL && argv[i][0] != '-')
						server_token = argv[i];
					else
						errexit("ERROR: Enter an auth token following the -a flag", NULL);
					auth_token_present = true;
					break;
				default:
					errexit("ERROR: Invalid flag. Valid flags are -p, -d, and -a", NULL);
					break;
			}
            
		}
        else{
            errexit("ERROR: Either no flags were entered or an invalid argument was passed.", NULL);
		}
	}
	if(!port_present)
		printf("ERROR: -p flag and port # must be included\n");
	if(!directory_present)
		printf("ERROR: -d flag and directory must be included\n");
	if(!auth_token_present)
		printf("ERROR: -p flag and port # must be included\n");
	if(!(port_present && directory_present && auth_token_present))
		errexit("ERROR: requird flags and fields were not entered" , NULL);
	
	//Now that local stuff has been processed, set up to receive and interpret request
	
	//booleans for errors
	bool valid_request = true;
	bool malformed = false;
	bool unimplemented_protocol = false;
	bool unsupported_method = false;
    
    //booleans for method type
    bool get = false;
    bool quit = false;
	
	int socket_server = setupSocket(port);
    
    bool done = false;
    
    while(!done){
	    int socket_connection = setupConnection(socket_server);
        
        char* request = readRequestFromSocket(socket_connection);

        char* method;
        char* filename;
        char* client_token;
        char* version;
        if(strcmp(request, "invalid") == 0){
            malformed = true;
            valid_request = false;
        }
        else{
            printf("%s\n", request);
            fflush(stdout);
            char* request_array[REQUEST_ARRAY_LEN];
            char* token = malloc(sizeof(request));
            int token_len = sizeof(token);
            int array_index = 0;
            int request_index = 0;
            while(request[i] != '\r' && array_index<REQUEST_ARRAY_LEN){
                int token_index = 0;
                while(request[request_index] != '\r' && request[request_index] != ' '){
                    token[token_index] = request[request_index];
                    request_index++;
                    token_index++;
                }
                token[token_index] = '\0';
                request_array[array_index] = malloc(sizeof(token));
                sprintf(request_array[array_index], "%s", token);
                bzero(token, token_len);
                array_index++;
                request_index++;
            }
            

            method = request_array[METHOD_POS];
            filename = request_array[FILE_POS];
            client_token = request_array[FILE_POS];
            version = request_array[VERSION_POS];
            
            if(strncasecmp(version, HTTP_START, strlen(HTTP_START)) != 0){
                valid_request = false;
                unimplemented_protocol = true;
            }

            if(strcmp("GET", method) == 0){
                get = true;
            }
            else{
                if(strcmp("QUIT", method) == 0){
                    quit = true;
                }
                else{
                    valid_request = false;
                    unsupported_method = true;
                }
            }
        }

        if(malformed){
            writeResponseToSocket(socket_connection, 400);
        }
        else{
            if(unimplemented_protocol){
                writeResponseToSocket(socket_connection, 501);
            }
            else{
                if(unsupported_method){
                    writeResponseToSocket(socket_connection, 405);
                }
            }
        }

        if(valid_request){
            if(get){
                int path_size = strlen(filename)+strlen(root);
                char full_path[path_size];
                if(filename[0] != '/'){
                    writeResponseToSocket(socket_connection, 406);
                }
                else{
                    if(strcmp(filename, "/") == 0)
                        sprintf(full_path, "//%s/default.html", root);
                    else
                        sprintf(full_path, "//%s%s", root, filename);
                    if(fileToSocket(socket_connection, full_path)<0)
                        writeResponseToSocket(socket_connection, 404);
                }
            }
            else{
                if(quit){
                    if(strcmp(server_token, client_token)==0){
                        writeResponseToSocket(socket_connection, 200);
                        printf("Client Auth Successful, Server Shutting Down...\n");
                        fflush(stdout);
                        exit(0);
                    }
                    else{
                        writeResponseToSocket(socket_connection, 403);
                    }
                }
            }
        }
    }
    /* close & exit */
    if(close (socket_server)<0)
		errexit("ERROR: Error closing socket", NULL);
}
