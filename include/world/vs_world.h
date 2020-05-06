#pragma once

#include <map>
#include <memory>

#include "renderer/vs_drawable.h"
#include "renderer/vs_shader.h"
#include "world/vs_block.h"

class VSCamera;
class VSCameraController;
class VSChunk;

class VSWorld : public IVSDrawable
{
public:
    VSWorld();

    const VSBlockData* getBlockData(short ID);

    void addDrawable(std::shared_ptr<IVSDrawable> drawable, std::shared_ptr<VSShader> shader);

    void draw(std::shared_ptr<VSWorld> world, std::shared_ptr<VSShader> shader) const override;

    std::shared_ptr<VSCamera> getCamera() const;

    std::shared_ptr<VSCameraController> getCameraController() const;

    glm::vec3 getDirectLightPos() const;

    glm::vec3 getDirectLightColor() const;

private:
    std::map<VSBlockID, VSBlockData*> blockIDtoBlockData;

    std::vector<VSChunk*> loadedChunks;
    std::vector<VSChunk*> activeChunks;

    std::shared_ptr<VSCamera> camera;
    std::shared_ptr<VSCameraController> cameraController;

    std::map<std::shared_ptr<IVSDrawable>, std::shared_ptr<VSShader>> drawables;
};