#ifndef U_MAZE_29
#define U_MAZE_29

#include "u_maze_def.h"

/**
 * This generates a grid of data.
 * @param width The amount of vertices would the grid have in the width axis.
 * @param depth The amount of vertices would the grid have in the depth axis.
 * @return UMazeData struct. If data members inside the struct are NULL then this allocation had failed.
 */
UMazeData u_maze_gen_full_sq_grid(unsigned width, unsigned depth);

void u_maze_delete_data(UMazeData *pMazeData);

UMazeGenResult u_maze_gen(const UMazeData *const pMazeData, uint32_t seed, int genVertexGrid);

void u_maze_delete_result(UMazeGenResult *pMazeGenResult);

#endif // U_MAZE_29
