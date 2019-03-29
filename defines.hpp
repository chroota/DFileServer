#pragma once

//socket buf size
#define MAXBUFSIZE 256

// msg type
#define MSG_JOIN 0x01
#define MSG_UPDATE_STATUS 0x02
#define MSG_UPDATE_STATE_HASH 0x04
#define MSG_GET_STATE_NODE 0x05

// keep alive peroid time for node
#define ALIVE_SYNC_TIME 3
// sync time for update state
#define STATE_SYNC_TIME 3

// state node
#define DEFAULT_STATE_NODE "node1"

//check file sync time
#define CHECK_FILE_SYNC_TIME

// default node server port
#define NODE_SERVER_PORT 8083

#define MAX_NODES 0xffff

//max file nums of writing of filesaver
#define MAX_OPEN_FILES 30

//chunk buf size for transfer
#define TRANS_CHUNK_SIZE 256


/*
 * notify events
*/
#define EVENT_NUM 12
// const char *event_str[EVENT_NUM] =
// {
// 	"IN_ACCESS",
// 	"IN_MODIFY",
// 	"IN_ATTRIB",
// 	"IN_CLOSE_WRITE",
// 	"IN_CLOSE_NOWRITE",
// 	"IN_OPEN",
// 	"IN_MOVED_FROM",
// 	"IN_MOVED_TO",
// 	"IN_CREATE",
// 	"IN_DELETE",
// 	"IN_DELETE_SELF",
// 	"IN_MOVE_SELF"
// };

enum NODE_STATUS{
    NODE_STATUS_CREATE,
    NODE_STATUS_ALIVE,
    NODE_STATUS_DEAD
};