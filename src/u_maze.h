#ifndef U_MAZE_29
#define U_MAZE_29

#include "u_maze_def.h"

/**
 * This generates a grid of data.
 * @param pMazeData A pointer to the UMazeData struct to be modified by this function.
 * @param width The amount of vertices would the grid have in the width axis.
 * @param depth The amount of vertices would the grid have in the depth axis.
 * @return 1 for success or 0 for failure.
 */
int u_maze_gen_full_sq_grid(UMazeData *pMazeData, unsigned width, unsigned depth);

/**
 * This function deletes the UMazeData struct data.
 * @warning ONLY delete UMazeData through this function due to technical reasons.
 * @note This is meant to delete UMazeData by u_maze_gen_full_sq_grid function.
 * @param pMazeData The maze data to be deleted.
 */
void u_maze_delete_data(UMazeData *pMazeData);

/**
 * This method is what generates a maze.
 * @param pMazeGenResult The struct that would hold the generated maze if this function succeeds.
 * @param pMazeData The nodes and connections to form a maze from. Basically the result of u_maze_gen_full_sq_grid.
 * @param seed The seed for the random number generator.
 * @param genVertexGrid If one then pMazeGenResult will also have vertexMazeData allocated.
 * @return 1 for success or 0 for failure.
 */
int u_maze_gen(UMazeGenResult *pMazeGenResult, const UMazeData *const pMazeData, uint32_t seed, int genVertexGrid);

/**
 * Deallocate the maze result.
 * @param pMazeGenResult The result to be deleted.
 */
void u_maze_delete_result(UMazeGenResult *pMazeGenResult);

#endif // U_MAZE_29
