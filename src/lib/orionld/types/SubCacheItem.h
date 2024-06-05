#ifndef SRC_LIB_ORIONLD_TYPES_SUBCACHEITEM_H_
#define SRC_LIB_ORIONLD_TYPES_SUBCACHEITEM_H_

/*
*
* Copyright 2024 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
#include <stdint.h>                                              // types: uint32_t, ...

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "orionld/types/OrionldContext.h"                        // OrionldContext



// -----------------------------------------------------------------------------
//
// SubDeltas -
//
typedef struct SubDeltas
{
  uint32_t timesSent;
  uint32_t timesFailed;
  double   lastSuccess;
  double   lastFailure;
} SubDeltas;



// -----------------------------------------------------------------------------
//
// SubCacheItem -
//
typedef struct SubCacheItem
{
  char*                 subId;              // Set when creating subscription - points inside subTree
  KjNode*               subTree;
  SubDeltas             deltas;
  bool                  dirty;              // The subscription has been patched - not only counters differ from copy in DB
  OrionldContext*       contextP;           // Set when creating/patching registration
  char*                 hostAlias;          // Broker identity - for the Via header

  // "Shortcuts" and transformations
  double                modifiedAt;         // Copied from inside the subTree

  struct SubCacheItem*  next;
} SubCacheItem;

#endif  // SRC_LIB_ORIONLD_TYPES_SUBCACHEITEM_H_
