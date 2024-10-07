#include "u_maze.h"

#include "u_random.h"

#include "SDL_log.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

UMazeData u_maze_gen_data(unsigned width, unsigned height) {
    assert(width >= 2);
    assert(height >= 2);

    const size_t AMOUNT_OF_RECT_CORNERS = 4;
    size_t amountOfRectEdges = 2 * ((width - 2) + (height - 2));
    size_t amountOfRectMiddles = (width - 2) * (height - 2);

    size_t totalVertices = width * height;
    size_t totalEdges    = 4 * amountOfRectMiddles + 3 * amountOfRectEdges + 2 * AMOUNT_OF_RECT_CORNERS;

    UMazeData mazeData = {0};
    mazeData.vertexAmount = totalVertices;
    mazeData.linkAmount = totalEdges;

    void *pMem = malloc(mazeData.vertexAmount * sizeof(UMazeVertex) + totalEdges * sizeof(UMazeVertex**));

    mazeData.pVertices = pMem;
    mazeData.ppVertexLinks = pMem + mazeData.vertexAmount * sizeof(UMazeVertex);

    UMazeVertex **ppConnections = mazeData.ppVertexLinks;

    for(unsigned y = 0; y < height; y++) {
        for(unsigned x = 0; x < width; x++) {
            size_t offset = y * width + x;
            UMazeVertex *pCurVert = &mazeData.pVertices[offset];

            pCurVert->metadata.data.isVisited = 0;
            pCurVert->metadata.data.count     = 0;
            pCurVert->metadata.position.x     = x;
            pCurVert->metadata.position.y     = y;
            pCurVert->ppVertexLinks           = ppConnections;

            if(offset + 1 < mazeData.vertexAmount && (offset + 1) % width != 0) {
                pCurVert->ppVertexLinks[pCurVert->metadata.data.count] = &mazeData.pVertices[offset + 1];
                pCurVert->metadata.data.count++;
                //SDL_Log( "pVertices[%li] links to %li", offset, offset + 1);
            }
            if(offset % width != 0) {
                pCurVert->ppVertexLinks[pCurVert->metadata.data.count] = &mazeData.pVertices[offset - 1];
                pCurVert->metadata.data.count++;
                //SDL_Log( "pVertices[%li] links to %li", offset, offset - 1);
            }
            if(offset + width < mazeData.vertexAmount) {
                pCurVert->ppVertexLinks[pCurVert->metadata.data.count] = &mazeData.pVertices[offset + width];
                pCurVert->metadata.data.count++;
                //SDL_Log( "pVertices[%li] links to %li", offset, offset + width);
            }
            if(offset >= width) {
                pCurVert->ppVertexLinks[pCurVert->metadata.data.count] = &mazeData.pVertices[offset - width];
                pCurVert->metadata.data.count++;
                //SDL_Log( "pVertices[%li] links to %li", offset, offset - width);
            }

            ppConnections = ppConnections + pCurVert->metadata.data.count;
        }
    }

    return mazeData;
}

void u_maze_delete_data(UMazeData *pMazeData) {
    assert(pMazeData != NULL);

    free(pMazeData->pVertices);
    // free(pMazeData->ppVertexLinks); // This does not need to happen because of the malloc scheme that was used.

    pMazeData->pVertices = NULL;
    pMazeData->ppVertexLinks = NULL;
    pMazeData->vertexAmount = 0;
    pMazeData->linkAmount = 0;
}

UMazeGenResult u_maze_gen(UMazeData *pMazeData, uint32_t seed, UMazeGenFlags uMazeGenFlags) {
    assert(pMazeData != NULL);

    UMazeGenResult mazeGenResult = {0};

    size_t answerIndex = 0;
    size_t answerSize  = pMazeData->vertexAmount - 1; // Amount of edges to be returned.
    UMazeLink *pAnswerMazeLinks = calloc(answerSize, sizeof(UMazeLink));

    if(pAnswerMazeLinks == NULL)
        return mazeGenResult;

    size_t linkArraySize = 0;
    size_t linkArrayMaxSize = pMazeData->linkAmount;
    UMazeLink *pLinkArray = malloc(pMazeData->linkAmount * sizeof(UMazeLink));

    if(pLinkArray == NULL) {
        free(pAnswerMazeLinks);
        return mazeGenResult;
    }

    mazeGenResult.pSource = pMazeData;

    size_t vertexIndex = u_random_xorshift32(&seed) % pMazeData->vertexAmount;

    pMazeData->pVertices[vertexIndex].metadata.data.isVisited = 1;
    for(uint32_t i = 0; i < pMazeData->pVertices[vertexIndex].metadata.data.count; i++) {
        assert(linkArraySize < linkArrayMaxSize);

        pLinkArray[linkArraySize].pVertexLink[0] = &pMazeData->pVertices[vertexIndex];
        pLinkArray[linkArraySize].pVertexLink[1] =  pMazeData->pVertices[vertexIndex].ppVertexLinks[i];

        linkArraySize++;
    }

    while(answerIndex != answerSize && linkArraySize != 0) {
        size_t linkIndex = u_random_xorshift32(&seed) % linkArraySize;

        if(pLinkArray[linkIndex].pVertexLink[1]->metadata.data.isVisited == 0) {
            pLinkArray[linkIndex].pVertexLink[1]->metadata.data.isVisited = 1;

            for(uint32_t i = 0; i < pLinkArray[linkIndex].pVertexLink[1]->metadata.data.count; i++) {
                if(pLinkArray[linkIndex].pVertexLink[1]->ppVertexLinks[i] != pLinkArray[linkIndex].pVertexLink[0]) {
                    assert(linkArraySize < linkArrayMaxSize);

                    pLinkArray[linkArraySize].pVertexLink[0] = pLinkArray[linkIndex].pVertexLink[1];
                    pLinkArray[linkArraySize].pVertexLink[1] = pLinkArray[linkIndex].pVertexLink[1]->ppVertexLinks[i];

                    linkArraySize++;
                }
            }

            assert(answerIndex < answerSize);
            pAnswerMazeLinks[answerIndex] = pLinkArray[linkIndex];
            answerIndex++;
        }

        assert(linkArraySize != 0);
        linkArraySize--;
        pLinkArray[linkIndex] = pLinkArray[linkArraySize];
    }

    free(pLinkArray);

    mazeGenResult.linkAmount = answerSize;
    mazeGenResult.pLinks = pAnswerMazeLinks;

    return mazeGenResult;
}

void u_maze_delete_result(UMazeGenResult *pMazeGenResult) {
    assert(pMazeGenResult != NULL);

    // u_maze_delete_data(pMazeGenResult->pSource); // pMazeGenResult->pSource

    if(pMazeGenResult->pLinks != NULL)
        free(pMazeGenResult->pLinks);

    pMazeGenResult->linkAmount = 0;
    pMazeGenResult->pLinks = NULL;

    u_maze_delete_data(&pMazeGenResult->vertexMazeData); // pMazeGenResult->vertexMazeData
}
