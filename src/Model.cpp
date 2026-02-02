#include "Model.h"

//-----------------------------------------------------------------------------------------------------
// Lee los atributos del modelo de un fichero de texto y los almacena creando submeshes por material
//-----------------------------------------------------------------------------------------------------
void Model::initModel(const char *modelFile) {
   
 // Importa el modelo mediante la librería Assimp
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(modelFile,  
        aiProcess_Triangulate |     
        aiProcess_JoinIdenticalVertices | 
        aiProcess_PreTransformVertices  |
        aiProcess_GenSmoothNormals | 
        aiProcess_CalcTangentSpace | 
        aiProcess_GenUVCoords);
    if(!scene) {
        std::cout << "El fichero " << modelFile << " no se puede abrir." << std::endl;
        std::cin.get();
        exit(1);
    }
  
 // Extraer cada mesh con su material
    for (unsigned int meshIdx = 0; meshIdx < scene->mNumMeshes; meshIdx++) {
        aiMesh *mesh = scene->mMeshes[meshIdx];
        
        // Crear submesh
        SubMesh subMesh;
        
        // Obtener nombre del material
        if (mesh->mMaterialIndex >= 0 && mesh->mMaterialIndex < scene->mNumMaterials) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            aiString name;
            material->Get(AI_MATKEY_NAME, name);
            subMesh.materialName = std::string(name.C_Str());
        } else {
            subMesh.materialName = "default";
        }
        
        // Vectores temporales para esta submesh
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> textureCoords;
        std::vector<unsigned short> indices;
        
        // Cargar vértices, normales y coordenadas de textura
        for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
            positions.push_back(glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z));
            normals.push_back(glm::normalize(glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z)));
            if(mesh->mTextureCoords[0]) 
                textureCoords.push_back(glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y));
            else 
                textureCoords.push_back(glm::vec2(0.0f, 0.0f));
        }
        
        // Cargar índices
        for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for(unsigned int j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }
        
        subMesh.indexCount = indices.size();

        // Crear VAO y VBOs para esta submesh
        glGenVertexArrays(1, &subMesh.vao);
        glGenBuffers(1, &subMesh.vboPositions);
        glGenBuffers(1, &subMesh.vboNormals);
        glGenBuffers(1, &subMesh.vboTextureCoords);
        glGenBuffers(1, &subMesh.eboIndices);
        
        glBindVertexArray(subMesh.vao);
         // Posiciones
            glBindBuffer(GL_ARRAY_BUFFER, subMesh.vboPositions);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*positions.size(), &(positions.front()), GL_STATIC_DRAW);  
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); 
            glEnableVertexAttribArray(0);
         // Normales   
            glBindBuffer(GL_ARRAY_BUFFER, subMesh.vboNormals);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*normals.size(), &(normals.front()), GL_STATIC_DRAW);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0); 
            glEnableVertexAttribArray(1);
         // Texturas
            glBindBuffer(GL_ARRAY_BUFFER, subMesh.vboTextureCoords);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2)*textureCoords.size(), &(textureCoords.front()), GL_STATIC_DRAW);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0); 
            glEnableVertexAttribArray(2);
         // Índices
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subMesh.eboIndices);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short)*indices.size(), &(indices.front()), GL_STATIC_DRAW);
        glBindVertexArray(0);
        
        subMeshes.push_back(subMesh);
    }
}

//--------------------------------
// Renderiza todas las submeshes
//--------------------------------
void Model::renderModel(unsigned long mode) {
    glPolygonMode(GL_FRONT_AND_BACK, mode);
    for(auto& subMesh : subMeshes) {
        glBindVertexArray(subMesh.vao);
        glDrawElements(GL_TRIANGLES, subMesh.indexCount, GL_UNSIGNED_SHORT, (void *)0);
        glBindVertexArray(0);
    }
}

//-----------------------------------
// Destructor de la clase
//-----------------------------------
Model::~Model() {
    for(auto& subMesh : subMeshes) {
        glDeleteVertexArrays(1, &subMesh.vao);
        glDeleteBuffers(1, &subMesh.vboPositions);
        glDeleteBuffers(1, &subMesh.vboNormals);
        glDeleteBuffers(1, &subMesh.vboTextureCoords);
        glDeleteBuffers(1, &subMesh.eboIndices);
    }
}
