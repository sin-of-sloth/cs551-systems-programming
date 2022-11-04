/********************************************************************
*   THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING     *
*   A TUTOR OR CODE WRITTEN BY OTHER STUDENTS                       *
*                               - ARJUN LAL                         *
********************************************************************/



#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>


int main(int argc, char *argv[]) {
    int opt; // optarg
    int count = 1; // number of sorters; default 1
    int min_length; // words must have more than min_length letters
    int max_length; // max_length is the max number of letters in a word
    int i, j; // generic counter
    int status; // childs return status
    int whom; // pid of dead child
    char c, prev = '\0';

    while ((opt = getopt(argc, argv, "n:s:l:")) != -1) {
        switch(opt) {
            case 'n':
                count = (int) strtol(optarg, NULL, 10);
                continue;
            case 's':
                min_length = (int) strtol(optarg, NULL, 10);
                continue;
            case 'l':
                max_length = (int) strtol(optarg, NULL, 10);
                continue;
            default:
                fprintf(stderr, "Usage: pipesort [n:s:l:]");
                exit(1);
        }
    }

    if(min_length > max_length) {
        fprintf(stderr, "short > long");
        exit(1);
    }

    // allocate two arrays of pipes - one to parse, and one from merge
    int **parse_pipefds = malloc(count * sizeof(int *));
    int **merge_pipefds = malloc(count * sizeof(int *));
    for(i = 0; i < count; i++) {
        parse_pipefds[i] = malloc(2 * sizeof(int));
        merge_pipefds[i] = malloc(2 * sizeof(int));
    }

    // array of file pointers to store pipes as streams
    FILE **fps = malloc(count * sizeof(FILE *));

    // allocate array to store child pids
    int *pid = malloc(count * sizeof(int));
    int parser_pid; // pid of parser
    int merger_pid; // pid of merger

    // initialize pipes
    for(i = 0; i < count; i++) {
        if(pipe(parse_pipefds[i]) == -1) {
            perror("pipe");
            exit(1);
        }
        if(pipe(merge_pipefds[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }

    // fork children and set them up to
    // execute sort with pipes as stdin and stdout
    for(i = 0; i < count; i++) {
        if((pid[i] = fork()) == -1) {
            perror("fork");
            exit(1);
        }

        // child code
        if(pid[i] == 0) {
            // close unused pipe ends
            for(j = 0; j < count; j++) {
                if (j != i) {
                    close(parse_pipefds[j][0]);
                    close(parse_pipefds[j][1]);
                    close(merge_pipefds[j][0]);
                    close(merge_pipefds[j][1]);
                }
            }
            close(parse_pipefds[i][1]);
            close(merge_pipefds[i][0]);

            // set stdout and stdin as pipes
            dup2(parse_pipefds[i][0], 0);
            close(parse_pipefds[i][0]);
            dup2(merge_pipefds[i][1], 1);
            close(merge_pipefds[i][1]);

            // exec /usr/bin/sort
            execl("/usr/bin/sort", "sort", NULL);
        }
        // parent code
        else {
            // close unused pipe ends
            close(parse_pipefds[i][0]);
            close(merge_pipefds[i][1]);
        }
    }

    // fork a child for parsing
    if((parser_pid = fork()) == -1) {
        perror("fork");
        exit(1);
    }

    if(parser_pid == 0) { //child for parsing
        int curr_length = 0; // keeps track of length of a word
        int curr_pipe = 0; // keeps track of which pipe to send chars to

        // open pipes as streams
        for(i = 0; i < count; i++) {
            fps[i] = fdopen(parse_pipefds[i][1], "w");
        }
        // loop and send read chars to the pipes
        while((int) (c = fgetc(stdin)) != EOF) {
            // if not alphabetic and previous char was an alphabet, send newline
            // and move to next pipe
            if(!isalpha(c)) {
                if(isalpha(prev)) {
                    c = '\n';
                    fputc(c, fps[curr_pipe]);
                    curr_length = 0;
                    curr_pipe = (curr_pipe + 1) % count;
                }
            }
            else {
                c = tolower(c);
                // if we've not already written max_length chars, write to pipe
                if(curr_length < max_length) {
                    fputc(c, fps[curr_pipe]);
                    curr_length += 1;
                }
            }
            prev = c;
        }

        // close parser write pipes in child
        for(i = 0; i < count; i++) {
            fclose(fps[i]);
            close(parse_pipefds[i][1]);
        }

        // free allocated stuff
        for(i = 0; i < count; i++) {
            free(parse_pipefds[i]);
            free(merge_pipefds[i]);
        }
        free(parse_pipefds);
        free(merge_pipefds);
        free(pid);
        free(fps);

        // parsing done, child needs to die
        exit(0);
    }

    // close parser write pipes in parent
    for(i = 0; i < count; i++) {
        close(parse_pipefds[i][1]);
    }

    // wait for parser child to die
    if((whom = waitpid(parser_pid, &status, 0)) != parser_pid) {
        perror("waitpid");
        exit(status);
    }

    // fork a child for merging
    if((merger_pid = fork()) == -1) {
        perror("fork");
        exit(1);
    }

    if(merger_pid == 0) { //child for merging
        // array of words to store words from pipes
        char **words = malloc(count * sizeof(char *));
        // holds current first sorted word
        char *word = malloc((max_length + 1) * sizeof(char));
        int word_count; // number of occurences of a word
        int cmp; //stores string comparison result

        // malloc words and open streams
        for(i = 0; i < count; i++) {
            words[i] = malloc((max_length + 1) * sizeof(char));
            fps[i] = fdopen(merge_pipefds[i][0], "r");

            // read one word from each pipe, ignore smaller words
            do {
                fgets(words[i], max_length + 1, fps[i]);
                if(
                    !words[i] || strcmp(words[i], "") == 0 || !isalpha(words[i][0]) || feof(fps[i])
                ) {
                    strcpy(words[i], "\0");
                    break;
                }
                else {
                    // strip newline
                    words[i][strcspn(words[i], "\r\n")] = 0;
                }
            } while(strlen(words[i]) <= min_length);
        }

        while(1) {
            // check if all words are empty -
            // means there's nothing to read from any pipes
            int empty = 1;
            for(i = 0; i < count; i++) {
                if(strcmp(words[i], "\0") != 0){
                    empty = 0;
                    break;
                }
            }
            if(empty)
                break;

            // let first pipe's word be first word in sorted order
            strcpy(word, words[i]);
            for(i = 1; i < count; i++) {
                // if word is empty string, nothing to read from pipe `i`
                if(strcmp(words[i], "\0") == 0) {
                    continue;
                }
                cmp = strcmp(words[i], word);
                // if `words[j]` comes before `word`, let it be new `word`
                if(cmp < 0) {
                    strcpy(word, words[i]);
                }
            }

            // now, `word` contains first word in sorted order
            // count duplicates of the word from all pipes
            word_count = 0;
            for(i = 0; i < count; i++) {
                if(strcmp(word, words[i]) == 0) {
                    word_count += 1;
                    // read from pipe until next word from pipe is not `word`
                    while(1) {

                        // read one word from each pipe, ignore smaller words
                        do {
                            fgets(words[i], max_length + 1, fps[i]);

                            if(
                                !words[i] || strcmp(words[i], "") == 0 || !isalpha(words[i][0]) || feof(fps[i])
                            ) {
                                strcpy(words[i], "\0");
                                if(feof(fps[i]))
                                    break;
                            }
                            else {
                                // strip newline
                                words[i][strcspn(words[i], "\r\n")] = 0;
                            }
                        } while(strlen(words[i]) <= min_length);

                        // see if `words[i]` is `word`
                        if(strcmp(word, words[i]) == 0) {
                            word_count += 1;
                        }
                        else {
                            break;
                        }
                    }
                }
            }

            printf("%-10d%s\n", word_count, word);
        }

        // close merger read pipes and stream in child
        for(i = 0; i < count; i++) {
            fclose(fps[i]);
            close(merge_pipefds[i][0]);
        }

        // free allocated stuff
        for(i = 0; i < count; i++) {
            free(parse_pipefds[i]);
            free(merge_pipefds[i]);
            free(words[i]);
        }
        free(parse_pipefds);
        free(merge_pipefds);
        free(pid);
        free(fps);
        free(words);
        free(word);

        // merging done, child needs to die
        exit(0);
    }

    // close merger read pipes and stream in parent
    for(i = 0; i < count; i++) {
        close(merge_pipefds[i][0]);
    }

    // wait for merger child to die
    if((whom = waitpid(merger_pid, &status, 0)) != merger_pid) {
        perror("waitpid");
        exit(status);
    }

    // wait for sort children to die
    for(i = 0; i < count; i++) {
        if((whom = wait(&status)) == -1) {
            perror("wait");
            exit(status);
        }
    }

    // free allocated stuff
    for(i = 0; i < count; i++) {
        free(parse_pipefds[i]);
        free(merge_pipefds[i]);
    }
    free(parse_pipefds);
    free(merge_pipefds);
    free(pid);
    free(fps);

    return 0;
}
