#include "u_maze.h"

#include "u_bit_array.h"
#include "u_random.h"

#include "SDL_log.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// #define DEBUG_U_MAZE

static int allocData(UMazeData *pMazeData, size_t vertexAmount, size_t linkAmount) {
    assert(pMazeData != NULL);

    void *pMem = malloc(vertexAmount * sizeof(UMazeVertex) + linkAmount * sizeof(size_t));

    if(pMem == NULL)
        return 0;

    pMazeData->vertexAmount = vertexAmount;
    pMazeData->linkAmount   = linkAmount;

    pMazeData->pVertices        = pMem;
    pMazeData->pVertexLinkArray = pMem + vertexAmount * sizeof(UMazeVertex);

    return 1;
}

int u_maze_gen_full_sq_grid(UMazeData *pMazeData, unsigned width, unsigned depth) {
    assert(width >= 2);
    assert(depth >= 2);

    const size_t AMOUNT_OF_RECT_CORNERS = 4;
    size_t amountOfRectEdges = 2 * ((width - 2) + (depth - 2));
    size_t amountOfRectMiddles = (width - 2) * (depth - 2);

    size_t totalVertices = width * depth;
    size_t totalLinks    = 4 * amountOfRectMiddles + 3 * amountOfRectEdges + 2 * AMOUNT_OF_RECT_CORNERS;

    if(!allocData(pMazeData, totalVertices, totalLinks))
        return 0;

    size_t *pVertexLinkArray = pMazeData->pVertexLinkArray;

    for(unsigned y = 0; y < depth; y++) {
        for(unsigned x = 0; x < width; x++) {
            size_t offset = y * width + x;
            UMazeVertex *pCurVert = &pMazeData->pVertices[offset];

            pCurVert->linkAmount          = 0;
            pCurVert->metadata.position.x = x;
            pCurVert->metadata.position.y = y;
            pCurVert->pVertexLinkIndexes  = pVertexLinkArray;

            if(offset + 1 < pMazeData->vertexAmount && (offset + 1) % width != 0) {
                pCurVert->pVertexLinkIndexes[pCurVert->linkAmount] = offset + 1;
                pCurVert->linkAmount++;
#ifdef DEBUG_U_MAZE
                SDL_Log( "pVertices[%li] links to %li", offset, offset + 1);
#endif
            }
            if(offset % width != 0) {
                pCurVert->pVertexLinkIndexes[pCurVert->linkAmount] = offset - 1;
                pCurVert->linkAmount++;
#ifdef DEBUG_U_MAZE
                SDL_Log( "pVertices[%li] links to %li", offset, offset - 1);
#endif
            }
            if(offset + width < pMazeData->vertexAmount) {
                pCurVert->pVertexLinkIndexes[pCurVert->linkAmount] = offset + width;
                pCurVert->linkAmount++;
#ifdef DEBUG_U_MAZE
                SDL_Log( "pVertices[%li] links to %li", offset, offset + width);
#endif
            }
            if(offset >= width) {
                pCurVert->pVertexLinkIndexes[pCurVert->linkAmount] = offset - width;
                pCurVert->linkAmount++;
#ifdef DEBUG_U_MAZE
                SDL_Log( "pVertices[%li] links to %li", offset, offset - width);
#endif
            }

            pVertexLinkArray = pVertexLinkArray + pCurVert->linkAmount;
        }
    }

    return 1;
}

void u_maze_delete_data(UMazeData *pMazeData) {
    assert(pMazeData != NULL);

    if(pMazeData->pVertices != NULL)
        free(pMazeData->pVertices);
    // free(pMazeData->ppVertexLinks); // This does not need to happen because of the malloc scheme that was used.

    pMazeData->pVertices = NULL;
    pMazeData->pVertexLinkArray = NULL;
    pMazeData->vertexAmount = 0;
    pMazeData->linkAmount = 0;
}

int u_maze_gen(UMazeGenResult *pUMazeGenResult, const UMazeData *const pMazeData, uint32_t seed, int genVertexGrid) {
    assert(pMazeData != NULL);

    size_t linkArraySize = 0;
    size_t linkArrayMaxSize = pMazeData->linkAmount;

    void *pMem = malloc(U_BIT_ARRAY_SIZE(pMazeData->vertexAmount) + pMazeData->linkAmount * sizeof(UMazeLink));

    if(pMem == NULL)
        return 0;

    memset(pMem, 0, U_BIT_ARRAY_SIZE(pMazeData->vertexAmount));

    u_bit_element *pBitVisitedArray = pMem;

    UMazeLink *pLinkArray = pMem + U_BIT_ARRAY_SIZE(pMazeData->vertexAmount);

    size_t answerIndex = 0;
    size_t answerSize  = pMazeData->vertexAmount - 1; // Amount of edges to be returned.

    pUMazeGenResult->pLinks = calloc(answerSize, sizeof(UMazeLink));

    if(pUMazeGenResult->pLinks == NULL) {
        free(pMem);
        return 0;
    }

    if(genVertexGrid && allocData(&pUMazeGenResult->vertexMazeData, pMazeData->vertexAmount, 2 * answerSize)) {
        for(size_t i = 0; i < pUMazeGenResult->vertexMazeData.vertexAmount; i++) {
            pUMazeGenResult->vertexMazeData.pVertices[i] = pMazeData->pVertices[i];
            pUMazeGenResult->vertexMazeData.pVertices[i].linkAmount = 0;
        }
    }

    pUMazeGenResult->pSource = pMazeData;

    size_t vertexIndex = u_random_xorshift32(&seed) % pMazeData->vertexAmount;

    U_BIT_ARRAY_SET(pBitVisitedArray, vertexIndex, 1);
    for(uint32_t i = 0; i < pMazeData->pVertices[vertexIndex].linkAmount; i++) {
        assert(linkArraySize < linkArrayMaxSize);

        pLinkArray[linkArraySize].vertexIndex[0] = vertexIndex;
        pLinkArray[linkArraySize].vertexIndex[1] = pMazeData->pVertices[vertexIndex].pVertexLinkIndexes[i];

        linkArraySize++;
    }

    while(answerIndex != answerSize && linkArraySize != 0) {
        const size_t linkIndex = u_random_xorshift32(&seed) % linkArraySize;
        const size_t vertexLinkIndex = pLinkArray[linkIndex].vertexIndex[1];

        if(U_BIT_ARRAY_GET(pBitVisitedArray, vertexLinkIndex) == 0) {
            U_BIT_ARRAY_SET(pBitVisitedArray, vertexLinkIndex, 1);

            for(uint32_t i = 0; i < pMazeData->pVertices[pLinkArray[linkIndex].vertexIndex[1]].linkAmount; i++) {
                if(pMazeData->pVertices[pLinkArray[linkIndex].vertexIndex[1]].pVertexLinkIndexes[i] != pLinkArray[linkIndex].vertexIndex[0]) {
                    assert(linkArraySize < linkArrayMaxSize);

                    pLinkArray[linkArraySize].vertexIndex[0] = pLinkArray[linkIndex].vertexIndex[1];
                    pLinkArray[linkArraySize].vertexIndex[1] = pMazeData->pVertices[pLinkArray[linkIndex].vertexIndex[1]].pVertexLinkIndexes[i];

                    linkArraySize++;
                }
            }

            assert(answerIndex < answerSize);
            pUMazeGenResult->pLinks[answerIndex] = pLinkArray[linkIndex];
            answerIndex++;

            if(pUMazeGenResult->vertexMazeData.pVertices != NULL) {
                pUMazeGenResult->vertexMazeData.pVertices[pLinkArray[linkIndex].vertexIndex[0]].linkAmount++;
                pUMazeGenResult->vertexMazeData.pVertices[pLinkArray[linkIndex].vertexIndex[1]].linkAmount++;
            }
        }

        assert(linkArraySize != 0);
        linkArraySize--;
        pLinkArray[linkIndex] = pLinkArray[linkArraySize];
    }

    if(pUMazeGenResult->vertexMazeData.pVertices != NULL) {
        size_t count = 0;

        for(size_t i = 0; i < pUMazeGenResult->vertexMazeData.vertexAmount; i++) {
            pUMazeGenResult->vertexMazeData.pVertices[i].pVertexLinkIndexes = &pUMazeGenResult->vertexMazeData.pVertexLinkArray[count];
            count += pUMazeGenResult->vertexMazeData.pVertices[i].linkAmount;

#ifdef DEBUG_U_MAZE
            SDL_Log("Point %zu has %zu vertices!\n", i, pUMazeGenResult->vertexMazeData.pVertices[i].linkAmount);
#endif

            pUMazeGenResult->vertexMazeData.pVertices[i].linkAmount = 0;
        }

        assert(pUMazeGenResult->vertexMazeData.linkAmount == count);

        for(size_t i = 0; i < pUMazeGenResult->vertexMazeData.linkAmount / 2; i++) {
            const size_t index_0 = pUMazeGenResult->pLinks[i].vertexIndex[0];
            const size_t index_1 = pUMazeGenResult->pLinks[i].vertexIndex[1];

#ifdef DEBUG_U_MAZE
            SDL_Log("Link %zu (%zu, %zu)\n", i, index_0, index_1);
#endif

            UMazeVertex *pMazeVertex0 = &pUMazeGenResult->vertexMazeData.pVertices[index_0];
            UMazeVertex *pMazeVertex1 = &pUMazeGenResult->vertexMazeData.pVertices[index_1];

            pMazeVertex0->pVertexLinkIndexes[pMazeVertex0->linkAmount] = index_1;
            pMazeVertex1->pVertexLinkIndexes[pMazeVertex1->linkAmount] = index_0;

            pMazeVertex0->linkAmount++;
            pMazeVertex1->linkAmount++;
        }

#ifdef DEBUG_U_MAZE
        for(size_t i = 0; i < pUMazeGenResult->vertexMazeData.vertexAmount; i++) {
            printf("Point %zu links to %zu vertices:", i, pUMazeGenResult->vertexMazeData.pVertices[i].linkAmount);

            for(unsigned l = 0; l < pUMazeGenResult->vertexMazeData.pVertices[i].linkAmount; l++) {
                printf(" %zu", pUMazeGenResult->vertexMazeData.pVertices[i].pVertexLinkIndexes[l]);
            }

            printf("\n");
        }
#endif
    }

    free(pMem);

    pUMazeGenResult->linkAmount = answerSize;

    return 1;
}

void u_maze_delete_result(UMazeGenResult *pMazeGenResult) {
    assert(pMazeGenResult != NULL);

    // u_maze_delete_data(pMazeGenResult->pSource); // pMazeGenResult->pSource is a reference not the source.

    if(pMazeGenResult->pLinks != NULL)
        free(pMazeGenResult->pLinks);

    pMazeGenResult->linkAmount = 0;
    pMazeGenResult->pLinks = NULL;

    u_maze_delete_data(&pMazeGenResult->vertexMazeData); // pMazeGenResult->vertexMazeData
}
