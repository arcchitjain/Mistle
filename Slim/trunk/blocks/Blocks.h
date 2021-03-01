#ifndef __BLOCKS_H
#define __BLOCKS_H

// --- Config public release build (overrule LocalFic.h) --- 

#ifdef _PUBLIC_RELEASE
#define ENABLE_CLASSIFIER
#undef ENABLE_CLUSTER
#define ENABLE_SLIM
#undef ENABLE_DATAGEN
#undef ENABLE_EMM
#undef ENABLE_LESS
#undef ENABLE_OCCLASSIFIER
#undef ENABLE_PTREE
#define ENABLE_STREAM
#undef ENABLE_TILES
#endif

// ---- Config blocks to be build (handles dependencies) ---

// ENABLE_ causes the TH and required code blocks to be enabled.
// BLOCK_ only enabled blocks of code, not the associated THs.

// Don't touch this (unless you really want to modify dependencies between blocks)
// See your LocalFic.h if you want to enable/disable certain blocks.

#ifdef ENABLE_CLASSIFIER
#define BLOCK_CLASSIFIER
#endif
#ifdef ENABLE_CLUSTER
#define BLOCK_CLUSTER
#define BLOCK_DATAGEN
#endif
#ifdef ENABLE_SLIM
#define BLOCK_SLIM
#endif
#ifdef ENABLE_DATAGEN
#define BLOCK_DATAGEN
#endif
#ifdef ENABLE_EMM
#define BLOCK_EMM
#endif
#ifdef ENABLE_LESS
#define BLOCK_LESS
#endif
#ifdef ENABLE_OCCLASSIFIER
#define BLOCK_DATAGEN
#define BLOCK_OCCLASSIFIER
#endif
#ifdef ENABLE_PTREE
#define BLOCK_PTREE
#endif
#ifdef ENABLE_STREAM
#define BLOCK_STREAM
#define BLOCK_DATAGEN
#endif
#ifdef ENABLE_TAGS
#define BLOCK_TAGGROUP
#define BLOCK_TAGRECOM
#endif
#ifdef ENABLE_TILES
#define BLOCK_TILES
#define BLOCK_TILING
#endif
#endif // __BLOCKS_H
