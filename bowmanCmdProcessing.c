#include "bowmanCmdProcessing.h"
#include "globals.h"

// Check if the given string has a ".mp3" extension.
// Returns 1 if true (has ".mp3" extension), 0 otherwise.
int hasMp3Extension(char* str) {
    
    int i = 0;
    char* dotPos;
    char* extension;
    int hasMp3Extension = 0;

    extension = malloc(sizeof(char));

    dotPos = strchr(str, '.');

    if (dotPos == NULL) {
        free(extension);
        return 0;
    }

    do {
        extension = realloc(extension, sizeof(char) * (i + 2));
        extension[i] = *(dotPos + i);
        i++;
    } while (*(dotPos + i) != '\0');
    extension[i] = '\0';

    hasMp3Extension = (strcasecmp(".mp3", extension) == 0);

    free(extension);

    return hasMp3Extension;

}

// Check the first command word and convert it to an integer.
int validFirstCommandWord(char* command1) {
    return ((strcasecmp("CONNECT", command1) && strcasecmp("LOGOUT", command1) && strcasecmp("LIST", command1) 
            && strcasecmp("DOWNLOAD", command1) && strcasecmp("CHECK", command1) && strcasecmp("CLEAR", command1)) == 0);
}

// Count the number of words in the command.
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

// Read the first word in the command.
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

// Read the second word in a two-word command.
char* readSecondCommandWord(char* command) {

    char* command2;
    int i = 0;
    int j = 0;

    command2 = malloc(sizeof(char));

    if (command[0] == ' ') {
        //If the first character is a space, we move the pointer until we encounter a non-space character
        while (command[i] == ' ') {
            i++;
        }
    }

    //We move the pointer until we encounter a space
    while (command[i] != ' ') {
        i++;
    };

    //We move the pounter past all the spaces
    while (command[i] == ' ') {
        i++;
    }

    while (command[i] != '\0') {
        
        if (command[i] != ' ') {
            command2 = realloc(command2, sizeof(char) * (j + 2));
            command2[j] = command[i];
            j++;
        }
        i++;

    }
    command2[j] = '\0';

    return command2;
    
}

// Process a single-word command.
int processSingleWordCmd(char* command1) {

    int command_case_num;

    if (strcasecmp("CONNECT", command1) == 0) {
        command_case_num = CONNECT_CMD;
    } else if (strcasecmp("LOGOUT", command1) == 0) {
        command_case_num = LOGOUT_CMD;
    } else if (validFirstCommandWord(command1)) {
        //If the command is not CONNECT or LOGOUT but it is the first word of one of the commands that require two words, the command is unknown
        command_case_num = PARTIALLY_CORRECT_CMD;
    } else {
        //If the command is not CONNECT or LOGOUT and it is not the first word of one of the commands that require two words, the command is invalid
        command_case_num = INVALID_CMD;
    }

    return command_case_num;

}

// Process a two-word command.
int processDoubleWordCmd(char* command1, char* command2) {

    int command_case_num;

    if (validFirstCommandWord(command1)) {

        if ((strcasecmp("LIST", command1) || strcasecmp("SONGS", command2)) == 0) {
            command_case_num = LIST_SONGS_CMD;
        } else if ((strcasecmp("LIST", command1) || strcasecmp("PLAYLISTS", command2)) == 0) {
            command_case_num = LIST_PLAYLISTS_CMD;
        } else if (strcasecmp("DOWNLOAD", command1) == 0) {
            if (hasMp3Extension(command2)) {
                command_case_num = DOWNLOAD_SONG_CMD;
            } else {
                command_case_num = DOWNLOAD_PLAYLIST_CMD;
            }  
        } else if ((strcasecmp("CHECK", command1) || strcasecmp("DOWNLOADS", command2)) == 0) {
            command_case_num = CHECK_DOWNLOADS_CMD;
        } else if ((strcasecmp("CLEAR", command1) || strcasecmp("DOWNLOADS", command2)) == 0) {
            command_case_num = CLEAR_DOWNLOADS_CMD;
        } else {
            //First word of command was valid, but the second word is unknown
            command_case_num = PARTIALLY_CORRECT_CMD;
        };

    } else {
        //First part of command was invalid, all the command is invalid
        command_case_num = INVALID_CMD;
    }

    return command_case_num;

}

// Function to convert a command (cmd) to an integer.
int commandToCmdCaseNum(char* command) {

    int command_case_num = NO_CMD;

    char* command1;
    char* command2; 

    int words_num = 0;


    words_num = wordsNum(command);

    if (words_num == 0) {
        command_case_num = NO_CMD;
        return command_case_num;
    }

    command1 = readFirstCommandWord(command);

    if (words_num == 1) {
        //The command does not contain spaces, it is a single word

        command_case_num = processSingleWordCmd(command1);
    }

    if (words_num == 2) {
        //The command has two words 

        command2 = readSecondCommandWord(command);

        command_case_num = processDoubleWordCmd(command1, command2);

        free(command2);

    } 

    if (words_num > 2) {
            //If the second part of the initial command also has a space, there are more than two command words and the command will be unknown or invalid

            if (validFirstCommandWord(command1)) {
                //First word of command was valid, but the rest is unknown
                command_case_num = PARTIALLY_CORRECT_CMD;

            } else {
                //First part of command was invalid, all the command is invalid
                command_case_num = INVALID_CMD;
            }

    }

    free(command1);

    return command_case_num;
}