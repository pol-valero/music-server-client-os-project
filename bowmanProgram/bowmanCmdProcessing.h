#ifndef _BOWMAN_CMD_PROCESSING_H_
#define _BOWMAN_CMD_PROCESSING_H_

#include "../globals.h"

#define CONNECT_CMD 1
#define LOGOUT_CMD 2
#define LIST_SONGS_CMD 3
#define LIST_PLAYLISTS_CMD 4
#define DOWNLOAD_SONG_CMD 5
#define DOWNLOAD_PLAYLIST_CMD 6
#define CHECK_DOWNLOADS_CMD 7
#define CLEAR_DOWNLOADS_CMD 8
#define PARTIALLY_CORRECT_CMD 9
#define INVALID_CMD 10
#define NO_CMD 11


int commandToCmdCaseNum(char* command);

#endif