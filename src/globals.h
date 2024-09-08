#pragma once
#include "VulkraftApplication.h"
#include "chunk/chunkDrawing.h"
class VulkraftApplication; // FORWARD DECLARATION
class ChunkDrawing;        // FORWARD DECLARATION
class Globals {
public:
  static VulkraftApplication vulkraftApp;
  static ChunkDrawing chunkDrawing;
  static char *GAME_TITLE;
};
