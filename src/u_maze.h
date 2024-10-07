#ifndef U_MAZE_29
#define U_MAZE_29

#include "u_maze_def.h"

UMazeData u_maze_gen_data(unsigned width, unsigned height);

void u_maze_delete_data(UMazeData *pMazeData);

UMazeLink* u_maze_gen(UMazeData *pMazeData, size_t *pEdgeAmount, uint32_t seed);

#endif // U_MAZE_29
