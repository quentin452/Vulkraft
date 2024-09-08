#pragma once

#include "../blocks/block.hpp"
#include "../structures/structure.hpp"
#include "../utils/perlin_noise.hpp"

#include <atomic>
#include <glm/glm.hpp>
#include <map>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

#include "../camera/camera.hpp"
#include "../chunk/chunk.hpp"
#include "../player/player.hpp"
#include "../raycast/trace-ray.hpp"

#include <ThreadedLoggerForCPP/LoggerThread.hpp>

#include <ThreadedLoggerForCPP/LoggerFileSystem.hpp>
#include <ThreadedLoggerForCPP/LoggerGlobals.hpp>

#include <game_performance_profiler.hpp>

#include "../globals.h"
#include <random>
class ChunkDrawing {
private:
  std::unordered_map<glm::ivec3, Chunk *> chunkMap;

  Camera camera{};

  bool rayTracePlayer(glm::ivec3 &target, glm::vec3 &norm);
  bool handleMouseClick(int button,
                        std::unordered_set<glm::ivec3> *chunkIndexesToAdd);
  bool handleMouseClicks(std::unordered_set<glm::ivec3> *chunkIndexesToAdd);
  // Tries to spread water from [sourceBlock] to [emptyBlock]
  // Returns true if [emptyBlock] was filled
  bool spreadWater(Chunk *emptyChunk, Chunk *sourceChunk, glm::ivec3 emptyBlock,
                   glm::ivec3 sourceBlock);
  // Loads the new visible chunks in [chunksIndexesToAdd] and [toBuild]
  // Returns true if the player entered a new chunk
  bool loadNewVisibleChunks(std::unordered_set<glm::ivec3> *chunkIndexesToAdd,
                            std::unordered_set<Chunk *> *toBuild);

  // Builds the first chunk in [toBuild]
  // Returns false if no build occurred
  bool buildNextChunk(std::unordered_set<Chunk *> *toBuild);

  // Initializes and builds the first chunks
  void initializeChunks();
  // Gets the visible chunks from [chunkMap] and draws them
  void drawVisibleChunks();

  // Inserts [newChunk] vertices and indices in the global buffers
  void drawChunk(Chunk *newChunk);
  void addNeighborsToBuild(Chunk *newChunk,
                           std::unordered_set<Chunk *> *toBuild);
  void processChunkData(const std::vector<BlockVertex> &vertices,
                        const std::vector<uint32_t> &indices,
                        std::vector<BlockVertex> &globalVertices,
                        std::vector<uint32_t> &globalIndices);
  const int VIEW_RANGE = 2;
  const int ACTION_RANGE = 3;

public:
  Player player{std::ref(camera), std::ref(chunkMap)};

  void initChunks();
  void updateUniform(float &delta, std::vector<BlockVertex> &vertices,
                     int &curBuffer);

  /// Clears the global vertex and index buffers
  void clearBuffers();
};