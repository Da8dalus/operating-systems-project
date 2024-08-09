
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <stdbool.h>

extern int total_guesses;
extern int total_wins;
extern int total_losses;
extern char ** words;

char ** dictionary;
int num_words;
int num_targets;

pthread_mutex_t mutex;
pthread_t * client_threads;
int num_threads;
int * client_newsds;
bool run_server = true;


int validity(char * guess, int pos, char * target_word){
	// if same; return 0
	if( *(guess + pos) == *(target_word + pos) ){
		return 0;
	}

	int wrongLetter = 0, wrongGuess = 0, i;
	for( i = 0; i < 5; i++ ) {
		if( *(guess + pos) == *(target_word + i) && *(guess + i) != *(guess + pos)){
			wrongLetter++;
		}
		if (i <= pos) {
			if( *(guess + i) == *(guess + pos) && *(target_word + i) != *(guess + pos) )
				wrongGuess++;
		}
		if(i >= pos) {
			if(wrongGuess == 0)
				break;
			if(wrongGuess <= wrongLetter)
				return 1;
		}
	}
	
	return 2;
}


void signal_ender( int sig ) {
	run_server = false;

	printf("MAIN: SIGUSR1 rcvd; Wordle server shutting down...\n");
	printf("MAIN:MAIN: valid guesses: %d\n", total_guesses);
	printf("MAIN:MAIN: win/loss: %d/%d\n", total_wins, total_losses);
	for (int i = 0; i < num_targets; i++ ){
		printf("MAIN:MAIN: word #%d: %s\n", i+1, *(words + i));
	}
}

void * wordle_gameplay( void * arg ){
    // printf("are yu here\n");

	char * target_word = calloc(6, sizeof(char));
   
    //get target word and put into words array
    // printf("num words : %d\n", num_words);
	int random_index = rand() % num_words;
    
	strcpy(target_word, *(dictionary+random_index));
	
	words = realloc( words, sizeof(char *) * (num_targets + 2) );
	*(words + num_targets) = calloc(6, sizeof(char));
	strcpy(*(words + num_targets), target_word);
    // printf("are yu here\n");
	
	for( int i = 0; i < 6; i++ ){
		*(*(words + num_targets)+i) = toupper( *(*(words + num_targets)+i) );
    }
	num_targets++;

    //start actual game
	int * newsock_fd = (int *)arg;
	int * rc = calloc( 1, sizeof( int ) );
	char * guess = calloc(6, sizeof(char));
	int guesses = 0;

    // printf("do we get here?\n");

    //start the actual game
	while(1){
        // printf("or do we get here?\n");
		printf("THREAD %lu: waiting for guess\n", pthread_self());

		int recv_rc = recv( *newsock_fd, guess, 5, MSG_WAITALL );
		if (recv_rc == 0) {
			// Client has closingthe connection
			printf("THREAD %lu: client gave up; closing TCP connection...\n", pthread_self());
			total_losses++;
			break;
		} 
		else if( recv_rc == -1) {
			fprintf(stderr, "ERROR: failed to recieve from client\n");
			pthread_exit( NULL );
		}
		printf("THREAD %lu: rcvd guess: %s\n", pthread_self(), guess);
		// Lock mutex until guess is processed
		pthread_mutex_lock(&mutex);

		char * copy = calloc(6, sizeof(char));
		void * package = calloc(9, sizeof(char)); // 1 byte

		//Check if valid guess
        bool found = false;
		for( int i = 0; i < num_words; i++ ){
			if( strcmp(*(dictionary + i), guess) == 0) {
                found = true;
				break;
			}
		}
        


        //Invalid word
		if(!found){
            // printf("not found\n");
			printf("THREAD %lu: invalid guess; sending reply: ????? (%d guess%s left)\n", pthread_self(), 6 - guesses, guesses == 5 ? "" : "es");
			memcpy( package, "N", 1);
			*(short*)( package + 1 ) = htons(6 - guesses);
			memcpy( package + 3, "?????", 5);

			send( *newsock_fd, package, 9, 0 );
		}else{//valid word
        // printf("found");
			total_guesses++;
			guesses++;
			memcpy( package, "Y", 1);
			*(short*)( package + 1 ) = htons(6 - guesses);


            //is guess target?
			if( !strcmp(guess, target_word)){
                for( int i = 0; i < 5; i++ ){
                    *(copy + i) = toupper( *(guess+i) );
                }
            }

            
            //guess not target -> how much right?
            for(int i = 0; i < 5; i++) {
                int c = validity(guess, i, target_word);
                if( c == 0 ){
                    *(copy + i) = toupper( *(guess+i) );
                } else if( c == 1 ){
                    *(copy + i) = tolower( *(guess+i) );
                } else if ( c == 2 ){
                    *(copy + i) = '-';
                }
            }

			memcpy( package + 3, copy, 5);

			printf("THREAD %lu: sending reply: %s (%d guess%s left)\n", pthread_self(), copy, 6 - guesses, guesses == 5 ? "" : "es");
			send( *newsock_fd, package, 9, 0 );
		}
		// Guess is processed; unlock mutex
		pthread_mutex_unlock(&mutex);

		free( copy );
		free( package );

		if( strcmp(guess, target_word) == 0){
			total_wins++;
			break;
		}
		if( guesses == 6 ){
			total_losses++;
			break;
		}
	}
	
	for( int i = 0; i < 5; i++ ){
		*(target_word + i) = toupper( *(target_word+i) );
    }
	printf("THREAD %lu: game over; word was %s!\n", pthread_self(), target_word);
	
	free(guess);
	free(target_word);
	close(*newsock_fd);

	pthread_exit( rc );
}


int wordle_server( int argc, char ** argv ){


    // Set up signal routine
	signal( SIGUSR1, signal_ender );
	signal( SIGINT, SIG_IGN );
	signal( SIGTERM, SIG_IGN );
	signal( SIGUSR2, SIG_IGN );

    total_guesses = 0;
	total_wins = 0;
	total_losses = 0;
	num_targets = 0;

	if(argc != 5){
		fprintf(stderr, "ERROR: Invalid argument(s)\n");
		fprintf(stderr, "USAGE: hw3.out <listener-port> <seed> <dictionary-filename> <num-words>\n");
        return EXIT_FAILURE;
	}

	setvbuf( stdout, NULL, _IONBF, 0);



    //create and read in dicitonary
	int fd = open(*(argv+3), O_RDONLY);
	dictionary = calloc(atoi(*(argv+4)), sizeof(char*));
    num_words = atoi(*(argv+4));
	for( int i = 0; i < atoi(*(argv+4)); i++ ) {
		*(dictionary + i) = calloc(6, sizeof(char));
		read(fd, *(dictionary + i), 5);
		lseek(fd, 1, SEEK_CUR);
	}


    // for( int i = 0; i < atoi(*(argv+4)); i++ ) {
	// 	printf("%s\n",*(dictionary + i));
    // }
    

	printf("MAIN: opened %s (%d words)\n",  *(argv+3),  atoi(*(argv+4)));

	int seed = atoi( *(argv+2) );
	srand( seed );
	printf("MAIN: seeded pseudo-random number generator with %d\n", seed);

	
	
    //Build server

	    // Create socket
    
    struct sockaddr_in my_addr, client_addr;
	int listener = socket(AF_INET, SOCK_STREAM, 0);
	if(listener == -1){
		perror("socket() failed");
		return EXIT_FAILURE;
	}

	    // Bind to a port number
	short port = atoi(*(argv+1));

	memset(&my_addr, 0, sizeof(struct sockaddr_in));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);

	if( bind(listener, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_in)) < 0){
		perror("bind() failed");
        for( int i = 0; i < atoi(*(argv+4)); i++ ) {
            if(*(dictionary + i) == NULL){
                continue;
            }
            free(*(dictionary + i));
        }
        free(dictionary);
		return EXIT_FAILURE;
	}

	// Initialize the mutex
	pthread_mutex_init(&mutex, NULL);

	// pthread_t array to keep threads
	num_threads = 0;
	client_threads = calloc(128, sizeof(pthread_t));
	client_newsds = calloc(128, sizeof(int));
	
	if(listen(listener, 5) == -1){
		perror("listen() failed\n");
        for( int i = 0; i < atoi(*(argv+4)); i++ ) {
            if(*(dictionary + i) == NULL){
                continue;
            }
            free(*(dictionary + i));
        }
        free(dictionary);
        free(client_threads);
        free(client_newsds);

		return EXIT_FAILURE;
	}

	printf("MAIN: Wordle server listening on port {%d}\n", port);

	while(run_server){
		//Set up connection request
		socklen_t client_len = sizeof(client_addr);
		int newsfd = accept(listener, (struct sockaddr *)&client_addr, &client_len);
		printf("MAIN: rcvd incoming connection request\n");

		*(client_newsds + num_threads) = newsfd;

		if(newsfd == -1){
			fprintf(stderr, "ERROR: failed to accept connection\n");
			return EXIT_FAILURE;
		}

        // printf("before making thread\n");
		// Create thread
		if( pthread_create((client_threads+num_threads), NULL, wordle_gameplay, client_newsds + num_threads) ){
			fprintf(stderr, "ERROR: failed to create thread\n");
            for( int i = 0; i < atoi(*(argv+4)); i++ ) {
                if(*(dictionary + i) == NULL){
                    continue;
                }
                free(*(dictionary + i));
            }
            free(dictionary);
            free(client_threads);
            free(client_newsds);
			return EXIT_FAILURE;
		}
        // printf("before detaching\n");
		pthread_detach(*(client_threads+num_threads));	
		num_threads++;
        // printf("get to end\n");
	}

	//Close the connection
	close(listener);
    //free allocations
	for( int i = 0; i < num_words; i++ ){
         free(*(dictionary + i));
    }
	for(int i = 0; i < num_threads; i++ ){
        close(*(client_newsds + i));
    } 

    
	pthread_mutex_destroy(&mutex);
	for( int i = 0; i < atoi(*(argv+4)); i++ ) {
        if(*(dictionary + i) == NULL){
            continue;
        }
        free(*(dictionary + i));
    }
    free(dictionary);
    free(client_threads);
    free(client_newsds);

	return EXIT_SUCCESS;
}





