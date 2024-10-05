#include "u_maze.h"

#include <assert.h>
#include <stdlib.h>

UMazeData u_maze_gen_grid(unsigned width, unsigned height) {
    assert(width >= 2);
    assert(height >= 2);

    const size_t AMOUNT_OF_RECT_CORNERS = 4;
    size_t amountOfRectEdges = 2 * ((width - 2) + (height - 2));
    size_t amountOfRectMiddles = (width - 2) * (height - 2);

    size_t totalVertices = width * height;
    size_t totalEdges    = 4 * amountOfRectMiddles + 3 * amountOfRectEdges + 2 * AMOUNT_OF_RECT_CORNERS;

    UMazeData mazeData = {0};
    mazeData.vertexAmount = totalVertices;

    void *pMem = malloc(mazeData.vertexAmount * sizeof(UMazeVertex) + totalEdges * sizeof(UMazeVertex**));

    mazeData.pVertices = pMem;
    mazeData.ppConnections = pMem + mazeData.vertexAmount * sizeof(UMazeVertex);

    UMazeVertex **ppConnections = mazeData.ppConnections;

    for(unsigned y = 0; y < height; y++) {
        for(unsigned x = 0; x < width; x++) {
            size_t offset = y * width + x;
            UMazeVertex *pCurVert = &mazeData.pVertices[offset];

            pCurVert->metadata.data.isVisited = 0;
            pCurVert->metadata.data.count     = 0;
            pCurVert->metadata.position.x     = x;
            pCurVert->metadata.position.y     = y;
            pCurVert->ppConnections           = ppConnections;

            if(offset + 1 < mazeData.vertexAmount && (offset + 1) % width != 0) {
                pCurVert->ppConnections[pCurVert->metadata.data.count] = &mazeData.pVertices[offset + 1];
                pCurVert->metadata.data.count++;
            }
            if(offset != 0 && (offset - 1) % width != 0) {
                pCurVert->ppConnections[pCurVert->metadata.data.count] = &mazeData.pVertices[offset - 1];
                pCurVert->metadata.data.count++;
            }
            if(offset + width < mazeData.vertexAmount) {
                pCurVert->ppConnections[pCurVert->metadata.data.count] = &mazeData.pVertices[offset + width];
                pCurVert->metadata.data.count++;
            }
            if(offset >= width) {
                pCurVert->ppConnections[pCurVert->metadata.data.count] = &mazeData.pVertices[offset - width];
                pCurVert->metadata.data.count++;
            }

            ppConnections = ppConnections + pCurVert->metadata.data.count;
        }
    }

    return mazeData;
}

void u_maze_delete_grid(UMazeData *pMazeData) {
    assert(pMazeData != NULL);

    free(pMazeData->pVertices);

    pMazeData->pVertices = NULL;
    pMazeData->ppConnections = NULL;
    pMazeData->vertexAmount = 0;
}

UMazeConnection* u_maze_gen(UMazeData *pMazeData, unsigned *pEdgeAmount) {
    assert(pMazeData != NULL);
    assert(pEdgeAmount != NULL);
}
