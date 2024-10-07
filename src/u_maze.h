#ifndef U_MAZE_29
#define U_MAZE_29

#include "u_maze_def.h"

UMazeData u_maze_gen_data(unsigned width, unsigned height);

void u_maze_delete_data(UMazeData *pMazeData);

UMazeGenResult u_maze_gen(UMazeData *pMazeData, uint32_t seed);

void u_maze_delete_result(UMazeGenResult *pMazeGenResult);

#endif // U_MAZE_29
