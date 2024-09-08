#include "chunkDrawing.h"

// Reacts to a mouse click event (if any)
// Returns true if the event caused some update
bool ChunkDrawing::handleMouseClicks(
    std::unordered_set<glm::ivec3> *chunkIndexesToAdd) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  static std::unordered_map<int, bool> buttonStates = {
      {GLFW_MOUSE_BUTTON_LEFT, false},
      {GLFW_MOUSE_BUTTON_RIGHT, false},
      {GLFW_MOUSE_BUTTON_MIDDLE, false}};
  bool shouldRedraw = false;
  for (const auto &[button, pressed] : buttonStates) {
    int state = glfwGetMouseButton(Globals::vulkraftApp.window, button);
    if (state == GLFW_PRESS && !pressed) {
      buttonStates[button] = true;
      if (handleMouseClick(button, chunkIndexesToAdd))
        shouldRedraw = true;
    } else if (state == GLFW_RELEASE && pressed) {
      buttonStates[button] = false;
    }
  }
  return shouldRedraw;
}

// Handles a mouse click event
bool ChunkDrawing::handleMouseClick(
    int button, std::unordered_set<glm::ivec3> *chunkIndexesToAdd) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  glm::ivec3 target;
  glm::vec3 norm;
  if (!rayTracePlayer(target, norm))
    return false;
  if (button == GLFW_MOUSE_BUTTON_RIGHT)
    target += norm;
  glm::ivec3 baseChunkIndex = Chunk::findChunkIndex(target);
  auto chunkIt = chunkMap.find(baseChunkIndex);
  if (chunkIt == chunkMap.end())
    return false;
  Chunk *chunk = chunkIt->second;
  glm::ivec3 blockIndex = Chunk::findBlockIndex(target);
  bool actionResult = false;
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    actionResult = chunk->destroyLocal(blockIndex);
  } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    actionResult = chunk->placeLocal(blockIndex);
  } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
    actionResult = chunk->selectBlockType(blockIndex);
  }
  if (actionResult) {
    chunkIndexesToAdd->insert(baseChunkIndex);
    std::vector<Chunk *> toBuild = {chunk};
    bool waterSpreaded =
        (button == GLFW_MOUSE_BUTTON_RIGHT) ||
        spreadWater(chunk, chunk, target, target + glm::ivec3(0, 1, 0));
    const std::vector<glm::ivec3> directions = {
        glm::ivec3(1, 0, 0),
        glm::ivec3(-1, 0, 0),
        glm::ivec3(0, 0, 1),
        glm::ivec3(0, 0, -1),
    };
    for (const auto &direction : directions) {
      glm::ivec3 source = target + direction;
      glm::ivec3 nearChunkIndex = Chunk::findChunkIndex(source);
      glm::ivec3 nearIndex = Chunk::findBlockIndex(source);
      Chunk *neighbor = nullptr;
      if (nearChunkIndex != baseChunkIndex) {
        auto neighborIt = chunkMap.find(nearChunkIndex);
        if (neighborIt != chunkMap.end()) {
          neighbor = neighborIt->second;
          toBuild.push_back(neighbor);
        }
      } else {
        neighbor = chunk;
      }
      if (neighbor) {
        waterSpreaded = waterSpreaded ||
                        spreadWater(chunk, neighbor, blockIndex, nearIndex);
      }
    }
    for (Chunk *c : toBuild) {
      c->build();
    }
    return true;
  }
  return false;
}

// Traces the line of sight of the player
bool ChunkDrawing::rayTracePlayer(glm::ivec3 &target, glm::vec3 &norm) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  Camera camera = player.getCamera();
  glm::vec3 position = camera.getPosition();
  glm::vec3 direction = camera.getDirection() * -1.f;
  return TraceRay::trace(
      [&](glm::ivec3 position) -> bool {
        auto chunkIt = chunkMap.find(Chunk::findChunkIndex(position));
        if (chunkIt != chunkMap.end()) {
          return chunkIt->second->isBlockBreakableGlobal(position);
        }
        return false;
      },
      position, direction, ACTION_RANGE, target, norm);
}

// Tries to spread water from [sourceBlock] to [emptyBlock]
bool ChunkDrawing::spreadWater(Chunk *emptyChunk, Chunk *sourceChunk,
                               glm::ivec3 emptyBlock, glm::ivec3 sourceBlock) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  if (sourceChunk->isBlockWaterLocal(sourceBlock)) {
    emptyChunk->spreadWater(emptyBlock);
    return true;
  }
  return false;
}

// Loads the new visible chunks
bool ChunkDrawing::loadNewVisibleChunks(
    std::unordered_set<glm::ivec3> *chunkIndexesToAdd,
    std::unordered_set<Chunk *> *toBuild) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  static glm::ivec3 oldChunkIndex(-1, 0, -1);
  glm::ivec3 baseChunkIndex =
      Chunk::findChunkIndex(player.getCamera().getPosition());
  if (oldChunkIndex == baseChunkIndex)
    return false;
  oldChunkIndex = baseChunkIndex;
  for (int i = -VIEW_RANGE; i <= VIEW_RANGE; ++i) {
    for (int j = -VIEW_RANGE; j <= VIEW_RANGE; ++j) {
      glm::ivec3 curChunkIndex(baseChunkIndex.x + i * CHUNK_WIDTH,
                               baseChunkIndex.y,
                               baseChunkIndex.z + j * CHUNK_DEPTH);

      if ((chunkMap.find(curChunkIndex) != chunkMap.end()) ||
          (!chunkIndexesToAdd->insert(curChunkIndex).second))
        continue;
      auto *newChunk = new Chunk(curChunkIndex, chunkMap);
      chunkMap[curChunkIndex] = newChunk;
      toBuild->insert(newChunk);
      addNeighborsToBuild(newChunk, toBuild);
    }
  }
  return true;
}

void ChunkDrawing::addNeighborsToBuild(Chunk *newChunk,
                                       std::unordered_set<Chunk *> *toBuild) {
  for (const auto &neighbor : newChunk->getNeighbors()) {
    toBuild->insert(neighbor.second);
  }
}

// Builds the first chunk in [toBuild]
bool ChunkDrawing::buildNextChunk(std::unordered_set<Chunk *> *toBuild) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  if (toBuild->empty())
    return false;
  Chunk *curChunk = *toBuild->begin();
  toBuild->erase(toBuild->begin());
  curChunk->build();
  return true;
}

// Initializes and builds the first chunks
void ChunkDrawing::initializeChunks() {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  const glm::ivec3 baseChunkIndex(0, 0, 0);
  // Create a random device and a random number generator
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, std::numeric_limits<int>::max());
  // Use the random number generator to set the seed
  Chunk::setSeed(dis(gen));
  chunkMap[baseChunkIndex] = new Chunk(baseChunkIndex, chunkMap);
  for (auto &[index, chunk] : chunkMap) {
    chunk->build();
  }
}
// Gets the visible chunks from [chunkMap] and draws them
void ChunkDrawing::drawVisibleChunks() {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  glm::ivec3 baseChunkIndex =
      Chunk::findChunkIndex(player.getCamera().getPosition());
  std::vector<Chunk *> visibleChunks;
  for (int i = -VIEW_RANGE; i <= VIEW_RANGE; ++i) {
    for (int j = -VIEW_RANGE; j <= VIEW_RANGE; ++j) {
      glm::ivec3 curChunkIndex(baseChunkIndex.x + i * CHUNK_WIDTH,
                               baseChunkIndex.y,
                               baseChunkIndex.z + j * CHUNK_DEPTH);
      auto it = chunkMap.find(curChunkIndex);
      if (it != chunkMap.end()) {
        visibleChunks.push_back(it->second);
      }
    }
  }
  clearBuffers();
  for (Chunk *chunk : visibleChunks) {
    drawChunk(chunk);
  }
}

// Clears the global vertex and index buffers
void ChunkDrawing::clearBuffers() {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  Globals::vulkraftApp.vertices.clear();
  Globals::vulkraftApp.indices.clear();
  Globals::vulkraftApp.waterVertices.clear();
  Globals::vulkraftApp.waterIndices.clear();
  Globals::vulkraftApp.waterVertices.resize(1);
  Globals::vulkraftApp.waterIndices.resize(3);
}

// Inserts [newChunk] vertices and indices in the global buffers
void ChunkDrawing::drawChunk(Chunk *newChunk) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  auto &app = Globals::vulkraftApp;
  // Process solid vertices and indices
  processChunkData(newChunk->getVertices(), newChunk->getIndices(),
                   app.vertices, app.indices);
  // Process water vertices and indices
  processChunkData(newChunk->getWaterVertices(), newChunk->getWaterIndices(),
                   app.waterVertices, app.waterIndices);
}

void ChunkDrawing::processChunkData(const std::vector<BlockVertex> &vertices,
                                    const std::vector<uint32_t> &indices,
                                    std::vector<BlockVertex> &globalVertices,
                                    std::vector<uint32_t> &globalIndices) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  size_t prevSizeIndices = globalIndices.size();
  globalIndices.resize(prevSizeIndices + indices.size());
  size_t vertexOffset = globalVertices.size();
  for (size_t i = 0; i < indices.size(); ++i) {
    globalIndices[prevSizeIndices + i] =
        static_cast<unsigned int>(indices[i] + vertexOffset);
  }
  globalVertices.insert(globalVertices.end(), vertices.begin(), vertices.end());
}

void ChunkDrawing::initChunks() {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  initializeChunks();
  drawVisibleChunks();
}

void ChunkDrawing::updateUniform(float &delta,
                                 std::vector<BlockVertex> &vertices,
                                 int &curBuffer) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  static bool shouldRedraw = false;
  static std::unordered_set<glm::ivec3> chunkIndexesToAdd;
  if (!Globals::vulkraftApp.cursorEnabled) {
    player.cursorPositionEventListener(Globals::vulkraftApp.window);
    player.keyEventListener(Globals::vulkraftApp.window, delta);
    if (handleMouseClicks(&chunkIndexesToAdd))
      shouldRedraw = true;
  }
  static std::unordered_set<Chunk *> toBuild;
  if (loadNewVisibleChunks(&chunkIndexesToAdd, &toBuild)) {
    shouldRedraw = true;
  }
  if (buildNextChunk(&toBuild)) {
    shouldRedraw = true;
  } else if (shouldRedraw) {
    drawVisibleChunks();
    if (!vertices.empty()) {
      curBuffer = (curBuffer + 1) % MAX_FRAMES_IN_FLIGHT;
      Globals::vulkraftApp.updateVertexBuffer();
      Globals::vulkraftApp.updateIndexBuffer();
    }
    shouldRedraw = false;
  }
}
