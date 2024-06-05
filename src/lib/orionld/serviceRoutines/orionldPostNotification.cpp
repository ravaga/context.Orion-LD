/*
*
 Copyright 2024 FIWARE Foundation e.V.
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
#include "kjson/kjLookup.h"                                    // kjLookup
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "cache/CachedSubscription.h"                          // CachedSubscription
#include "cache/subCache.h"                                    // subCacheItemLookup

#include "orionld/types/HttpKeyValue.h"                        // HttpKeyValue
#include "orionld/types/OrionLdRestService.h"                  // OrionLdRestService
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/http/httpRequest.h"                          // httpRequest
#include "orionld/http/httpRequestHeaderAdd.h"                 // httpRequestHeaderAdd
#include "orionld/serviceRoutines/orionldPostNotification.h"   // Own interface



// ----------------------------------------------------------------------------
//
// orionldPostNotification -
//
bool orionldPostNotification(void)
{
  char* parentSubId = orionldState.wildcard[0];

  if (distSubsEnabled == false)
  {
    LM_W(("Got a notification on remote subscription subordinate to '%s', but, distributed subscriptions are not enabled", parentSubId));

    orionldError(OrionldOperationNotSupported, "Distributed Subscriptions Are Not Enabled", orionldState.serviceP->url, 501);
    orionldState.noLinkHeader   = true;  // We don't want the Link header for non-implemented requests

    return true;
  }

  LM_T(LmtSR, ("Got a notification on remote subscription subordinate to '%s'", parentSubId));

  kjTreeLog(orionldState.requestTree, "notification", LmtSR);

  CachedSubscription* cSubP = subCacheItemLookup(orionldState.tenantP->tenant, parentSubId);
  if (cSubP == NULL)
  {
    LM_W(("Got a notification from a remote subscription '%s' on IP:PORT, but, its local parent subscription was not found", parentSubId));
    return false;
  }

  // Modify the payload body to fit the "new" notification triggered
  KjNode* subIdNodeP = kjLookup(orionldState.requestTree, "subscriptionId");
  if (subIdNodeP != NULL)
    subIdNodeP->value.s = parentSubId;

  // Send the notification
  OrionldProblemDetails pd;
  char                  url[256];
  KjNode*               responseTree = NULL;
  int                   httpStatus;
  HttpKeyValue          uriParams[2];
  HttpKeyValue          headers[3];

  bzero(&uriParams, sizeof(uriParams));
  bzero(&headers, sizeof(headers));

  uriParams[0].key   = (char*) "subscriptionId";
  uriParams[0].value = (char*) parentSubId;

  snprintf(url, sizeof(url), "%s://%s:%d/%s", cSubP->protocolString, cSubP->ip, cSubP->port, cSubP->rest);
  LM_T(LmtSR, ("ip:  '%s'", cSubP->ip));
  LM_T(LmtSR, ("url: '%s'", url));

  httpRequestHeaderAdd(&headers[0], "Content-Type", "application/json", 0);
  httpStatus = httpRequest(cSubP->ip, "POST", url, orionldState.requestTree, uriParams, headers, 5000, &responseTree, &pd);
  if (httpStatus != 200)
  {
    LM_W(("httpRequest for a forwarded notification gave HTTP status %d", httpStatus));
    kjTreeLog(responseTree, "forwarded notification response body", LmtSR);
  }

  return true;
}
