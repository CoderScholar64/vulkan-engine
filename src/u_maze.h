#ifndef U_MAZE_29
#define U_MAZE_29

#include "u_maze_def.h"

UMazeData u_maze_gen_grid(unsigned width, unsigned height);

void u_maze_delete_grid(UMazeData *pMazeData);

UMazeConnection* u_maze_gen(UMazeData *pMazeData, unsigned *pEdgeAmount);

#endif // U_MAZE_29
