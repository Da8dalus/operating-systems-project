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
    // int* signalfd;
};

void disconnect(int signum){
    pthread_mutex_lock(&mutex);
    disconnect_signal = 1;
    pthread_mutex_unlock(&mutex);
    longjmp(env, 1);
    
}


bool find(char* guess){
    for(int i = 0; i < dicitonary_count; i++){
        pthread_mutex_lock(&mutex);
        // printf("%s\n", *(dictionary + i));
        if(strcasecmp(*(dictionary + i), guess) == 0){
            pthread_mutex_unlock(&mutex);
            return true;
        }
         pthread_mutex_unlock(&mutex);
    }
    return false;
}

void * wordle_gameplay(void * arg){

    struct threadNeeded *details = (struct threadNeeded*)arg;
    char* target_word = calloc(6, sizeof(char));
    strcpy(target_word, details->target_word);
    int client_fd = details->clientfd;

    free(details);


    short attempts_left = 6;
    
    char * results = calloc(6, sizeof(char));
    char* copyHelp = calloc(6, sizeof(char));
    char*package = calloc(8, sizeof(char));
    char* buffer = calloc(6, sizeof(char));


    bool winner = false;
    while(attempts_left > 0 && disconnect_signal == 0 && !winner){

        printf("THREAD %lu: waiting for guess\n",pthread_self());

        if(disconnect_signal == 1){
            break;
        }

        memset(buffer, 0, 6);
        memset(package, 0, 8);
        char valid;
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
            free(results);
            
            close(client_fd);
            pthread_exit(NULL);
        }
        if(n == 0){
            printf("THREAD %lu:  client gave up; closing TCP connection...\n",pthread_self());
            free(buffer);
            free(target_word);
            free(copyHelp);
            free(package);
            free(results);


            pthread_mutex_lock(&mutex);
            total_losses = total_losses + 1;
            pthread_mutex_unlock(&mutex);

            close(client_fd);
            pthread_exit(NULL);
        }

        *(buffer + 5) = '\0';



        if(disconnect_signal == 1){
            break;
        }


        printf("THREAD %lu: rcvd guess: %s\n", pthread_self(), buffer);
        

        if(!find(buffer)){
            if(disconnect_signal == 1){
                break;
            }

            valid = 'N';

            *(results + 0) = '?';
            *(results + 1) = '?';
            *(results + 2) = '?';
            *(results + 3) = '?';
            *(results + 4) = '?';
            *(results + 5) = '\0';

            results = realloc(results, 5 * sizeof(char));
            *(package) = valid;
            *(short *)(package + 1) = htons(attempts_left);
            *(char *)(package + 3) = *(results + 0);
            *(char *)(package + 4) = *(results + 1);
            *(char *)(package + 5) = *(results + 2);
            *(char *)(package + 6) = *(results + 3);
            *(char *)(package + 7) = *(results + 4);

            send(client_fd, package, 9,0);


        }else{

            if(disconnect_signal == 1){
                break;
            }

            valid = 'Y';

            attempts_left = attempts_left - 1;

            pthread_mutex_lock(&mutex);
            total_guesses = total_guesses + 1;
            pthread_mutex_unlock(&mutex);



            //see validity
            if(strcasecmp(buffer, target_word) == 0){
                *(results + 0) = toupper(*(buffer + 0));
                *(results + 1) = toupper(*(buffer + 1));
                *(results + 2) = toupper(*(buffer + 2));
                *(results + 3) = toupper(*(buffer + 3));
                *(results + 4) = toupper(*(buffer + 4));
                *(results + 5) = '\0';
                pthread_mutex_lock(&mutex);
                total_wins = total_wins + 1;
                pthread_mutex_unlock(&mutex);
                winner = true;
            }else{

                if(disconnect_signal == 1){
                    break;
                }

                strcpy(copyHelp, target_word);
                // printf("%s : %s\n", copyHelp, target_word);

                *(results + 0) = '-';
                *(results + 1) = '-';
                *(results + 2) = '-';
                *(results + 3) = '-';
                *(results + 4) = '-';
                *(results + 5) = '\0';
                
                if(disconnect_signal == 1){
                    break;
                }

                for(int i = 0; i < 5; i++){
                    // printf("%c : %c\n",*(copyHelp + i),*(buffer + i));
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
                
            }

            if(disconnect_signal == 1){
                break;
            }
    
            
        

            results = realloc(results, 5 * sizeof(char));

            // unsigned short attempts_encoded = htons(attempts_left);
            *(package) = valid;
            *(short *)(package + 1) = htons(attempts_left);
            *(char *)(package + 3) = *(results + 0);
            *(char *)(package + 4) = *(results + 1);
            *(char *)(package + 5) = *(results + 2);
            *(char *)(package + 6) = *(results + 3);
            *(char *)(package + 7) = *(results + 4);
            
            send(client_fd, package, 9,0);
        }

    
        

        if(valid == 'Y'){
            if(attempts_left == 1){
                printf("THREAD %lu: sending reply: %s (%01X guess left)\n", pthread_self(), results, attempts_left);
            }else{
                printf("THREAD %lu: sending reply: %s (%01X guesses left)\n", pthread_self(), results, attempts_left);
            }
            
        }else{
            if(attempts_left == 1){
                printf("THREAD %lu: invalid guess; sending reply: %s (%01X guess left)\n", pthread_self(), results, attempts_left);
            }else{
                printf("THREAD %lu: invalid guess; sending reply: %s (%01X guesses left)\n", pthread_self(), results, attempts_left);
            }
        }
        
        
    }
    

    free(buffer);
    free(copyHelp);
    free(package);
    free(results);



    if(disconnect_signal == 0){
        if(!winner){
            pthread_mutex_lock(&mutex);
            total_losses = total_losses + 1;
            pthread_mutex_unlock(&mutex);
        }
        printf("THREAD %lu: game over! word was %s!\n", pthread_self(), target_word);
        
    }

    free(target_word);
    close(client_fd);
    pthread_exit(NULL);

    
}

int wordle_server( int argc, char ** argv ){
    setvbuf( stdout, NULL, _IONBF, 0 );

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

    //put dictionary into an array
    dictionary = calloc(atoi(*(argv + 4)), sizeof(char*));
    dicitonary_count = atoi(*(argv + 4));

    int fd = open(*(argv + 3), O_RDONLY);
    if (fd == -1){
        free(dictionary);
        perror("open() failed");
        return EXIT_FAILURE;
    }

    printf("MAIN: opened %s (%s words)\n", *(argv + 3), *(argv + 4));
    
    char * output = calloc(7, sizeof(char));
    int rc = 0;
    rc = read(fd, output, 7);
    if(rc == -1){
        free(output);
        free(dictionary);
        perror("read) failed");
        return EXIT_FAILURE;  
    }
    
    int word_index = 0;
    while(rc > 0 && word_index < atoi(*(argv + 4))){
        output = realloc(output, 6 * sizeof(char));
        *(dictionary + word_index) = calloc(6, sizeof(char));
        strncpy(*(dictionary + word_index), output, 5);
        word_index++;
        free(output);
        output = calloc(7, sizeof(char));
        rc = read(fd, output, 7);
        if(rc == -1){
            free(output);
            for (int i = 0; i < dicitonary_count; i++) {
                free(*(dictionary + i));
            }
            free(dictionary);
            perror("read) failed");
            return EXIT_FAILURE;
        }
    
    }

    free(output);

    printf("MAIN: seeded pseudo-random number generator with %s\n", *(argv + 2));
    printf("MAIN: Wordle server listening on port {%s}\n",*(argv + 1));


    /* ==================== network setup code above ===================== */
    // create array of threads
    int target_word_count = 1;
    bool run;

    if(setjmp(env) == 0 ){
        run = true;
    }

    while (run){

        if(setjmp(env) != 0){
            break;
        }


        struct sockaddr_in remote_client;
        int addrlen = sizeof( remote_client );

        int newsd = accept( listener, (struct sockaddr *)&remote_client,
                            (socklen_t *)&addrlen );
        if ( newsd == -1 ) { 
            for (int i = 0; i < dicitonary_count; i++) {
                 free(*(dictionary + i));
            }
            free(dictionary);
            perror( "accept() failed" ); 
            continue;
        }


        printf("MAIN: rcvd incoming connection request\n");

        // create hidden word
        char* target_word = calloc(6, sizeof(char));
        srand48(atoi(*(argv + 2)));
        
        int dictionary_index = rand() % (atoi(*(argv + 4))-1);
        strcpy(target_word, *(dictionary + dictionary_index));
        *(target_word + 5) = '\0';

        // //add hidden word to words
        pthread_mutex_lock(&mutex);

        char** temp = realloc(words, (target_word_count + 1) * sizeof(char*));
        if (temp == NULL) {
            // Handle realloc failure
            for (int i = 0; i < dicitonary_count; i++) {
                 free(*(dictionary + i));
            }
            free(temp);
            free(dictionary);
            free(target_word);
            perror("realloc() failed");
            continue;
        }
        
        words = temp;
        target_word_count++;
        *(words + target_word_count - 1) = calloc(6, sizeof(char));
        strcpy(*(words + target_word_count - 1), target_word);
        pthread_mutex_unlock(&mutex);
        
        //create thread and pass hidden word, clientfd
        struct threadNeeded *details = malloc(sizeof(struct threadNeeded));
        details->clientfd = newsd;
        details->target_word = target_word;

        pthread_t tid;
        
        rc = pthread_create(&tid, NULL, wordle_gameplay, (void*)details);
        if ( rc != 0 ){
            for (int i = 0; i < dicitonary_count; i++) {
                 free(*(dictionary + i));
            }
            free(dictionary);
            free(details);
            free(target_word);
            perror ("MAIN: pthread_create() failed");
            continue;
        }
        pthread_detach(tid);
        
        // free(target_word);
        if(setjmp(env) == 0 ){
            run = true;
        }else{
            run = false;
        }
    }

    //end server
    
    close(listener);

    for (int i = 0; i < dicitonary_count; i++) {
        free(*(dictionary + i));
    }
    free(dictionary);

    printf("MAIN: SIGUSR1 rcvd; Wordle server shutting down...\n");
    return EXIT_SUCCESS;


}




