/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Orion dev team
*/
#include <string>
#include <vector>

#include "orionld/common/orionldState.h"             // orionldState

#include "common/statistics.h"
#include "common/clockFunctions.h"
#include "common/string.h"
#include "common/errorMessages.h"

#include "rest/ConnectionInfo.h"
#include "rest/uriParamNames.h"
#include "ngsi/ParseData.h"
#include "apiTypesV2/Entities.h"
#include "rest/EntityTypeInfo.h"
#include "serviceRoutinesV2/getEntities.h"
#include "serviceRoutines/postQueryContext.h"
#include "rest/OrionError.h"
#include "parse/forbiddenChars.h"



/* ****************************************************************************
*
* getEntity -
*
* GET /v2/entities/:id:[?attrs=:list:]
*
* Payload In:  None
* Payload Out: Entity
*
* URI parameters:
*   - type=<TYPE>
*   - options=keyValues|values|unique   (used in Entity::render)
*   - attrs=A1,A2,...An                 (used in Entity::render)
*   - metadata=M1,M2,...Mn              (used in Entity::render)
*/
std::string getEntity
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string entityId        = compV[2];
  char*       type            = (orionldState.uriParams.type == NULL)? (char*) "" : orionldState.uriParams.type;

  if (entityId == "")
  {
    OrionError oe(SccBadRequest, ERROR_DESC_BAD_REQUEST_EMPTY_ENTITY_ID, ERROR_BAD_REQUEST);
    orionldState.httpStatusCode = oe.code;
    return oe.toJson();
  }

  if (forbiddenIdChars(orionldState.apiVersion, entityId.c_str(), NULL))
  {
    OrionError oe(SccBadRequest, ERROR_DESC_BAD_REQUEST_INVALID_CHAR_URI, ERROR_BAD_REQUEST);
    orionldState.httpStatusCode = oe.code;
    return oe.toJson();
  }

  // Fill in QueryContextRequest
  parseDataP->qcr.res.fill(entityId, type, "false", EntityTypeEmptyOrNotEmpty, "");

  // Call standard op postQueryContext
  postQueryContext(ciP, components, compV, parseDataP);


  // Render entity response
  Entity       entity;

  // If request was for /entities/<<id>>/attrs, type and id should not be shown
  if (compV.size() == 4 && compV[3] == "attrs")
  {
    entity.hideIdAndType();
  }

  entity.fill(&parseDataP->qcrs.res);

  std::string answer;
  TIMED_RENDER(answer = entity.render(false));

  if (parseDataP->qcrs.res.errorCode.code == SccOk && parseDataP->qcrs.res.contextElementResponseVector.size() > 1)
  {
    // No problem found, but we expect only one entity
    orionldState.httpStatusCode = SccConflict;
  }
  else
  {
    // the same of the wrapped operation
    orionldState.httpStatusCode = parseDataP->qcrs.res.errorCode.code;
  }

  // 04. Cleanup and return result
  entity.release();
  parseDataP->qcr.res.release();

  return answer;
}
