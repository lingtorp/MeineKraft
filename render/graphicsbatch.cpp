#include "graphicsbatch.h"
#include "rendercomponent.h"

GraphicsBatch::GraphicsBatch(uint64_t hash_id): hash_id(hash_id), components{}, mesh{} {};