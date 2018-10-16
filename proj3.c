//Chris Shorter
//cws68
//proj3

//This is the main file for project three

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#define ERROR 1

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

int main(int argc, char *argv[]){
	//booleans to record which flags are present
    bool port_present = false;        //-p is present, and is followed by a port number
	bool directory_present = false;		//-d is present, and a directory is specified
	bool auth_token_present = false;		//-a is present and is followed by an auth token
	
	//Strings for output file location and url
	char *directory = "";
	char *auth_token;
	
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
						directory = argv[i];
					else
						errexit("ERROR: Enter a directory following the -d flag", NULL);
					directory_present = true;
					break;
				case 'a':
					i++;
					if(argv[i] != NULL && argv[i][0] != '-')
						auth_token = argv[i];
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
	
	printf("%d\n%d\n%d\n%d\n", port_present, directory_present, auth_token_present, port);
	printf("%s\n%s\n", directory, auth_token);
	fflush(stdout);
	
}
