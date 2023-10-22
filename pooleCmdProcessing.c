/* #include "pooleCmdProcessing.h"
#include "globals.h"

int wordsNum(char* str) {

    int words_num = 0;
    int i = 0;

    if (str[0] == '\0') {
        words_num = 0;
        return words_num;
    } 

    if (str[0] != ' ') {
        words_num = 1;
        while (str[i] != '\0') {
            if (str[i] == ' ' && str[i + 1] != ' ' && str[i + 1] != '\0') {
                words_num++;
            }
            i++;
        }
    } else {
        while (str[i] != '\0') {
            if (str[i] == ' ' && str[i + 1] != ' '  && str[i + 1] != '\0') {
                words_num++;
            }
            i++;
        }
    }

    write(1, &words_num, sizeof(int));

    return words_num;
}

char* readFirstCommandWord(char* command) {
  
    char* command1;
    int i = 0;
    int j = 0;

    command1 = malloc(sizeof(char));

    if (command[0] == ' ') {
        //If the first character is a space, we move the pointer until we encounter a non-space character
        while (command[i] == ' ') {
            i++;
        }
    }

    while (command[i] != ' ' && command[i] != '\0') {
        
        if (command[i] != ' ') {
            command1 = realloc(command1, sizeof(char) * (j + 2));
            command1[j] = command[i];
            j++;
        }
        i++;

    }
    command1[j] = '\0';

    return command1;
}

int processSingleWordCmd(char* command1) {

    int command_case_num;

    if (strcasecmp("LOGOUT", command1) == 0) {
        command_case_num = LOGOUT_CMD;
    } else {
        //If the command is not CONNECT or LOGOUT and it is not the first word of one of the commands that require two words, the command is invalid
        command_case_num = INVALID_CMD;
    }

    return command_case_num;

}

int commandToCmdCaseNum(char* command) {

    int command_case_num = NO_CMD;

    char* command1;

    int words_num = 0;


    words_num = wordsNum(command);

    if (words_num == 0) {
        command_case_num = NO_CMD;
        return command_case_num;
    }

    if (words_num == 1) {
        //The command does not contain spaces, it is a single word
        command1 = readFirstCommandWord(command);
        command_case_num = processSingleWordCmd(command1);
        free(command1);
        return command_case_num;
    }

    if (words_num > 1) {
        command_case_num = INVALID_CMD;
    }

    return command_case_num;

    
} */