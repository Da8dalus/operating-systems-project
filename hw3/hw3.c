#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>
#include <fcntl.h>
#include <setjmp.h>
#include <ctype.h>

extern int total_guesses;
extern int total_wins;
extern int total_losses;
extern char ** words;

char** dictionary;
int dicitonary_count;
pthread_mutex_t mutex;
int disconnect_signal;
jmp_buf env;


struct threadNeeded{
    char* target_word;
    int clientfd;
};

void disconnect(int signum){
    disconnect_signal = 1;
    longjmp(env, 1);
}


bool find(char* guess){
    for(int i = 0; i < dicitonary_count; i++){
        if(strcasecmp(*(dictionary + i), guess) == 0){
            return true;
        }
    }
    return false;
}

void * wordle_gameplay(void * arg){

    struct threadNeeded *details = (struct threadNeeded*)arg;
    char* target_word = calloc(5, sizeof(char));
    strcpy(target_word, details->target_word);
    int client_fd = details->clientfd;

    free(details);


    int attempts_left = 6;
    char * valid = calloc(1, sizeof(char));
    char * results = calloc(5, sizeof(char));
    short* guess_remainding = calloc(1, sizeof(short));
    char* copyHelp = calloc(5, sizeof(char));
    char*package = calloc(8, sizeof(char));
    char* buffer = calloc(6, sizeof(char));


    bool winner = false;
    while(attempts_left > 0 && disconnect_signal == 0){

        printf("Thread %lu: waiting for guess\n",pthread_self());

        if(disconnect_signal == 1){
            break;
        }

        // char* buffer = calloc(6, sizeof(char));

        int n = read(client_fd, buffer, 5);

        if(disconnect_signal == 1){
            break;
        }

        if (n < 0) {
            perror("ERROR: read() failed");
            free(buffer);
            free(target_word);
            free(copyHelp);
            free(package);
            free(valid);
            free(results);
            free(guess_remainding);
            
            close(client_fd);
            pthread_exit((void*) EXIT_FAILURE);
        }
        if(n == 0){
            printf("Thread %lu:  client gave up; closing TCP connection...\n",pthread_self());
            free(buffer);
            free(target_word);
            free(copyHelp);
            free(package);
            free(valid);
            free(results);
            free(guess_remainding);


            pthread_mutex_lock(&mutex);
            total_losses = total_losses + 1;
            pthread_mutex_unlock(&mutex);

            close(client_fd);
            pthread_exit(EXIT_SUCCESS);
        }

        *(buffer + 5) = '\0';

        if(disconnect_signal == 1){
            break;
        }


        printf("Thread %lu: rcvd guess: %s\n", pthread_self(), buffer);
    
        // char * valid = calloc(1, sizeof(char));
        // char * results = calloc(5, sizeof(char));
        // short* guess_remainding = calloc(1, sizeof(short));

        if(!find(buffer)){
            if(disconnect_signal == 1){
                break;
            }

            *(valid) = 'N';

            *(results + 0) = '?';
            *(results + 1) = '?';
            *(results + 2) = '?';
            *(results + 3) = '?';
            *(results + 4) = '?';

            *(guess_remainding) = (short)attempts_left;
        }else{

            if(disconnect_signal == 1){
                break;
            }

            *(valid) = 'Y';

            attempts_left = attempts_left - 1;
            *(guess_remainding) = (short)attempts_left;

            pthread_mutex_lock(&mutex);
            total_guesses = total_guesses + 1;
            pthread_mutex_unlock(&mutex);



            //see validity
            if(strcasecmp(buffer, target_word) == 0){
                strcpy(results, buffer);
                pthread_mutex_lock(&mutex);
                total_wins = total_wins + 1;
                pthread_mutex_unlock(&mutex);
                winner = true;
            }else{

                if(disconnect_signal == 1){
                    break;
                }

                // char* copyHelp = calloc(5, sizeof(char));
                strcpy(copyHelp, target_word);

                *(results + 0) = '-';
                *(results + 1) = '-';
                *(results + 2) = '-';
                *(results + 3) = '-';
                *(results + 4) = '-';
                
                if(disconnect_signal == 1){
                    break;
                }

                for(int i = 0; i < 5; i++){
                    if(tolower(*(copyHelp + i)) == tolower(*(buffer + i))){
                        *(results + i) = toupper(*(buffer + i));
                        *(copyHelp + i) = '-';
                        *(buffer + i) = '-';
                    }
                }

                for(int i = 0; i < 5; i++){
                    if(*(buffer + i) == '-'){
                        continue;
                    }
                    for(int j = 0; j < 5; j++){
                        if(tolower(*(buffer + i)) == tolower(*(copyHelp + j))){
                            *(results + i) = tolower(*(buffer + i));
                            *(copyHelp + j) = '-';
                            j = 5;
                        }
                    }
                }

                if(disconnect_signal == 1){
                    break;
                }
                
            }

            if(disconnect_signal == 1){
                break;
            }

            // char*package = calloc(8, sizeof(char));
            snprintf(package, 8, "%c%s%d", *(valid),results, *(guess_remainding));
            write(client_fd, package, 8);

            if(*(valid) == 'Y'){
                printf("Thread %lu: sending reply: %s (%d guesses left)\n", pthread_self(), results, *(guess_remainding));
            }else{
                printf("Thread %lu: invalid guess; sending reply: %s (%d guesses left)\n", pthread_self(), results, *(guess_remainding));
            }
            

            if(winner || disconnect_signal == 1){
                break;
            }
        }
    }

    free(buffer);
    free(copyHelp);
    free(package);
    free(valid);
    free(results);
    free(guess_remainding);



    if(disconnect_signal == 0){
        if(!winner){
            pthread_mutex_lock(&mutex);
            total_losses = total_losses + 1;
            pthread_mutex_unlock(&mutex);
            printf("THREAD %lu: game over!\n", pthread_self());
        }else{
            printf("THREAD %lu: game over! word was %s!\n", pthread_self(), target_word);
        }
    }

    free(target_word);
    close(client_fd);
    pthread_exit(EXIT_SUCCESS);

    
}

int wordle_server( int argc, char ** argv ){

    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);

    signal(SIGUSR1, disconnect);


    disconnect_signal = 0;

    if(argc != 5){
        perror("ERROR: Invalid argument(s)\nUSAGE: hw3.out <listener-port> <seed> <dictionary-filename> <num-words>");
        return EXIT_FAILURE;
    }

    /* Create the listener socket as TCP socket (SOCK_STREAM) */
    int listener = socket( AF_INET, SOCK_STREAM, 0 );
                                /* here, SOCK_STREAM indicates TCP */

    if ( listener == -1 ) { perror( "socket() failed" ); return EXIT_FAILURE; }

    /* populate the socket structure for bind() */
    struct sockaddr_in tcp_server;
    tcp_server.sin_family = AF_INET;   /* IPv4 */

    tcp_server.sin_addr.s_addr = htonl( INADDR_ANY );
    /* allow any remote IP address to connect to our socket */

    int  port = atoi( *(argv + 1) );  /* user provides port number */

    // printf("%d\n", port);
    tcp_server.sin_port = htons( port );

    if ( bind( listener, (struct sockaddr *)&tcp_server, sizeof( tcp_server ) ) == -1 ){
        perror( "bind() failed" );
        return EXIT_FAILURE;
    }

    /* identify our port number as a TCP listener port */
    if ( listen( listener, 5 ) == -1 ){
        perror( "listen() failed" );
        return EXIT_FAILURE;
    }

    printf( "SERVER: TCP listener socket (fd %d) bound to port %d\n", listener, port );

    //put dictionary into an array
    dictionary = calloc(atoi(*(argv + 4)), sizeof(char*));
    dicitonary_count = atoi(*(argv + 4));

    int fd = open(*(argv + 3), O_RDONLY);
    if (fd == -1){
        perror("open() failed");
        return EXIT_FAILURE;
    }

    printf("MAIN: opened %s (%s words)\n", *(argv + 3), *(argv + 4));

    char * output = (char*)malloc((5) * sizeof(char));
    int rc = 1;
    rc = read(fd, output, 5);
    int word_index = 0;
    while(rc > 0 && word_index < atoi(*(argv + 4))){
        *(dictionary + word_index) = calloc(5, sizeof(char));
        strcpy(*(dictionary + word_index), output);
        word_index++;
        lseek(fd, 1, SEEK_CUR);
        rc = read(fd, output, 5);
    }
    free(output);

    printf("MAIN: seeded pseudo-random number generator with %s\n", *(argv + 2));
    printf("MAIN: Wordle server listening on port {%s}\n",*(argv + 1));


    /* ==================== network setup code above ===================== */
    // create array of threads
    pthread_t* tids = calloc(1, sizeof(pthread_t));
    int target_word_count = 1;
    int tids_count = 0;
    while ( setjmp(env) == 0 ){
        struct sockaddr_in remote_client;
        int addrlen = sizeof( remote_client );

        printf( "SERVER: Blocked on accept()\n" );
        int newsd = accept( listener, (struct sockaddr *)&remote_client,
                            (socklen_t *)&addrlen );
        if ( newsd == -1 ) { 
            perror( "accept() failed" ); 
            return EXIT_FAILURE;
        }

        printf("MAIN: rcvd incoming connection request\n");
        printf( "SERVER: Accepted new client connection on newsd %d\n", newsd );

        // create hidden word
        char* target_word = calloc(5, sizeof(char));
        srand48(atoi(*(argv + 2)));

        int dictionary_index = rand() % (atoi(*(argv + 4))-1);
        strcpy(target_word, *(dictionary + dictionary_index));

        //add hidden word to words
        pthread_mutex_lock(&mutex);

        char** temp = realloc(words, (target_word_count + 1) * sizeof(char*));
        if (temp == NULL) {
            // Handle realloc failure
            pthread_mutex_unlock(&mutex);
            perror("realloc() failed");
            return EXIT_FAILURE;
        }
        words = temp;
        target_word_count++;
        *(words + target_word_count - 1) = calloc(5, sizeof(char));
        strcpy(*(words + target_word_count - 1), target_word);
        pthread_mutex_unlock(&mutex);
        
        //create thread and pass hidden word, dictionary, and fd
        struct threadNeeded *details = malloc(sizeof(struct threadNeeded));
        details->clientfd = newsd;
        details->target_word = target_word;

        pthread_t tid;
        
        int rc = pthread_create(&tid, NULL, wordle_gameplay, (void*)details);
        if ( rc != 0 ){
            perror ("MAIN: pthread_create() failed");
            return EXIT_FAILURE;
        }
        pthread_detach(tid);
        //put thread into array of threads
        tids = realloc(tids, (tids_count + 1) * sizeof(pthread_t));
        tids_count++;
        *(tids + tids_count - 1) = tid;
    }

    
    for(int i = 1; i < tids_count; i++){

		int * rc;
		if (pthread_join(*(tids + i), (void **)&rc) != 0) {
        	perror("pthread_join failed");
        	return EXIT_FAILURE;
    	}
    }

    free(tids);

    close(listener);
    for (int i = 0; i < dicitonary_count; i++) {
        free(*(dictionary + i));
    }
    free(dictionary);

    printf("MAIN: SIGUSR1 rcvd; Wordle server shutting down...\n");
    return EXIT_SUCCESS;


}




