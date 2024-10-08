#include "block.hpp"

#include <iostream>

#include <ThreadedLoggerForCPP/LoggerThread.hpp>

#include <ThreadedLoggerForCPP/LoggerFileSystem.hpp>
#include <ThreadedLoggerForCPP/LoggerGlobals.hpp>

#include <game_performance_profiler.hpp>

glm::vec2 BlockType::purge(Direction dir, glm::ivec3 coords) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  static const float margin = 0.003f;

  glm::ivec2 coords2D;
  switch (dir) {
  case Direction::Up:
    coords2D = glm::ivec2(coords.x, coords.z);
    break;
  case Direction::Down:
    coords2D = glm::ivec2((coords.x + 1) % 2, coords.z);
    break;
  case Direction::North:
    coords2D = glm::ivec2(coords.x, (coords.y + 1) % 2);
    break;
  case Direction::South:
    coords2D = glm::ivec2((coords.x + 1) % 2, (coords.y + 1) % 2);
    break;
  case Direction::East:
    coords2D = glm::ivec2((coords.z + 1) % 2, (coords.y + 1) % 2);
    break;
  case Direction::West:
    coords2D = glm::ivec2(coords.z, (coords.y + 1) % 2);
    break;
  }

  return (glm::vec2)coords2D * (1 - 2 * margin) + margin;
}

glm::vec2 Air::getTextureOffset(Direction dir, glm::ivec3 corner) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  glm::vec2 fileOffset(11, 1);
  glm::vec2 blockOffset = BlockType::purge(dir, corner);
  return (fileOffset + blockOffset) * BlockType::textureSize;
}

glm::vec2 Dirt::getTextureOffset(Direction dir, glm::ivec3 corner) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  glm::vec2 fileOffset(2, 0);
  glm::vec2 blockOffset = BlockType::purge(dir, corner);
  return (fileOffset + blockOffset) * BlockType::textureSize;
}

glm::vec2 Grass::getTextureOffset(Direction dir, glm::ivec3 corner) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  glm::vec2 fileOffset;
  glm::vec2 blockOffset = BlockType::purge(dir, corner);
  switch (dir) {
  case Direction::Up:
    fileOffset = glm::vec2(0, 0);
    break;
  case Direction::Down:
    fileOffset = glm::vec2(2, 0);
    break;
  default:
    fileOffset = glm::vec2(3, 0);
    break;
  }
  return (fileOffset + blockOffset) * BlockType::textureSize;
}

glm::vec2 WoodLog::getTextureOffset(Direction dir, glm::ivec3 corner) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  glm::vec2 fileOffset;
  glm::vec2 blockOffset = BlockType::purge(dir, corner);
  switch (dir) {
  case Direction::Up:
  case Direction::Down:
    fileOffset = glm::vec2(5, 1);
    break;
  default:
    fileOffset = glm::vec2(4, 1);
    break;
  }
  return (fileOffset + blockOffset) * BlockType::textureSize;
}

glm::vec2 WoodPlank::getTextureOffset(Direction dir, glm::ivec3 corner) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  glm::vec2 fileOffset(4, 0);
  glm::vec2 blockOffset = BlockType::purge(dir, corner);
  return (fileOffset + blockOffset) * BlockType::textureSize;
}

glm::vec2 Stone::getTextureOffset(Direction dir, glm::ivec3 corner) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  glm::vec2 fileOffset(1, 0);
  glm::vec2 blockOffset = BlockType::purge(dir, corner);
  return (fileOffset + blockOffset) * BlockType::textureSize;
}

glm::vec2 Cobblestone::getTextureOffset(Direction dir, glm::ivec3 corner) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  glm::vec2 fileOffset(0, 1);
  glm::vec2 blockOffset = BlockType::purge(dir, corner);
  return (fileOffset + blockOffset) * BlockType::textureSize;
}

glm::vec2 Sand::getTextureOffset(Direction dir, glm::ivec3 corner) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  glm::vec2 fileOffset(2, 1);
  glm::vec2 blockOffset = BlockType::purge(dir, corner);
  return (fileOffset + blockOffset) * BlockType::textureSize;
}

glm::vec2 Gravel::getTextureOffset(Direction dir, glm::ivec3 corner) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  glm::vec2 fileOffset(3, 1);
  glm::vec2 blockOffset = BlockType::purge(dir, corner);
  return (fileOffset + blockOffset) * BlockType::textureSize;
}

glm::vec2 Bedrock::getTextureOffset(Direction dir, glm::ivec3 corner) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  glm::vec2 fileOffset(1, 1);
  glm::vec2 blockOffset = BlockType::purge(dir, corner);
  return (fileOffset + blockOffset) * BlockType::textureSize;
}

glm::vec2 Leaves::getTextureOffset(Direction dir, glm::ivec3 corner) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  glm::vec2 fileOffset(4, 8);
  glm::vec2 blockOffset = BlockType::purge(dir, corner);
  return (fileOffset + blockOffset) * BlockType::textureSize;
}

glm::vec2 Water::getTextureOffset(Direction dir, glm::ivec3 corner) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  glm::vec2 fileOffset(13, 12);
  glm::vec2 blockOffset = BlockType::purge(dir, corner);
  return (fileOffset + blockOffset) * BlockType::textureSize;
}

glm::vec2 Bush::getTextureOffset(Direction dir, glm::ivec3 corner) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  glm::vec2 fileOffset(7, 2);
  glm::vec2 blockOffset = BlockType::purge(dir, corner);
  return (fileOffset + blockOffset) * BlockType::textureSize;
}

glm::vec2 BlackWool::getTextureOffset(Direction dir, glm::ivec3 corner) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  glm::vec2 fileOffset(1, 7);
  glm::vec2 blockOffset = BlockType::purge(dir, corner);
  return (fileOffset + blockOffset) * BlockType::textureSize;
}

glm::vec2 WhiteWool::getTextureOffset(Direction dir, glm::ivec3 corner) {
  PROFILE_SCOPED(std::string("Vulkraft:") + ":" + __FUNCTION__)
  glm::vec2 fileOffset(0, 4);
  glm::vec2 blockOffset = BlockType::purge(dir, corner);
  return (fileOffset + blockOffset) * BlockType::textureSize;
}