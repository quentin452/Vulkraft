#include "chunkDrawing.h"

// Reacts to a mouse click event (if any)
// Returns true if the event caused some update
bool ChunkDrawing::handleMouseClicks(
    std::unordered_set<glm::ivec3> *chunkIndexesToAdd) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  static bool leftPressed = false;
  static bool rightPressed = false;
  static bool middlePressed = false;

  bool shouldRedraw = false;

  if (!leftPressed && !rightPressed && !middlePressed) {
    if (glfwGetMouseButton(Globals::vulkraftApp.window,
                           GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
      leftPressed = true;
      if (handleMouseClick(GLFW_MOUSE_BUTTON_LEFT, chunkIndexesToAdd))
        shouldRedraw = true;
    } else if (glfwGetMouseButton(Globals::vulkraftApp.window,
                                  GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
      rightPressed = true;
      if (handleMouseClick(GLFW_MOUSE_BUTTON_RIGHT, chunkIndexesToAdd))
        shouldRedraw = true;
    } else if (glfwGetMouseButton(Globals::vulkraftApp.window,
                                  GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
      middlePressed = true;
      handleMouseClick(GLFW_MOUSE_BUTTON_MIDDLE, chunkIndexesToAdd);
    }
  }

  if (glfwGetMouseButton(Globals::vulkraftApp.window, GLFW_MOUSE_BUTTON_LEFT) ==
      GLFW_RELEASE) {
    leftPressed = false;
  }
  if (glfwGetMouseButton(Globals::vulkraftApp.window,
                         GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE) {
    rightPressed = false;
  }
  if (glfwGetMouseButton(Globals::vulkraftApp.window,
                         GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_RELEASE) {
    middlePressed = false;
  }

  return shouldRedraw;
}
// If [isRight] is false, destroys the block that the player is looking at
// If [isRight] is true, places a block near the one that the player is
// looking at Returns true if a block was destroyed or placed
bool ChunkDrawing::handleMouseClick(
    int button, std::unordered_set<glm::ivec3> *chunkIndexesToAdd) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  glm::ivec3 target;
  glm::vec3 norm;
  if (!rayTracePlayer(target, norm))
    return false;

  if (button == GLFW_MOUSE_BUTTON_RIGHT)
    target += norm; // The real target is the adjacent block

  glm::ivec3 baseChunkIndex = Chunk::findChunkIndex(target);
  Chunk *chunk = chunkMap.find(baseChunkIndex)->second;
  glm::ivec3 blockIndex = Chunk::findBlockIndex(target);

  if (button == GLFW_MOUSE_BUTTON_LEFT && !chunk->destroyLocal(blockIndex))
    return false;
  if (button == GLFW_MOUSE_BUTTON_RIGHT && !chunk->placeLocal(blockIndex))
    return false;
  if (button == GLFW_MOUSE_BUTTON_MIDDLE)
    return chunk->selectBlockType(blockIndex);
  chunkIndexesToAdd->insert(baseChunkIndex);

  std::vector<Chunk *> toBuild = {chunk};
  bool waterSpreaded =
      button == GLFW_MOUSE_BUTTON_RIGHT ||
      spreadWater(chunk, chunk, target, target + glm::ivec3(0, 1, 0));

  const std::vector<glm::ivec3> directions = {
      glm::ivec3(1, 0, 0),
      glm::ivec3(-1, 0, 0),
      glm::ivec3(0, 0, 1),
      glm::ivec3(0, 0, -1),
  };

  for (auto direction : directions) {
    glm::ivec3 source = target + direction;
    glm::ivec3 nearChunkIndex = Chunk::findChunkIndex(source);
    glm::ivec3 nearIndex = Chunk::findBlockIndex(source);

    Chunk *neighbor;
    if (nearChunkIndex != baseChunkIndex) {
      neighbor = chunkMap.find(nearChunkIndex)->second;
      toBuild.push_back(neighbor);
    } else {
      neighbor = chunk;
    }

    waterSpreaded =
        waterSpreaded || spreadWater(chunk, neighbor, blockIndex, nearIndex);
  }

  for (auto &chunk : toBuild)
    chunk->build();
  return true;
}

// Traces the line of sight of the player and fills [target] and [norm] with
// the details of the hit
// Returns true if a hit occurred
bool ChunkDrawing::rayTracePlayer(glm::ivec3 &target, glm::vec3 &norm) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  Camera camera = player.getCamera();
  glm::vec3 position = camera.getPosition();
  glm::vec3 direction = camera.getDirection() * -1.f;

  return TraceRay::trace(
      [&](glm::ivec3 position) -> bool {
        glm::ivec3 baseChunkIndex = Chunk::findChunkIndex(position);
        Chunk *chunk = chunkMap.find(baseChunkIndex)->second;
        return chunk->isBlockBreakableGlobal(position);
      },
      position, direction, ACTION_RANGE, target, norm);
}

// Tries to spread water from [sourceBlock] to [emptyBlock]
// Returns true if [emptyBlock] was filled
bool ChunkDrawing::spreadWater(Chunk *emptyChunk, Chunk *sourceChunk,
                               glm::ivec3 emptyBlock, glm::ivec3 sourceBlock) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  if (!sourceChunk->isBlockWaterLocal(sourceBlock))
    return false;
  emptyChunk->spreadWater(emptyBlock);
  return true;
}

// Loads the new visible chunks in [chunksIndexesToAdd] and [toBuild]
// Returns true if the player entered a new chunk
bool ChunkDrawing::loadNewVisibleChunks(
    std::unordered_set<glm::ivec3> *chunkIndexesToAdd,
    std::unordered_set<Chunk *> *toBuild) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  static glm::ivec3 oldChunkIndex(
      -1, 0, -1); // Any value different from (0, 0, 0) should be fine
  glm::ivec3 baseChunkIndex =
      Chunk::findChunkIndex(player.getCamera().getPosition());

  if (oldChunkIndex != baseChunkIndex) {
    oldChunkIndex = baseChunkIndex;
    for (int i = -VIEW_RANGE; i <= VIEW_RANGE; ++i) {
      for (int j = -VIEW_RANGE; j <= VIEW_RANGE; ++j) {
        glm::ivec3 curChunkIndex(baseChunkIndex.x + i * CHUNK_WIDTH,
                                 baseChunkIndex.y,
                                 baseChunkIndex.z + j * CHUNK_DEPTH);
        if (chunkMap.find(curChunkIndex) == chunkMap.end()) {
          if (chunkIndexesToAdd->find(curChunkIndex) ==
              chunkIndexesToAdd->end()) {
            chunkIndexesToAdd->insert(curChunkIndex);
            Chunk *newChunk = new Chunk(curChunkIndex, chunkMap);
            chunkMap.insert({curChunkIndex, newChunk});
            toBuild->insert(newChunk);
            std::vector<std::pair<glm::ivec3, Chunk *>> neighbors =
                newChunk->getNeighbors();
            for (auto &c : neighbors) {
              toBuild->insert(c.second);
            }
          }
        }
      }
    }
    return true;
  }
  return false;
}

// Builds the first chunk in [toBuild]
// Returns false if no build occurred
bool ChunkDrawing::buildNextChunk(std::unordered_set<Chunk *> *toBuild) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  if (toBuild->empty())
    return false;

  auto curChunkIter = toBuild->begin();
  Chunk *curChunk = *curChunkIter;
  toBuild->erase(curChunkIter);
  curChunk->build();
  return true;
}

// Initializes and builds the first chunks
void ChunkDrawing::initializeChunks() {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  const glm::ivec3 baseChunkIndex = glm::ivec3(0, 0, 0);

  Chunk::setSeed(rand());
  chunkMap.insert(
      std::pair(baseChunkIndex, new Chunk(baseChunkIndex, chunkMap)));

  for (auto &iter : chunkMap) {
    iter.second->build();
  }
}

// Gets the visible chunks from [chunkMap] and draws them
void ChunkDrawing::drawVisibleChunks() {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  const glm::ivec3 baseChunkIndex =
      Chunk::findChunkIndex(player.getCamera().getPosition());
  std::vector<Chunk *> visibleChunks;

  for (int i = -VIEW_RANGE; i <= VIEW_RANGE; ++i) {
    for (int j = -VIEW_RANGE; j <= VIEW_RANGE; ++j) {
      glm::ivec3 curChunkIndex(baseChunkIndex.x + i * CHUNK_WIDTH,
                               baseChunkIndex.y,
                               baseChunkIndex.z + j * CHUNK_DEPTH);
      auto iter = chunkMap.find(curChunkIndex);
      if (iter != chunkMap.end()) {
        visibleChunks.push_back(iter->second);
      }
    }
  }

  clearBuffers();
  for (auto &iter : visibleChunks) {
    drawChunk(iter);
  }
}

/// Clears the global vertex and index buffers
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
  std::vector<BlockVertex> curVertices = newChunk->getVertices();
  std::vector<uint32_t> curIndices = newChunk->getIndices();

  Globals::vulkraftApp.indices.reserve(Globals::vulkraftApp.indices.size() +
                                       curIndices.size());
  for (int i = 0; i < curIndices.size(); ++i) {
    Globals::vulkraftApp.indices.push_back(
        curIndices[i] + Globals::vulkraftApp.vertices.size());
  }
  Globals::vulkraftApp.vertices.insert(Globals::vulkraftApp.vertices.end(),
                                       curVertices.begin(), curVertices.end());

  std::vector<BlockVertex> curWaterVertices = newChunk->getWaterVertices();
  std::vector<uint32_t> curWaterIndices = newChunk->getWaterIndices();

  Globals::vulkraftApp.waterIndices.reserve(
      Globals::vulkraftApp.waterIndices.size() + curWaterIndices.size());
  for (int i = 0; i < curWaterIndices.size(); ++i) {
    Globals::vulkraftApp.waterIndices.push_back(
        curWaterIndices[i] + Globals::vulkraftApp.waterVertices.size());
  }
  Globals::vulkraftApp.waterVertices.insert(
      Globals::vulkraftApp.waterVertices.end(), curWaterVertices.begin(),
      curWaterVertices.end());
}

void ChunkDrawing::initChunks() {
  initializeChunks();
  drawVisibleChunks();
}

void ChunkDrawing::updateUniform(float &delta,
                                 std::vector<BlockVertex> &vertices,
                                 int &curBuffer) {
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
    if (vertices.size()) {
      curBuffer = (curBuffer + 1) % MAX_FRAMES_IN_FLIGHT;
      Globals::vulkraftApp.updateVertexBuffer();
      Globals::vulkraftApp.updateIndexBuffer();
    }
    shouldRedraw = false;
  }
}