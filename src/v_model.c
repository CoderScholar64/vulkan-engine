#include "v_model.h"

#include "cgltf.h"
#include "SDL_log.h"

#include "u_read.h"
#include "context.h"

VEngineResult v_load_models(Context *this, const char *const pUTF8Filepath, unsigned *pModelAmount, VModelData **ppVModelData) {
    *pModelAmount = 0;
    *ppVModelData = NULL;

    cgltf_result result;
    cgltf_data *pModel = u_gltf_read(pUTF8Filepath, &result);

    if(result != cgltf_result_success) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to find model!");
        RETURN_RESULT_CODE(VE_LOAD_MODEL_FAILURE, 0)
    }

    if(pModel->meshes_count == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "This glTF file has no meshes contained in it to read!");
        cgltf_free(pModel);
        RETURN_RESULT_CODE(VE_LOAD_MODEL_FAILURE, 1)
    }

    // TODO Find cleaner and more stable loading algorithm.

    VModelData *pVModel = malloc(sizeof(VModelData) * pModel->meshes_count );

    for(unsigned mesh_index = 0; mesh_index < pModel->meshes_count; mesh_index++) {
        if(pModel->meshes[mesh_index].primitives_count == 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "This glTF file has no primitives stored in it!");
            cgltf_free(pModel);
            RETURN_RESULT_CODE(VE_LOAD_MODEL_FAILURE, 2)
        }

        if(pModel->buffers_count == 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "This glTF file has no buffers!");
            cgltf_free(pModel);
            RETURN_RESULT_CODE(VE_LOAD_MODEL_FAILURE, 3)
        }

        if(pModel->meshes[mesh_index].primitives[0].attributes_count == 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "This glTF file has no attributes!");
            cgltf_free(pModel);
            RETURN_RESULT_CODE(VE_LOAD_MODEL_FAILURE, 4)
        }

        if(pModel->meshes[mesh_index].primitives[0].type != cgltf_primitive_type_triangles) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "This glTF file has unsupported triangle type %d!", pModel->meshes[mesh_index].primitives[0].type);
            cgltf_free(pModel);
            RETURN_RESULT_CODE(VE_LOAD_MODEL_FAILURE, 5)
        }

        cgltf_size loadBufferSize = 0;

        cgltf_attribute *pPositionAttribute = NULL;
        cgltf_size       positionNumComponent;
        cgltf_attribute *pColorAttribute    = NULL;
        cgltf_size       colorNumComponent;
        cgltf_attribute *pTexCoordAttribute = NULL;
        cgltf_size       texCoordNumComponent;

        cgltf_size vertexAmount;

        for(size_t i = 0; i < pModel->meshes[mesh_index].primitives[0].attributes_count; i++) {
            switch(pModel->meshes[mesh_index].primitives[0].attributes[i].type) {
                case cgltf_attribute_type_position:
                    pPositionAttribute = &pModel->meshes[mesh_index].primitives[0].attributes[i];

                    positionNumComponent = cgltf_num_components(pPositionAttribute->data->type);

                    if(positionNumComponent < 3) {
                        pPositionAttribute = NULL;
                        SDL_Log("Position does not have enough components = %li", positionNumComponent);
                        break;
                    }

                    vertexAmount = pPositionAttribute->data->count;

                    SDL_Log("Position Buffer size = %li", sizeof(cgltf_float) * cgltf_accessor_unpack_floats(pPositionAttribute->data, NULL, positionNumComponent * pPositionAttribute->data->count));

                    loadBufferSize = fmax(loadBufferSize, sizeof(cgltf_float) * cgltf_accessor_unpack_floats(pPositionAttribute->data, NULL, positionNumComponent * pPositionAttribute->data->count));
                    break;
                case cgltf_attribute_type_color:
                    pColorAttribute = &pModel->meshes[mesh_index].primitives[0].attributes[i];

                    colorNumComponent = cgltf_num_components(pColorAttribute->data->type);

                    if(colorNumComponent < 3) {
                        pColorAttribute = NULL;
                        SDL_Log("Color does not have enough components = %li", colorNumComponent);
                        break;
                    }

                    SDL_Log("Color Buffer size = %li", sizeof(cgltf_float) * cgltf_accessor_unpack_floats(pColorAttribute->data, NULL, colorNumComponent * pColorAttribute->data->count));

                    loadBufferSize = fmax(loadBufferSize, sizeof(cgltf_float) * cgltf_accessor_unpack_floats(pColorAttribute->data, NULL, colorNumComponent * pColorAttribute->data->count));
                    break;
                case cgltf_attribute_type_texcoord:
                    pTexCoordAttribute = &pModel->meshes[mesh_index].primitives[0].attributes[i];

                    texCoordNumComponent = cgltf_num_components(pTexCoordAttribute->data->type);

                    if(texCoordNumComponent < 2) {
                        pTexCoordAttribute = NULL;
                        SDL_Log("Texture coordinates does not have enough components = %li", texCoordNumComponent);
                        break;
                    }

                    SDL_Log("Texture Buffer size = %li", sizeof(cgltf_float) * cgltf_accessor_unpack_floats(pTexCoordAttribute->data, NULL, texCoordNumComponent * pTexCoordAttribute->data->count));

                    loadBufferSize = fmax(loadBufferSize, sizeof(cgltf_float) * cgltf_accessor_unpack_floats(pTexCoordAttribute->data, NULL, texCoordNumComponent * pTexCoordAttribute->data->count));
                    break;
                default:
                    // Just do nothing for unrecognized attributes.
            }
            SDL_Log("Attribute %li\n  name = %s\n  type = %i\n  index = %i\n", i, pModel->meshes[mesh_index].primitives[0].attributes[i].name, pModel->meshes[mesh_index].primitives[0].attributes[i].type, pModel->meshes[mesh_index].primitives[0].attributes[i].index);
        }

        if(pPositionAttribute == NULL) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No position attribute found!");
            cgltf_free(pModel);
            RETURN_RESULT_CODE(VE_LOAD_MODEL_FAILURE, 6)
        }

        cgltf_accessor* pIndices = NULL;
        VkIndexType indexType;
        cgltf_size indexComponentSize;
        cgltf_size indexBufferSize = 0;

        if(pModel->meshes[mesh_index].primitives[0].indices != NULL) {
            pIndices = pModel->meshes[mesh_index].primitives[0].indices;

            if(pIndices->type != cgltf_type_scalar) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "This model has component_type = %i", pIndices->type);
                cgltf_free(pModel);
                RETURN_RESULT_CODE(VE_LOAD_MODEL_FAILURE, 7)
            }

            switch(pIndices->component_type) {
                case cgltf_component_type_r_8u:
                    SDL_Log("This model actually stores cgltf_component_type_r_8u. It will be converted to 16u");
                case cgltf_component_type_r_16u:
                    SDL_Log("This model has component_type = cgltf_component_type_r_16u");

                    indexComponentSize = cgltf_component_size(cgltf_component_type_r_16u);
                    indexType = VK_INDEX_TYPE_UINT16;
                    break;
                case cgltf_component_type_r_32u:
                    SDL_Log("This model has component_type = cgltf_component_type_r_32u");

                    indexComponentSize = cgltf_component_size(cgltf_component_type_r_32u);
                    indexType = VK_INDEX_TYPE_UINT32;
                    break;
                default:
                    SDL_Log("This model has invalid component_type = %i", pIndices->component_type);
                    cgltf_free(pModel);
                    RETURN_RESULT_CODE(VE_LOAD_MODEL_FAILURE, 8)
            }

            indexBufferSize = indexComponentSize * cgltf_accessor_unpack_indices(pIndices, NULL, indexComponentSize, pIndices->count);

            SDL_Log("This model has indices = %li.\n", pIndices->count);
        }

        // Record the mesh name.
        snprintf(pVModel[mesh_index].name, sizeof(pVModel[mesh_index].name) / sizeof(pVModel[mesh_index].name[0]), "%s", pModel->meshes[mesh_index].name);

        SDL_Log("\n  Name = %s\n  Buffer size = %li", pVModel[mesh_index].name, loadBufferSize);

        void *pLoadBuffer = malloc(loadBufferSize + indexBufferSize + sizeof(Vertex) * vertexAmount);

        if(pLoadBuffer == NULL || pVModel == NULL) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate a %li large buffer", loadBufferSize);
            cgltf_free(pModel);
            free(pVModel);
            RETURN_RESULT_CODE(VE_LOAD_MODEL_FAILURE, 9)
        }

        void   *pIndexedBuffer    = pLoadBuffer + (loadBufferSize);
        Vertex *pInterlacedBuffer = pLoadBuffer + (loadBufferSize + indexBufferSize);

        if(pIndices != NULL) {
            cgltf_accessor_unpack_indices(pIndices, pIndexedBuffer, indexComponentSize, pIndices->count);

            pVModel[mesh_index].indexType = indexType;
        }

        cgltf_accessor_unpack_floats(pPositionAttribute->data, pLoadBuffer, positionNumComponent * pPositionAttribute->data->count);

        SDL_Log( "components = %li", positionNumComponent);

        const Vertex defaultVertex = {{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}};
        for(cgltf_size v = 0; v < vertexAmount; v++) {
            pInterlacedBuffer[v].pos.x = ((float*)pLoadBuffer)[positionNumComponent * v + 0];
            pInterlacedBuffer[v].pos.z = ((float*)pLoadBuffer)[positionNumComponent * v + 1];
            pInterlacedBuffer[v].pos.y =-((float*)pLoadBuffer)[positionNumComponent * v + 2];

            pInterlacedBuffer[v].color    = defaultVertex.color;
            pInterlacedBuffer[v].texCoord = defaultVertex.texCoord;
        }

        if(pColorAttribute != NULL) {
            cgltf_accessor_unpack_floats(pColorAttribute->data, pLoadBuffer, colorNumComponent * pColorAttribute->data->count);

            for(cgltf_size v = 0; v < vertexAmount; v++) {
                pInterlacedBuffer[v].color.x = ((float*)pLoadBuffer)[colorNumComponent * v + 0];
                pInterlacedBuffer[v].color.y = ((float*)pLoadBuffer)[colorNumComponent * v + 1];
                pInterlacedBuffer[v].color.z = ((float*)pLoadBuffer)[colorNumComponent * v + 2];
            }
        }

        if(pTexCoordAttribute != NULL) {
            cgltf_accessor_unpack_floats(pTexCoordAttribute->data, pLoadBuffer, texCoordNumComponent * pTexCoordAttribute->data->count);

            for(cgltf_size v = 0; v < vertexAmount; v++) {
                pInterlacedBuffer[v].texCoord.x = ((float*)pLoadBuffer)[texCoordNumComponent * v + 0];
                pInterlacedBuffer[v].texCoord.y = ((float*)pLoadBuffer)[texCoordNumComponent * v + 1];
            }
        }

        if(pIndices != NULL)
            pVModel[mesh_index].vertexAmount = pIndices->count;
        else
            pVModel[mesh_index].vertexAmount = vertexAmount;

        pVModel[mesh_index].vertexOffset = indexBufferSize;

        v_alloc_static_buffer(this, pIndexedBuffer, indexBufferSize + sizeof(Vertex) * vertexAmount, &pVModel[mesh_index].buffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &pVModel[mesh_index].bufferMemory);

        free(pLoadBuffer);

        (*pModelAmount)++;
    }

    cgltf_free(pModel);
    *ppVModelData = pVModel;

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

void v_record_model_draws(Context *this, VkCommandBuffer commandBuffer, VModelData *pModelData, unsigned numInstances, PushConstantObject *pPushConstantObjects) {
    VkBuffer vertexBuffers[] = {pModelData->buffer};
    VkDeviceSize offsets[] = {pModelData->vertexOffset};

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    if(pModelData->vertexOffset != 0)
        vkCmdBindIndexBuffer(commandBuffer, pModelData->buffer, 0, pModelData->indexType);

    for(unsigned i = 0; i < numInstances; i++) {
        vkCmdPushConstants(commandBuffer, this->vk.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantObject), &pPushConstantObjects[i]);

        if(pModelData->vertexOffset != 0)
            vkCmdDrawIndexed(commandBuffer, pModelData->vertexAmount, 1, 0, 0, 0);
        else
            vkCmdDraw(commandBuffer, pModelData->vertexAmount, 1, 0, 0);
    }
}
