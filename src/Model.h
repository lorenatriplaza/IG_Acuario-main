#ifndef MODEL_H
#define MODEL_H

#include <iostream>
#include <vector>
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define I glm::mat4(1.0)

// Estructura para almacenar información de cada submesh con su material
struct SubMesh {
    unsigned int vao;
    unsigned int vboPositions;
    unsigned int vboNormals;
    unsigned int vboTextureCoords;
    unsigned int eboIndices;
    unsigned int indexCount;
    std::string materialName;
};

class Model {
    
    public:
                        
        void initModel  (const char *modelFile);
        void renderModel(unsigned long mode);
        std::vector<SubMesh>& getSubMeshes() { return subMeshes; }
               
        virtual ~Model();
               
    private:
        
        std::vector<SubMesh> subMeshes;

};

#endif /* MODEL_H */
