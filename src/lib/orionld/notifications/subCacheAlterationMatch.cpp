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
#include "kjson/KjNode.h"                                      // KjNode
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "cache/subCache.h"                                    // CachedSubscription, subCacheMatch, tenantMatch

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/types/OrionldAlteration.h"                   // OrionldAlteration, OrionldAlterationMatch, orionldAlterationType
#include "orionld/notifications/subCacheAlterationMatch.h"     // Own interface



// -----------------------------------------------------------------------------
//
// entityIdMatch -
//
static bool entityIdMatch(CachedSubscription* subP, const char* entityId, int eItems)
{
  for (int ix = 0; ix < eItems; ++ix)
  {
    EntityInfo* eiP = subP->entityIdInfos[ix];

    if (eiP->isPattern)
    {
      if (regexec(&eiP->entityIdPattern, entityId, 0, NULL, 0) == 0)
        return true;
    }
    else
    {
      if (strcmp(eiP->entityId.c_str(), entityId) == 0)
        return true;
    }
  }

  LM_TMP(("NFY: Sub '%s': no match due to Entity ID", subP->subscriptionId));
  return false;
}



// -----------------------------------------------------------------------------
//
// entityTypeMatch -
//
// Entity Type is mandatory in an NGSI-LD subscription. so can't be an empty string
// There is no pattern allowed for Entity Type in NGSI-LD (unlike NGSIv2)
//
static bool entityTypeMatch(CachedSubscription* subP, const char* entityType, int eItems)
{
  for (int ix = 0; ix < eItems; ++ix)
  {
    EntityInfo* eiP = subP->entityIdInfos[ix];

    if (strcmp(entityType, eiP->entityType.c_str()) == 0)
      return true;
  }

  LM_TMP(("NFY: Sub '%s': no match due to Entity Type", subP->subscriptionId));
  return false;
}



// -----------------------------------------------------------------------------
//
// matchListInsert -
//
static OrionldAlterationMatch* matchListInsert(OrionldAlterationMatch* matchList, OrionldAlterationMatch* itemP)
{
  OrionldAlterationMatch* matchP = matchList;

  while (matchP != NULL)
  {
    if (matchP->subP == itemP->subP)
    {
      itemP->next = matchP->next;
      matchP->next = itemP;

      return matchList;
    }

    matchP = matchP->next;
  }

  // First alteration-match of a subscription - prepending it to the matchList
  itemP->next = matchList;
  return itemP;
}



// -----------------------------------------------------------------------------
//
// matchToMatchList -
//
static OrionldAlterationMatch* matchToMatchList(OrionldAlterationMatch* matchList, CachedSubscription* subP, OrionldAlteration* altP, OrionldAttributeAlteration* aaP)
{
  LM_TMP(("NFY: Alteration made it all the way for sub %s:", subP->subscriptionId));
  if (aaP != NULL)
  {
    LM_TMP(("NFY:   - Alteration Type:  %s", orionldAlterationType(aaP->alterationType)));
    LM_TMP(("NFY:   - Attribute:        %s", aaP->attrName));
  }

  OrionldAlterationMatch* amP = (OrionldAlterationMatch*) kaAlloc(&orionldState.kalloc, sizeof(OrionldAlterationMatch));
  amP->altP     = altP;
  amP->altAttrP = aaP;
  amP->subP     = subP;

  if (matchList == NULL)
  {
    matchList = amP;
    amP->next = NULL;
  }
  else
    matchList = matchListInsert(matchList, amP);  // Items in matchList are grouped by their subP

  return matchList;
}



// -----------------------------------------------------------------------------
//
// attributeMatch -
//
static OrionldAlterationMatch* attributeMatch(OrionldAlterationMatch* matchList, CachedSubscription* subP, OrionldAlteration* altP, int* matchesP)
{
  int matches = 0;

  if (altP->alteredAttributes == 0)  // E.g. complete replace of an entity - treating it as EntityModified (for now)
  {
    // Is the Alteration type ON for this subscription?
    if (subP->triggers[EntityModified] == true)
    {
      matchList = matchToMatchList(matchList, subP, altP, NULL);
      ++matches;
    }
    else
      LM_TMP(("NFY: Sub '%s' - no match due to Trigger '%s'", subP->subscriptionId, orionldAlterationType(EntityModified)));
  }

  for (int aaIx = 0; aaIx < altP->alteredAttributes; aaIx++)
  {
    OrionldAttributeAlteration*  aaP        = &altP->alteredAttributeV[aaIx];
    int                          watchAttrs = subP->notifyConditionV.size();
    int                          nIx        = 0;

    while (nIx < watchAttrs)
    {
      // LM_TMP(("NFY: Comparing '%s' and '%s'", aaP->attrName, subP->notifyConditionV[nIx].c_str()));
      if (strcmp(aaP->attrName, subP->notifyConditionV[nIx].c_str()) == 0)
        break;
      ++nIx;
    }

    if ((watchAttrs > 0) && (nIx == watchAttrs))  // No match found
      continue;

    // Is the Alteration type ON for this subscription?
    if (subP->triggers[aaP->alterationType] == false)
    {
      LM_TMP(("NFY: Sub '%s' - no match due to Trigger '%s'", subP->subscriptionId, orionldAlterationType(aaP->alterationType)));
      continue;
    }

    matchList = matchToMatchList(matchList, subP, altP, aaP);
    ++matches;
  }

  if (matches == 0)
    LM_TMP(("NFY: Sub '%s' - no match due to Watched Attribute List (or Trigger!)", subP->subscriptionId));

  *matchesP += matches;

  return matchList;
}



// -----------------------------------------------------------------------------
//
// subCacheAlterationMatch -
//
OrionldAlterationMatch* subCacheAlterationMatch(OrionldAlteration* alterationList, int* matchesP)
{
  OrionldAlterationMatch*  matchList = NULL;
  int                      matches   = 0;

  for (OrionldAlteration* altP = alterationList; altP != NULL; altP = altP->next)
  {
    for (CachedSubscription* subP = subCacheHeadGet(); subP != NULL; subP = subP->next)
    {
      if ((multitenancy == true) && (tenantMatch(subP->tenant, orionldState.tenantName) == false))
      {
        LM_TMP(("NFY: Sub '%s' - no match due to tenant", subP->subscriptionId));
        continue;
      }

      if (subP->isActive == false)
      {
        LM_TMP(("NFY: Sub '%s' - no match due to isActive == false", subP->subscriptionId));
        continue;
      }

      if (strcmp(subP->status.c_str(), "active") != 0)
      {
        LM_TMP(("NFY: Sub '%s' - no match due to status == '%s' (!= 'active')", subP->subscriptionId, subP->status.c_str()));
        continue;
      }

      if (subP->expirationTime < orionldState.requestTime)
      {
        LM_TMP(("NFY: Sub '%s' - no match due to expiration", subP->subscriptionId));
        subP->status   = "expired";
        subP->isActive = false;
        continue;
      }

      if ((subP->throttling > 0) && ((orionldState.requestTime - subP->lastNotificationTime) < subP->throttling))
      {
        LM_TMP(("NFY: Sub '%s' - no match due to throttling", subP->subscriptionId));
        continue;
      }

      int eItems = subP->entityIdInfos.size();
      if (eItems > 0)
      {
        if (entityIdMatch(subP, altP->entityId, eItems) == false)
          continue;

        if (entityTypeMatch(subP, altP->entityType, eItems) == false)
          continue;
      }

      matchList = attributeMatch(matchList, subP, altP, &matches);  // Each call adds to matchList AND matches
    }
  }

  *matchesP = matches;

  return matchList;
}