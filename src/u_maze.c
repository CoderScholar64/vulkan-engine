#include "u_maze.h"

#include "u_random.h"

#include "SDL_log.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static int allocData(UMazeData *pMazeData, size_t vertexAmount, size_t linkAmount) {
    assert(pMazeData != NULL);

    void *pMem = malloc(vertexAmount * sizeof(UMazeVertex) + linkAmount * sizeof(UMazeVertex**));

    if(pMem == NULL)
        return 0;

    pMazeData->vertexAmount = vertexAmount;
    pMazeData->linkAmount   = linkAmount;

    pMazeData->pVertices     = pMem;
    pMazeData->ppVertexLinks = pMem + vertexAmount * sizeof(UMazeVertex);

    return 1;
}

UMazeData u_maze_gen_data(unsigned width, unsigned height) {
    assert(width >= 2);
    assert(height >= 2);

    const size_t AMOUNT_OF_RECT_CORNERS = 4;
    size_t amountOfRectEdges = 2 * ((width - 2) + (height - 2));
    size_t amountOfRectMiddles = (width - 2) * (height - 2);

    size_t totalVertices = width * height;
    size_t totalEdges    = 4 * amountOfRectMiddles + 3 * amountOfRectEdges + 2 * AMOUNT_OF_RECT_CORNERS;

    UMazeData mazeData = {0};

    if(!allocData(&mazeData, totalVertices, totalEdges))
        return mazeData;

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

    if(pMazeData->pVertices != NULL)
        free(pMazeData->pVertices);
    // free(pMazeData->ppVertexLinks); // This does not need to happen because of the malloc scheme that was used.

    pMazeData->pVertices = NULL;
    pMazeData->ppVertexLinks = NULL;
    pMazeData->vertexAmount = 0;
    pMazeData->linkAmount = 0;
}

UMazeGenResult u_maze_gen(UMazeData *pMazeData, uint32_t seed, int genVertexGrid) {
    assert(pMazeData != NULL);

    UMazeGenResult mazeGenResult = {0};

    size_t linkArraySize = 0;
    size_t linkArrayMaxSize = pMazeData->linkAmount;
    UMazeLink *pLinkArray = malloc(pMazeData->linkAmount * sizeof(UMazeLink));

    if(pLinkArray == NULL)
        return mazeGenResult;

    size_t answerIndex = 0;
    size_t answerSize  = pMazeData->vertexAmount - 1; // Amount of edges to be returned.

    mazeGenResult.pLinks = calloc(answerSize, sizeof(UMazeLink));

    if(mazeGenResult.pLinks == NULL)
        return mazeGenResult;

    if(genVertexGrid && allocData(&mazeGenResult.vertexMazeData, pMazeData->vertexAmount, 2 * answerSize)) {
        for(size_t i = 0; i < mazeGenResult.vertexMazeData.vertexAmount; i++) {
            mazeGenResult.vertexMazeData.pVertices[i] = pMazeData->pVertices[i];
            mazeGenResult.vertexMazeData.pVertices[i].metadata.data.count = 0;
        }
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
            mazeGenResult.pLinks[answerIndex] = pLinkArray[linkIndex];
            answerIndex++;

            if(mazeGenResult.vertexMazeData.pVertices != NULL) {
                const size_t index_0 = pLinkArray[linkIndex].pVertexLink[0] - pMazeData->pVertices;
                const size_t index_1 = pLinkArray[linkIndex].pVertexLink[1] - pMazeData->pVertices;

                mazeGenResult.vertexMazeData.pVertices[index_0].metadata.data.count++;
                mazeGenResult.vertexMazeData.pVertices[index_1].metadata.data.count++;
            }
        }

        assert(linkArraySize != 0);
        linkArraySize--;
        pLinkArray[linkIndex] = pLinkArray[linkArraySize];
    }

    if(mazeGenResult.vertexMazeData.pVertices != NULL) {
        size_t count = 0;

        for(size_t i = 0; i < mazeGenResult.vertexMazeData.vertexAmount; i++) {
            mazeGenResult.vertexMazeData.pVertices[i].ppVertexLinks = &mazeGenResult.vertexMazeData.ppVertexLinks[count];
            count += mazeGenResult.vertexMazeData.pVertices[i].metadata.data.count;

            mazeGenResult.vertexMazeData.pVertices[i].metadata.data.count = 0;
        }

        assert(mazeGenResult.vertexMazeData.linkAmount == count);

        for(size_t i = 0; i < mazeGenResult.linkAmount; i++) {
            const size_t index_0 = mazeGenResult.pLinks[i].pVertexLink[0] - pMazeData->pVertices;
            const size_t index_1 = mazeGenResult.pLinks[i].pVertexLink[1] - pMazeData->pVertices;

            UMazeVertex *pMazeVertex0 = &mazeGenResult.vertexMazeData.pVertices[index_0];
            UMazeVertex *pMazeVertex1 = &mazeGenResult.vertexMazeData.pVertices[index_1];

            pMazeVertex0->ppVertexLinks[pMazeVertex0->metadata.data.count] = pMazeVertex1;
            pMazeVertex1->ppVertexLinks[pMazeVertex1->metadata.data.count] = pMazeVertex0;

            pMazeVertex0->metadata.data.count++;
            pMazeVertex1->metadata.data.count++;
        }
    }

    free(pLinkArray);

    mazeGenResult.linkAmount = answerSize;

    return mazeGenResult;
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
