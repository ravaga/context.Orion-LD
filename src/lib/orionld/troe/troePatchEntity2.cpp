/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
extern "C"
{
#include "kbase/kMacros.h"                                       // K_VEC_SIZE
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/kjTree/kjTreeLog.h"                            // kjTreeLog
#include "orionld/troe/troePatchEntity.h"                        // troePatchEntity - to reuse the "push to TRoE" of troePatchEntity
#include "orionld/troe/troePatchEntity2.h"                       // Own interface



// -----------------------------------------------------------------------------
//
// pathComponentsSplit -
//
static int pathComponentsSplit(char* path, char** compV)
{
  int compIx = 1;

  compV[0] = path;

  while (*path != 0)
  {
    if (*path == '.')
    {
      *path = 0;
      ++path;
      compV[compIx] = path;

      //
      // We only split the first 6 components
      // Max PATH is "attrs.P1.md.Sub-P1.value[.X]*
      //
      if (compIx == 5)
        return 6;

      //
      // We break if we find "attrs.X.value", but not until we have 4 components; "attrs.P1.value.[.X]*"
      // It is perfectly possible 'path' is only "attrs.P1.value", and if so, we'd have left the function already
      //
      if ((compIx == 3) && (strcmp(compV[2], "value") == 0))
        return 4;

      ++compIx;
    }

    ++path;
  }

  return compIx;
}



// -----------------------------------------------------------------------------
//
// kjNavigate -
//
KjNode* kjNavigate(KjNode* treeP, char** pathCompV, KjNode** parentPP, bool* onlyLastMissingP)
{
  KjNode* hitP = kjLookup(treeP, pathCompV[0]);

  *parentPP = treeP;

  if (hitP == NULL)  // No hit - we're done
  {
    *onlyLastMissingP = (pathCompV[1] == NULL)? true : false;
    return NULL;
  }

  if (pathCompV[1] == NULL)  // Found it - we're done
    return hitP;

  return kjNavigate(hitP, &pathCompV[1], parentPP, onlyLastMissingP);  // Recursive call, one level down
}



// -----------------------------------------------------------------------------
//
// pathComponentsFix -
//
//
// Some pieces of information for the DB are either redundant info for the database model of Orion or
// builtin timestamps, that are not of interest for the TRoE database
//
// Also, all fields that are entity-characteristics, not attribute, are skipped (for now)
// Once multi-attribute and/or Scope is supported, this will have to change
//
int pathComponentsFix(char** compV, int components, bool* skipP)
{
  int oldIx = 0;
  int newIx = 0;

  for (int ix = 0; ix < components; ix++)
  {
    if (oldIx == 0)
    {
      if (strcmp(compV[oldIx], "attrs") == 0)
      {
        ++oldIx;  // Jump over "attrs"
        continue;
      }
      else
      {
        *skipP = true;
        return -1;
      }
    }

    if ((oldIx == 2) && strcmp(compV[oldIx], "md") == 0)
    {
      ++oldIx;  // Jump over "md"
      continue;
    }
#if 1
    if ((oldIx == 4) && strcmp(compV[oldIx], "value") == 0)  // This will change once I eat the "value" object in dbModel function
    {
      if (strcmp(compV[3], "observedAt") == 0)
      {
        compV[newIx] = NULL;  // Never mind "value"
        return newIx;
      }

      if (strcmp(compV[3], "unitCode") == 0)
      {
        compV[newIx] = NULL;  // Never mind "value"
        return newIx;
      }
    }
#endif
    compV[newIx] = compV[oldIx];
    ++newIx;
    ++oldIx;
  }

  if (strcmp(compV[newIx - 1], "modDate") == 0) *skipP = true;
  if (strcmp(compV[newIx - 1], "creDate") == 0) *skipP = true;
  if (strcmp(compV[newIx - 1], "mdNames") == 0) *skipP = true;

  compV[newIx] = NULL;

  return newIx;
}


char* pathArrayJoin(char* out, char** compV, int components)
{
  strcpy(out, compV[0]);

  for (int ix = 1; ix < components; ix++)
  {
    strcat(out, ".");
    strcat(out, compV[ix]);
  }

  return out;
}



// -----------------------------------------------------------------------------
//
// patchApply
//
void patchApply(KjNode* patchBase, KjNode* patchP)
{
  KjNode* pathNode = kjLookup(patchP, "PATH");
  KjNode* treeNode = kjLookup(patchP, "TREE");
//  KjNode* opNode   = kjLookup(patchP, "op");

  if (pathNode == NULL)           return;
  if (treeNode == NULL)           return;
  if (pathNode->type != KjString) return;

  char* path = pathNode->value.s;
  LM_TMP(("KZ: PATH: '%s'", path));
  char* compV[7];
  int   components = pathComponentsSplit(path, compV);
  char  buf[512];

  LM_TMP(("KZ: Split component array: %s (%d components)", pathArrayJoin(buf, compV, components), components));

  bool  skip = false;

  components = pathComponentsFix(compV, components, &skip);
  if (skip)
  {
    LM_TMP(("KZ: ------- Skipped ----------"));
    return;
  }

  pathNode->value.s = kaStrdup(&orionldState.kalloc, pathArrayJoin(buf, compV, components));
  LM_TMP(("KZ: Fixed component array: %s (%d components)", pathNode->value.s, components));
  LM_TMP(("KZ: -----------------"));


  KjNode*  parentP         = NULL;
  bool     onlyLastMissing = false;
  KjNode*  nodeP           = kjNavigate(patchBase, compV, &parentP, &onlyLastMissing);

  // Remove non-wanted parts of objects
  if (treeNode->type == KjObject)
  {
    const char* unwanted[] = { "modDate", "creDate", "mdNames" };

    for (unsigned int ix = 0; ix < K_VEC_SIZE(unwanted); ix++)
    {
      KjNode* nodeP = kjLookup(treeNode, unwanted[ix]);
      if (nodeP != NULL)
        kjChildRemove(treeNode, nodeP);
    }
  }

  if (nodeP == NULL)  // Did not exist in the "patch base" - add to parentP
  {
    if (onlyLastMissing == true)
    {
      kjChildRemove(patchP, treeNode);
      treeNode->name = compV[components - 1];
      LM_TMP(("KZ: Adding tree '%s' to parent '%s'", treeNode->name, parentP->name));
      kjChildAdd(parentP, treeNode);
    }
    else
      LM_E(("KZ: Did not find the immediate parent (parent: '%s')", (parentP != NULL)? parentP->name : "no parent"));

    return;
  }

  if (treeNode->type == KjNull)
  {
    LM_TMP(("KZ: Removing '%s' from '%s' of patchBase", path, parentP->name));
    kjChildRemove(parentP, nodeP);
    return;
  }

  LM_TMP(("KZ: Replacing treeNode (%s: %s) with patchNodeP (%s: %s)", treeNode->name, kjValueType(treeNode->type), nodeP->name, kjValueType(nodeP->type)));
  treeNode->name = nodeP->name;
  kjChildRemove(parentP, nodeP);
  kjChildRemove(patchP, treeNode);
  kjChildAdd(parentP, treeNode);
}



// ----------------------------------------------------------------------------
//
// troePatchEntity2 -
//
bool troePatchEntity2(void)
{
  KjNode* patchTree = orionldState.requestTree;
  KjNode* patchBase = orionldState.patchBase;

  kjTreeLog(orionldState.patchBase, "KZ: patchBase");
  kjTreeLog(patchTree, "KZ: patchTree");

  for (KjNode* patchP = patchTree->value.firstChildP; patchP != NULL; patchP = patchP->next)
  {
    patchApply(patchBase, patchP);
  }

  kjTreeLog(patchBase, "KZ: patched result");

  //
  // No need to reimplement what troePatchEntity already implement - push to the TRoE DB
  // We just need to "pretend" that the patched result (orionldState.patchBase) was what came in
  //
  orionldState.requestTree = patchBase;
  orionldState.patchTree   = patchTree;

  troePatchEntity();

  return true;
}