#ifndef __LOCALFIC_H
#define __LOCALFIC_H

// --- Configure your Personalised FIC executable here --- //

// Commenting any line results in the removal of that block from your build.
// Note1: editing anything here results in full rebuild of fic!
// Note2: _PUBLIC_RELEASE overrules this config and is configured in Blocks.h.

//#define ENABLE_CLASSIFIER
//#define ENABLE_CLUSTER
#define ENABLE_SLIM
//#define ENABLE_DATAGEN
//#define ENABLE_EMM
//#define ENABLE_LESS
//#define ENABLE_OCCLASSIFIER
//#define ENABLE_PTREE
//#define ENABLE_STREAM
//#define ENABLE_TAGGROUP
//#define ENABLE_TILES

// --- The following include handles actual block activation and handles dependencies --- //

#include "blocks/Blocks.h"

#endif // __LOCALFIC_H
