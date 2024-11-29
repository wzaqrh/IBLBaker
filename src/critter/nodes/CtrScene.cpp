//------------------------------------------------------------------------------------//
//                                                                                    //
//               _________        .__  __    __                                       //
//               \_   ___ \_______|__|/  |__/  |_  ___________                        //
//               /    \  \/\_  __ \  \   __\   __\/ __ \_  __ \                       //
//               \     \____|  | \/  ||  |  |  | \  ___/|  | \/                       //
//                \______  /|__|  |__||__|  |__|  \___  >__|                          //
//                       \/                           \/                              //
//                                                                                    //
//    Critter is provided under the MIT License(MIT)                                  //
//    Critter uses portions of other open source software.                            //
//    Please review the LICENSE file for further details.                             //
//                                                                                    //
//    Copyright(c) 2015 Matt Davidson                                                 //
//                                                                                    //
//    Permission is hereby granted, free of charge, to any person obtaining a copy    //
//    of this software and associated documentation files(the "Software"), to deal    //
//    in the Software without restriction, including without limitation the rights    //
//    to use, copy, modify, merge, publish, distribute, sublicense, and / or sell     //
//    copies of the Software, and to permit persons to whom the Software is           //
//    furnished to do so, subject to the following conditions :                       //
//                                                                                    //
//    1. Redistributions of source code must retain the above copyright notice,       //
//    this list of conditions and the following disclaimer.                           //
//    2. Redistributions in binary form must reproduce the above copyright notice,    //
//    this list of conditions and the following disclaimer in the                     //
//    documentation and / or other materials provided with the distribution.          //
//    3. Neither the name of the copyright holder nor the names of its                //
//    contributors may be used to endorse or promote products derived                 //
//    from this software without specific prior written permission.                   //
//                                                                                    //
//    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      //
//    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        //
//    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE      //
//    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          //
//    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   //
//    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN       //
//    THE SOFTWARE.                                                                   //
//                                                                                    //
//------------------------------------------------------------------------------------//
#include <CtrScene.h>
#include <CtrMesh.h>
#include <CtrEntity.h>
#include <CtrMaterial.h>
#include <CtrIndexedMesh.h>
#include <CtrShaderMgr.h>
#include <CtrMaterial.h>
#include <CtrIBLProbe.h>
#include <CtrCamera.h>
#include <CtrBrdf.h>
#include <Ctrimgui.h>

#if IBL_USE_ASS_IMP_AND_FREEIMAGE
// Assimp
#include <Importer.hpp>
#include <scene.h>
#include <postprocess.h>
#else
#include <tiny_obj_loader.h>
#endif


namespace Ctr
{
namespace
{
//
// Windows only.
// [TODO] Will need abstraction for linux / osx.
//
void findFiles(std::wstring path, std::wstring pattern, 
               std::vector<std::string>& files)
{
    WIN32_FIND_DATA ffd;
    HANDLE hFind;

    hFind = FindFirstFileEx((path + L"\\" + pattern).c_str(), // File Name
        FindExInfoStandard,    // information Level
        &ffd,                  // information buffer
        FindExSearchNameMatch, // 
        NULL,                  // search Criteria
        0                      // reserved
        );

    if (hFind == INVALID_HANDLE_VALUE)
    {
        DWORD le = GetLastError();

        if (le != ERROR_FILE_NOT_FOUND)
        {
            LOG("Invalid File Handle, err code: " << le);
            return;
        }
    }

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;

            std::wstring fileName = ffd.cFileName;
            files.push_back(std::string(fileName.begin(), fileName.end()));

        } while (FindNextFile(hFind, &ffd));
        FindClose(hFind);
    }

    hFind = FindFirstFileEx(
        (path + L"\\*").c_str(), // File Name
        FindExInfoStandard,    // information Level
        &ffd,                  // information buffer
        FindExSearchLimitToDirectories, // only Directories
        NULL,                  // search Criteria
        0                      // reserved
        );

    if (hFind == INVALID_HANDLE_VALUE) {
        DWORD le = GetLastError();
        printf("Invalid File Handle, err code: %d\n", le);
        return;
    }

    do 
    {
        std::wstring dirName = ffd.cFileName;
        if (dirName == L"." || dirName == L"..")
            continue;

        if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            continue;

        findFiles(path + L"\\" + ffd.cFileName, pattern, files);

    } while (FindNextFile(hFind, &ffd));

    FindClose(hFind);
}

std::string trimPathName(const std::string& filePath)
{
    // Fuck I hate windows pathing...
    size_t index = filePath.rfind('\\');
    if (index != std::string::npos)
    { 
        return filePath.substr(0, index + 1);
    }
    return std::string("");
}

std::string trimFileName(const std::string& filePath)
{
    size_t index = filePath.rfind('\\');
    if (index != std::string::npos && index < filePath.length() - 1)
    { 
        std::string fileName = filePath.substr((index + 1), filePath.length() - (index+1));
        return fileName;
    }
    else
    {
        index = filePath.rfind('/');
        if (index != std::string::npos && index < filePath.length() - 1)
        { 
            return filePath.substr(index + 1, filePath.length()-(index+1));
        }
    }
    return filePath;
}

// Returns true if a file exits
bool fileExists(const std::string& filePath)
{
    if (filePath.length() == 0)
        return false;

    std::wstring filePathW(filePath.begin(), filePath.end());
    uint32_t fileAttr = GetFileAttributes(filePathW.c_str());
    if (fileAttr == INVALID_FILE_ATTRIBUTES)
    { 
        LOG("FilePath " << filePath)
        return false;
    }
    return true;
}

// Returns the name of the file given the path (extension included)
std::string fileName(const std::string& filePath)
{
    size_t idx = filePath.rfind('\\');
    if (idx != std::string::npos && idx < filePath.length() - 1)
        return filePath.substr(idx + 1);
    else
    {
        idx = filePath.rfind('/');
        if (idx != std::string::npos && idx < filePath.length() - 1)
            return filePath.substr(idx + 1);
        else
            return filePath;
    }
}

// Returns the given file path, minus the extension
std::string filePathWithoutExtension(const std::string& path)
{
    std::string filePath(path);
    size_t idx = filePath.rfind('.');
    if (idx != std::string::npos)
        return filePath.substr(0, idx);
    else
        return std::string("");
}

// Returns the name of the file given the path, minus the extension
std::string GetFileNameWithoutExtension(const std::string& filePath)
{
    std::string tmp = fileName(filePath);
    return filePathWithoutExtension(tmp);
}

}

Scene::Scene(Ctr::IDevice* device) : 
    Ctr::RenderNode(device),
    _camera(nullptr),
    _activeBrdfProperty(nullptr),
    _brdfType(nullptr)
{
    _camera = new Ctr::Camera(_device);
    loadBrdfs();
}

Scene::~Scene()
{
    if (_camera)
    {
        safedelete(_camera);
    }

    for (auto it = _entities.begin(); it != _entities.end(); it++)
    {
        Ctr::Entity* entity = *it;
        safedelete(entity);
    }
    _entities.clear();

    for (auto it = _probes.begin(); it != _probes.end(); it++)
    {
        Ctr::IBLProbe* probe = *it;
        safedelete(probe);
    }
    _probes.clear();

    for (auto it = _materials.begin(); it != _materials.end(); it++)
    {
        Ctr::Material* material = *it;
        safedelete(material);
    }
    _materials.clear();

    for (auto it = _brdfCache.begin(); it != _brdfCache.end(); it++)
    {
        Ctr::Brdf* brdf = *it;
        safedelete(brdf);
    }
    _brdfCache.clear();
}

bool
Scene::loadBrdfs()
{
    std::vector<std::string> brdfHeaders;
    findFiles(L"data/shadersD3D11/", L"*.brdf", brdfHeaders);

    // Load the brdfs
    for (auto it = brdfHeaders.begin(); it != brdfHeaders.end(); it++)
    {
        Ctr::Brdf* brdf = new Ctr::Brdf(_device);
        if (brdf->load(*it))
        {
            _brdfCache.push_back(brdf);
        }
        else
        {
            LOG("Failed to load brdf " << brdf->name())
        }
    }

    // Build the enum for display
    uint32_t brdfId = 0;
    std::vector<ImguiEnumVal> _brdfEnumValues;
    _brdfEnumValues.reserve(_brdfCache.size());
    for (auto it = _brdfCache.begin(); it != _brdfCache.end(); it++, brdfId++)
    {
        ImguiEnumVal enumValue;
        enumValue.value = brdfId;
        enumValue.label = (*it)->name().c_str();
        _brdfEnumValues.push_back(enumValue);

    }
    _brdfType = new EnumTweakType(&_brdfEnumValues[0], (uint32_t)(_brdfEnumValues.size()), "BrdfType");
    _activeBrdfProperty = new IntProperty(this, "Brdf", new TweakFlags(_brdfType, "Brdf"));
    _activeBrdfProperty->set(0);

    return _brdfCache.size() > 0;
}

IntProperty*
Scene::activeBrdfProperty()
{
    return _activeBrdfProperty;
}

const Brdf*
Scene::activeBrdf() const
{
    return _brdfCache[_activeBrdfProperty->get()];
}


#if IBL_USE_ASS_IMP_AND_FREEIMAGE

Entity*
Scene::load(const std::string& meshFilePathName,
const std::string& userMaterialPathName)
{
    Assimp::Importer importer;
    uint32_t flags = aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_PreTransformVertices |
        aiProcess_FlipUVs;
    const aiScene* scene = importer.ReadFile(meshFilePathName, flags);

    if (scene == nullptr)
    {
        LOG("Failed to load scene " << meshFilePathName);
        return nullptr;
    }

    if (scene->mNumMeshes == 0)
    {
        LOG("Failed to load any meshes " << meshFilePathName);
        return nullptr;
    }

    bool implicitlyGenerateMaterials = false;
    if (scene->mNumMaterials == 0)
    {
        implicitlyGenerateMaterials = true;
    }

    Ctr::Entity* entity = new Ctr::Entity(_device);
    entity->setName(meshFilePathName);

    for (size_t meshId = 0; meshId < scene->mNumMeshes; meshId++)
    {
        Ctr::IndexedMesh* mesh = new Ctr::IndexedMesh(_device);
        mesh->setName(scene->mMeshes[meshId]->mName.C_Str());
        mesh->load(scene->mMeshes[meshId]);
        Material * material = new Material(_device);

        if (userMaterialPathName.length() > 0)
        {
            if (!material->load(userMaterialPathName))
            {
                delete material;
                throw (std::runtime_error("Can't load user material " +
                    userMaterialPathName));
            }
        }
        else if (implicitlyGenerateMaterials)
        {
            // TODO: Setup default based on passed in material.
            // Related to very, very old code (2003).
            assert(0);
        }
        else
        {
            size_t materialId = scene->mMeshes[meshId]->mMaterialIndex;
            const aiMaterial& mat = *scene->mMaterials[materialId];


            // Setup material. This is a little braindead, but it
            // is good enough for the purposes of this demo.
            material->textureGammaProperty()->set(2.2f);
            material->setShaderName("PBRDebug");
            material->setTechniqueName("Default");
            material->addPass("color");

            // Thank you DCC tool for this. Meah.
            char name[512];
            memset(name, 0, sizeof(char) * 512);
            uint32_t nameLength = 512;
            mat.Get("?mat.name", 0, 0,  name, &nameLength);
            material->setName(name);
            material->twoSidedProperty()->set(true);

            //
            // Load textures
            //
            std::string assetPath = trimPathName(meshFilePathName);
            LOG("asset path " << assetPath)
                aiString textureFilePath;
            if (mat.GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath) == aiReturn_SUCCESS)
            {
                std::string mapFilePathName = assetPath + trimFileName(std::string(textureFilePath.C_Str()));
                material->setAlbedoMap(mapFilePathName);
                // Retarded necessity. ArseImp doesn't load material or mesh names.
                material->setName(mapFilePathName);
            }
            else
            {
                LOG("Could not find albedo map for " << meshFilePathName);
            }

            if (mat.GetTexture(aiTextureType_NORMALS, 0, &textureFilePath) == aiReturn_SUCCESS ||
                mat.GetTexture(aiTextureType_HEIGHT, 0, &textureFilePath) == aiReturn_SUCCESS)
            {
                std::string mapFilePathName = assetPath + trimFileName(std::string(textureFilePath.C_Str()));
                material->setNormalMap(mapFilePathName);
            }
            else
            {
                LOG("Could not find normal map for " << meshFilePathName);
            }

            if (mat.GetTexture(aiTextureType_SPECULAR, 0, &textureFilePath) == aiReturn_SUCCESS)
            {
                std::string mapFilePathName = assetPath + trimFileName(std::string(textureFilePath.C_Str()));
                material->setSpecularRMCMap(mapFilePathName);
            }
            else
            {
                LOG("Could not find specular map for " << meshFilePathName);
            }
        }

        mesh->setMaterial(material);
        entity->addMesh(mesh);
        addMesh(mesh);
        _materials.insert(material);
    }

    // Resolve shaders
    _device->shaderMgr()->resolveShaders(entity);
    // Setup the material.
    _entities.insert(entity);

    return entity;
}

#else
Entity*
Scene::load(const std::string& meshFilePathName, 
            const std::string& userMaterialPathName)
{
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;



    size_t materialBaseIndex = meshFilePathName.rfind("/");
    if (materialBaseIndex == std::string::npos)
        materialBaseIndex = meshFilePathName.rfind("\\");

    std::string materialBasePath;
    if (materialBaseIndex != std::string::npos)
    {
        materialBasePath = meshFilePathName.substr(0, materialBaseIndex);
        LOG("Have mesh base index " << materialBasePath);
    }

    std::string error = tinyobj::LoadObj(shapes, materials, meshFilePathName.c_str(), materialBasePath.length() ? materialBasePath.c_str() : nullptr);

    if (error.length() > 0)
    {
        LOG("Failed to load any meshes " << error);
        return nullptr;
    }

    if(shapes.size() == 0)
    {
        LOG("Failed to load any meshes " << meshFilePathName);
        return nullptr;
    }
    
    bool implicitlyGenerateMaterials = false;
    if(materials.size() == 0)
    {
        implicitlyGenerateMaterials = true;
    }
    
    Ctr::Entity* entity = new Ctr::Entity(_device);
    entity->setName(meshFilePathName);
    
    for (size_t meshId = 0; meshId < shapes.size(); meshId++)
    {
        Ctr::IndexedMesh* mesh = new Ctr::IndexedMesh(_device);
        mesh->setName(shapes[meshId].name);
        mesh->load(&shapes[meshId]);
        Material * material = new Material(_device);
    


        if (userMaterialPathName.length() > 0)
        {
            if (!material->load(userMaterialPathName))
            {
                delete material;
                throw (std::runtime_error("Can't load user material " + 
                                          userMaterialPathName));
            }
        }
        else if (implicitlyGenerateMaterials)
        {
            // TODO: Setup default based on passed in material.
        }
        else
        {
            tinyobj::material_t* mat = &materials[meshId];

            // Setup material. This is a little braindead, but it
            // is good enough for the purposes of this demo.
            material->textureGammaProperty()->set(2.2f);
            material->setShaderName("PBRDebug");
            material->setTechniqueName("Default");
            material->addPass("color");

            //
            // Load textures
            //
            std::string assetPath = trimPathName(meshFilePathName);
            LOG("asset path " << assetPath)
            
            if (mat->diffuse_texname.length())
            {
                std::string mapFilePathName = assetPath + (mat->diffuse_texname);
                material->setAlbedoMap(mapFilePathName);
            }
            if (mat->normal_texname.length())
            {
                std::string mapFilePathName = assetPath + (mat->normal_texname);
                material->setNormalMap(mapFilePathName);
            }
            if (mat->specular_texname.length())
            {
                std::string mapFilePathName = assetPath + (mat->specular_texname);
                material->setSpecularRMCMap(mapFilePathName);
            }
        }

        mesh->setMaterial(material);
        entity->addMesh(mesh);
        addMesh(mesh);
        _materials.insert(material);
    }
    
    _device->shaderMgr()->resolveShaders(entity);
    _entities.insert(entity);
    
    return entity;
}
#endif

void
Scene::destroy(Entity* entity)
{
    auto entityIt = _entities.find(entity);

    if (entityIt != _entities.end())
    {
        // Remove from the shader library.
        _device->shaderMgr()->remove(entity);

        for (std::map<std::string, std::vector<Ctr::Mesh*> >::iterator passIt =
            _meshesByPass.begin(); passIt != _meshesByPass.end(); passIt++)
        {
            for (auto entityMeshIt = entity->meshes().begin(); entityMeshIt != entity->meshes().end(); entityMeshIt++)
            {
                auto meshIt = std::find(passIt->second.begin(), passIt->second.end(), *entityMeshIt);
                if (meshIt != passIt->second.end())
                {
                    passIt->second.erase(meshIt);
                }
            }
        }
 
        auto entityIt = _entities.find(entity);
        if (entityIt != _entities.end())
            _entities.erase(entityIt);
        // If it did not exist in this list,
        // I cannot account for it.
        delete entity;
    }
}

#if IBL_USE_ASS_IMP_AND_FREEIMAGE

Entity*
Scene::load(Ctr::IDevice* device,
const std::string& meshFilePathName)
{
    Assimp::Importer importer;
    uint32_t flags = aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_PreTransformVertices |
        aiProcess_FlipUVs;
    //aiProcess_FlipWindingOrder ;
    const aiScene* scene = importer.ReadFile(meshFilePathName, flags);

    if (scene == nullptr)
    {
        LOG("Failed to load scene " << meshFilePathName);
        return nullptr;
    }

    if (scene->mNumMeshes == 0)
    {
        LOG("Failed to load any meshes " << meshFilePathName);
        return nullptr;
    }

    bool implicitlyGenerateMaterials = false;
    if (scene->mNumMaterials == 0)
    {
        implicitlyGenerateMaterials = true;
    }

    Ctr::Entity* entity = new Ctr::Entity(device);
    entity->setName(meshFilePathName);

    for (size_t meshId = 0; meshId < scene->mNumMeshes; meshId++)
    {
        Ctr::IndexedMesh* mesh = new Ctr::IndexedMesh(device);
        mesh->setName(scene->mMeshes[meshId]->mName.C_Str());
        mesh->load(scene->mMeshes[meshId]);
        Material * material = new Material(device);

        material->twoSidedProperty()->set(true);
        mesh->setMaterial(material);
        entity->addMesh(mesh);
    }

    return entity;
}

#else
Entity*
Scene::load(Ctr::IDevice* device,
            const std::string& meshFilePathName)
{
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string error = tinyobj::LoadObj(shapes, materials, meshFilePathName.c_str(), nullptr);

    if (error.length() > 0)
    {
        LOG_CRITICAL("Could not load any meshes from " << meshFilePathName)
        return nullptr;
    }

    Ctr::Entity* entity = new Ctr::Entity(device);
    entity->setName(meshFilePathName);

    for (size_t meshId = 0; meshId < shapes.size(); meshId++)
    {
        Ctr::IndexedMesh* mesh = new Ctr::IndexedMesh(device);
        mesh->setName(shapes[meshId].name);
        mesh->load(&shapes[meshId]);
        entity->addMesh(mesh);
    }

    return entity;
}
#endif

void
Scene::update()
{
    for (auto it = _probes.begin(); it != _probes.end(); it++)
    {
        (*it)->update();
    }

    _brdfCache[_activeBrdfProperty->get()]->compute();

}

const std::vector<IBLProbe*>&
Scene::probes() const
{
    return _probes;
}

Ctr::IBLProbe*
Scene::addProbe()
{
    IBLProbe * probe = new IBLProbe(_device);
    _probes.push_back(probe);
    return probe;
}

const Camera *
Scene::camera() const
{
    return _camera;
}

Camera *
Scene::camera()
{
    return _camera;
}

const std::vector<Ctr::Mesh*>& 
Scene::meshesForPass(const std::string& passName) const
{
    auto meshPassIt = _meshesByPass.find(passName);
    if (meshPassIt != _meshesByPass.end())
    {
        return meshPassIt->second;
    }
    else
    {
        static std::vector<Ctr::Mesh*> nullSet;
        return nullSet;
    }
}

void
Scene::addMesh(Mesh* mesh)
{
    Ctr::Material* material = mesh->material();
    const std::vector<std::string>& passes = material->passes();

    // Cache meshes by contributing pass name.
    addToPass("all", mesh);
    for (auto passIt = passes.begin(); passIt != passes.end(); passIt++)
    {
        addToPass(*passIt, mesh);
    }
}

void
Scene::addToPass(const std::string& passName, Mesh* mesh)
{
    auto meshPassIt = _meshesByPass.find(passName);
    if (meshPassIt == _meshesByPass.end())
    {
        _meshesByPass.insert(std::make_pair(passName, std::vector<Ctr::Mesh*>()));
        meshPassIt = _meshesByPass.find(passName);
    }
    meshPassIt->second.push_back(mesh);
}

}