#include <array>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <unordered_set>

#include <iostream>

#include "../blocks/block.hpp"
#include "../structures/structure.hpp"
#include "../utils/enums.hpp"
#include "../utils/perlin_noise.hpp"
#include "chunk.hpp"

#include <ThreadedLoggerForCPP/LoggerThread.hpp>

#include <ThreadedLoggerForCPP/LoggerFileSystem.hpp>
#include <ThreadedLoggerForCPP/LoggerGlobals.hpp>

#include <game_performance_profiler.hpp>
siv::PerlinNoise::seed_type Chunk::seed = 0;
siv::PerlinNoise Chunk::perlin{0};

/* singleton block definition */
const Air *AIR = new Air();
const Dirt *DIRT = new Dirt();
const Grass *GRASS = new Grass();
const WoodLog *WOOD_LOG = new WoodLog();
const WoodPlank *WOOD_PLANK = new WoodPlank();
const Stone *STONE = new Stone();
const Cobblestone *COBBLESTONE = new Cobblestone();
const Sand *SAND = new Sand();
const Bedrock *BEDROCK = new Bedrock();
const Leaves *LEAVES = new Leaves();
const Water *WATER = new Water();
const Bush *BUSH = new Bush();
const BlackWool *BLACK_WOOL = new BlackWool();
const WhiteWool *WHITE_WOOL = new WhiteWool();
/* -------------------------  */

Block::Block() { type = (BlockType *)AIR; }

BlockFace *Block::getFace(Direction dir) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  if (type->isDiagonal()) {
    if (diagonals.count(dir) == 0)
      return NULL;
    return &diagonals[dir];
  }
  if (faces.count(dir) == 0)
    return NULL;
  return &faces[dir];
}

VkVertexInputBindingDescription BlockVertex::getBindingDescription() {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  VkVertexInputBindingDescription bindingDescription{};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(BlockVertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 4>
BlockVertex::getAttributeDescriptions() {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[0].offset = offsetof(BlockVertex, pos);
  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[1].offset = offsetof(BlockVertex, norm);
  attributeDescriptions[2].binding = 0;
  attributeDescriptions[2].location = 2;
  attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[2].offset = offsetof(BlockVertex, tex);
  attributeDescriptions[3].binding = 0;
  attributeDescriptions[3].location = 3;
  attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[3].offset = offsetof(BlockVertex, mat);
  return attributeDescriptions;
}

bool shouldSeeFace(Block self, Block other) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  return !other.type->isOpaque &&
         (!self.type->shouldBlend() || self.type != other.type);
}
std::vector<Direction> Chunk::getVisibleFaces(int x, int y, int z,
                                              bool opaqueOnly) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__);
  Block block = blocks[x][y][z];
  // Early exit if block is not visible or blending conditions are not met
  if (!block.type->isVisible() || (opaqueOnly && block.type->shouldBlend()) ||
      (!opaqueOnly && !block.type->shouldBlend())) {
    return {};
  }
  std::vector<Direction> faces;
  auto checkFace = [&](int nx, int ny, int nz, Direction dir) {
    if (nx < 0 || ny < 0 || nz < 0 || nx >= CHUNK_WIDTH || ny >= CHUNK_HEIGHT ||
        nz >= CHUNK_DEPTH) {
      glm::ivec3 adjChunkPos = coordinates;
      if (nx < 0) {
        adjChunkPos.x -= CHUNK_WIDTH;
        nx = CHUNK_WIDTH - 1;
      }
      if (nx >= CHUNK_WIDTH) {
        adjChunkPos.x += CHUNK_WIDTH;
        nx = 0;
      }
      if (ny < 0) {
        adjChunkPos.y -= CHUNK_HEIGHT;
        ny = CHUNK_HEIGHT - 1;
      }
      if (ny >= CHUNK_HEIGHT) {
        adjChunkPos.y += CHUNK_HEIGHT;
        ny = 0;
      }
      if (nz < 0) {
        adjChunkPos.z -= CHUNK_DEPTH;
        nz = CHUNK_DEPTH - 1;
      }
      if (nz >= CHUNK_DEPTH) {
        adjChunkPos.z += CHUNK_DEPTH;
        nz = 0;
      }
      auto iter = chunkMap.find(adjChunkPos);
      if (iter != chunkMap.end() &&
          shouldSeeFace(block, iter->second->blocks[nx][ny][nz])) {
        faces.push_back(dir);
      }
    } else if (shouldSeeFace(block, blocks[nx][ny][nz])) {
      faces.push_back(dir);
    }
  };
  // Check border faces if SHOW_CHUNK_BORDER is true
  if (SHOW_CHUNK_BORDER) {
    if (x == 0)
      faces.push_back(Direction::West);
    if (y == 0)
      faces.push_back(Direction::Down);
    if (z == 0)
      faces.push_back(Direction::South);
    if (x == CHUNK_WIDTH - 1)
      faces.push_back(Direction::East);
    if (y == CHUNK_HEIGHT - 1)
      faces.push_back(Direction::Up);
    if (z == CHUNK_DEPTH - 1)
      faces.push_back(Direction::North);
  }
  // Check adjacent blocks
  checkFace(x - 1, y, z, Direction::West);
  checkFace(x + 1, y, z, Direction::East);
  checkFace(x, y - 1, z, Direction::Down);
  checkFace(x, y + 1, z, Direction::Up);
  checkFace(x, y, z - 1, Direction::South);
  checkFace(x, y, z + 1, Direction::North);
  return faces;
}
void Chunk::buildBlockFace(int x, int y, int z, Direction dir,
                           bool opaqueOnly) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__);
  Block block = blocks[x][y][z];
  BlockFace *face = block.getFace(dir);
  if (!face)
    return;
  glm::ivec3 pos = coordinates + glm::ivec3(x, y, z);
  glm::vec3 mat = block.type->getMaterialSettings();
  // Helper lambda to add vertices and indices
  auto addFaceData = [&](auto &vertexArray, auto &indexArray,
                         bool reversed = false) {
    int index = vertexArray.size();
    vertexArray.push_back({pos + face->a, face->norm,
                           block.type->getTextureOffset(dir, face->a), mat});
    vertexArray.push_back({pos + face->b, face->norm,
                           block.type->getTextureOffset(dir, face->b), mat});
    vertexArray.push_back({pos + face->c, face->norm,
                           block.type->getTextureOffset(dir, face->c), mat});
    vertexArray.push_back({pos + face->d, face->norm,
                           block.type->getTextureOffset(dir, face->d), mat});
    if (reversed) {
      indexArray.push_back(index + 0);
      indexArray.push_back(index + 2);
      indexArray.push_back(index + 1);
      indexArray.push_back(index + 0);
      indexArray.push_back(index + 3);
      indexArray.push_back(index + 2);
    } else {
      indexArray.push_back(index + 0);
      indexArray.push_back(index + 1);
      indexArray.push_back(index + 2);
      indexArray.push_back(index + 0);
      indexArray.push_back(index + 2);
      indexArray.push_back(index + 3);
    }
  };
  // Add face data based on the opaqueOnly flag
  if (opaqueOnly) {
    addFaceData(vertices, indices);
  } else {
    addFaceData(waterVertices, waterIndices);
    // Add the opposite face for water to avoid culling
    addFaceData(waterVertices, waterIndices, true);
  }
}
void Chunk::buildBlock(int x, int y, int z) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  std::vector<Direction> visibleFacesOpaque = getVisibleFaces(x, y, z, true);
  std::vector<Direction> visibleFacesNonOpaque =
      getVisibleFaces(x, y, z, false);
  for (const Direction &dir : visibleFacesOpaque) {
    buildBlockFace(x, y, z, dir, true);
  }
  for (const Direction &dir : visibleFacesNonOpaque) {
    buildBlockFace(x, y, z, dir, false);
  }
}

void Chunk::buildStructure(StructureMeta *meta) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  for (int i = 0; i < meta->size; i++) {
    glm::ivec3 pos = meta->coords[i];
    BlockType *type = meta->types[i];
    if (type->isOpaque || blocks[pos.x][pos.y][pos.z].type == (BlockType *)AIR)
      blocks[pos.x][pos.y][pos.z].type = meta->types[i];
  }
}

int Chunk::sampleHeight(int x, int z, float depth) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  static const int octaves = 8;
  int minY = 8;
  float varY = 240. * depth;
  float dx = (float)(x + coordinates.x) / (100.);
  float dz = (float)(z + coordinates.z) / (100.);
  const float noise = varY * perlin.normalizedOctave2D_01(dx, dz, octaves);
  return std::clamp(minY + (int)noise, 0, CHUNK_HEIGHT - 1);
}

void Chunk::initTerrain() {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__);
  // Precompute the sample height values to avoid multiple calls
  std::vector<int> midHeights(CHUNK_WIDTH * CHUNK_DEPTH);
  std::vector<int> maxHeights(CHUNK_WIDTH * CHUNK_DEPTH);
  for (int x = 0; x < CHUNK_WIDTH; ++x) {
    for (int z = 0; z < CHUNK_DEPTH; ++z) {
      int index = x * CHUNK_DEPTH + z;
      midHeights[index] = sampleHeight(x, z, 0.4);
      maxHeights[index] = sampleHeight(x, z, 1.0);
    }
  }
  for (int x = 0; x < CHUNK_WIDTH; ++x) {
    for (int z = 0; z < CHUNK_DEPTH; ++z) {
      int midY = midHeights[x * CHUNK_DEPTH + z];
      int maxY = maxHeights[x * CHUNK_DEPTH + z];
      // Set bedrock at the bottom layer
      blocks[x][0][z].type = (BlockType *)BEDROCK;
      // Fill stone
      for (int y = 1; y < midY; ++y) {
        blocks[x][y][z].type = (BlockType *)STONE;
      }
      // Fill dirt
      for (int y = midY; y < maxY; ++y) {
        blocks[x][y][z].type = (BlockType *)DIRT;
      }
      // Set top layer based on height
      blocks[x][maxY][z].type =
          (maxY < WATER_LEVEL) ? (BlockType *)SAND : (BlockType *)GRASS;
      // Fill water above the terrain
      for (int y = maxY + 1; y <= WATER_LEVEL; ++y) {
        blocks[x][y][z].type = (BlockType *)WATER;
      }
    }
  }
}

void Chunk::initPlants() {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__);
  for (int x = 2; x < CHUNK_WIDTH - 2; ++x) {
    for (int z = 2; z < CHUNK_DEPTH - 2; ++z) {
      double noise = (static_cast<double>(rand()) / RAND_MAX) * 2.0 - 1.0;
      int y = sampleHeight(x, z, 1.0);
      if (blocks[x][y][z].type == (BlockType *)GRASS) {
        if (noise < -0.95) {
          blocks[x][y + 1][z].type = (BlockType *)BUSH;
        } else if (noise > 0.95) {
          glm::ivec3 base(x, y + 1, z);
          StructureMeta meta;
          Tree::generate(&meta, base);
          buildStructure(&meta);
        }
      }
    }
  }
}

void Chunk::initLogo() {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  glm::ivec3 base(0, 200, 0);
  StructureMeta meta;
  Logo::generate(&meta, base);
  buildStructure(&meta);
}

Chunk::Chunk(glm::ivec3 pos, const std::unordered_map<glm::ivec3, Chunk *> &m)
    : coordinates(pos), chunkMap(m) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  initTerrain();
  initPlants();
  if (pos == glm::ivec3(0)) {
    initLogo();
  }
}

Chunk::Chunk(int x, int y, int z,
             const std::unordered_map<glm::ivec3, Chunk *> &m)
    : Chunk(glm::ivec3(x, y, z), m) {}

std::vector<BlockVertex> Chunk::getVertices() { return vertices; }

std::vector<uint32_t> Chunk::getIndices() { return indices; }

std::vector<BlockVertex> Chunk::getWaterVertices() { return waterVertices; }

std::vector<uint32_t> Chunk::getWaterIndices() { return waterIndices; }

void Chunk::build() {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  vertices.clear();
  indices.clear();
  waterVertices.clear();
  waterIndices.clear();
  for (int x = 0; x < CHUNK_WIDTH; ++x) {
    for (int y = 0; y < CHUNK_HEIGHT; ++y) {
      for (int z = 0; z < CHUNK_DEPTH; ++z) {
        buildBlock(x, y, z);
      }
    }
  }
}

void Chunk::clear() {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  vertices.clear();
  indices.clear();
}

std::vector<std::pair<glm::ivec3, Chunk *>> Chunk::getNeighbors() {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  std::vector<std::pair<glm::ivec3, Chunk *>> neighbors;
  const int xOff[] = {CHUNK_WIDTH, -CHUNK_WIDTH};
  const int zOff[] = {CHUNK_DEPTH, -CHUNK_DEPTH};
  for (int i = 0; i < 2; ++i) {
    glm::ivec3 newVec = coordinates + glm::ivec3(xOff[i], 0, 0);
    auto iter = chunkMap.find(newVec);
    if (iter != chunkMap.end())
      neighbors.push_back(*iter);
  }
  for (int i = 0; i < 2; ++i) {
    glm::ivec3 newVec = coordinates + glm::ivec3(0, 0, zOff[i]);
    auto iter = chunkMap.find(newVec);
    if (iter != chunkMap.end())
      neighbors.push_back(*iter);
  }
  return neighbors;
}

void Chunk::setSeed(unsigned int seedIn) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  Chunk::seed = seedIn;
  Chunk::perlin = siv::PerlinNoise{Chunk::seed};
}

std::vector<glm::ivec3> Chunk::getBlockPositions() {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  std::vector<glm::ivec3> positions;
  positions.reserve(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH);
  for (int x = 0; x < CHUNK_WIDTH; ++x) {
    for (int y = 0; y < CHUNK_HEIGHT; ++y) {
      for (int z = 0; z < CHUNK_DEPTH; ++z) {
        const Block &block = blocks[x][y][z];
        if (block.type->isSolid)
          positions.push_back(coordinates + glm::ivec3(x, y, z));
      }
    }
  }
  return positions;
}

bool Chunk::isBlockSolidLocal(glm::ivec3 position) {
  return position.y < CHUNK_HEIGHT &&
         blocks[position.x][position.y][position.z].type->isSolid;
}

bool Chunk::isBlockSolidGlobal(glm::ivec3 position) {
  return isBlockSolidLocal(position - coordinates);
}

bool Chunk::isBlockBreakableLocal(glm::ivec3 position) {
  return position.y < CHUNK_HEIGHT &&
         blocks[position.x][position.y][position.z].type->isBreakable;
}

bool Chunk::isBlockBreakableGlobal(glm::ivec3 position) {
  return isBlockBreakableLocal(position - coordinates);
}

glm::ivec3 Chunk::findChunkIndex(glm::vec3 position) {
  return glm::ivec3(floor(position.x / CHUNK_WIDTH) * CHUNK_WIDTH, 0,
                    floor(position.z / CHUNK_DEPTH) * CHUNK_DEPTH);
}

glm::ivec3 Chunk::findBlockIndex(glm::vec3 position) {
  glm::ivec3 chunkIndex = findChunkIndex(position);
  return glm::ivec3(floor(position.x - chunkIndex.x), floor(position.y),
                    floor(position.z - chunkIndex.z));
}

bool Chunk::destroyLocal(glm::ivec3 position) {
  Block &block = blocks[position.x][position.y][position.z];
  if (!block.type->isBreakable)
    return false;
  block.type = (BlockType *)AIR;
  if (blocks[position.x][position.y + 1][position.z].type == BUSH)
    blocks[position.x][position.y + 1][position.z].type = (BlockType *)AIR;
  return true;
}

bool Chunk::destroyGlobal(glm::ivec3 position) {
  return destroyLocal(position - coordinates);
}

bool Chunk::placeLocal(glm::ivec3 position) {
  Block *block = &blocks[position.x][position.y][position.z];
  if (block->type->isSolid)
    return false;
  block->type = selectedType;
  return true;
}

bool Chunk::placeGlobal(glm::ivec3 position) {
  return placeLocal(position - coordinates);
}

bool Chunk::isBlockWaterLocal(glm::ivec3 position) {
  return position.y < CHUNK_HEIGHT &&
         blocks[position.x][position.y][position.z].type == WATER;
}

bool Chunk::isBlockWaterGlobal(glm::ivec3 position) {
  return isBlockWaterLocal(position - coordinates);
}

void Chunk::spreadWater(glm::ivec3 position) {
  blocks[position.x][position.y][position.z].type = (BlockType *)WATER;
}

bool Chunk::selectBlockType(glm::ivec3 position) {
  selectedType = blocks[position.x][position.y][position.z].type;
  return false;
}
