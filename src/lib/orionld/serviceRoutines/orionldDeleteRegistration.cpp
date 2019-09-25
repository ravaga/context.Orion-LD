/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Ken Zangelin
*/
#include "string.h"
#include "logMsg/logMsg.h"                                        // LM_*
#include "logMsg/traceLevels.h"                                   // Lmt*

#include "rest/ConnectionInfo.h"                                  // ConnectionInfo
#include "orionld/common/orionldState.h"                          // orionldState
#include "orionld/common/orionldErrorResponse.h"                  // orionldErrorResponseCreate
#include "orionld/serviceRoutines/orionldDeleteRegistration.h"    // Own Interface
#include "mongoBackend/mongoRegistrationDelete.h"                 // mongoRegistrationDelete
#include "orionld/common/urlCheck.h"                              // urlCheck
#include "orionld/common/urnCheck.h"                              // urlCheck


// ----------------------------------------------------------------------------
//
// orionldDeleteRegistration -
//
bool orionldDeleteRegistration(ConnectionInfo* ciP)
{
   OrionError                       oError;
   char*                            details;
   const std::string                regId(orionldState.wildcard[0]);
  
  LM_T(LmtServiceRoutine, ("In orionldDeleteRegistration"));

  if ((urlCheck(orionldState.wildcard[0], &details) == false) && (urnCheck(orionldState.wildcard[0], &details) == false))
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Registration ID", details, OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }
  
  LM_T(LmtServiceRoutine, ("jorge-log: valor do id %s", regId.c_str()));

  mongoRegistrationDelete(regId, orionldState.tenant, ciP->servicePathV[0], &oError);

 
  if(oError.code != 0)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, oError.reasonPhrase.c_str(), oError.details.c_str(), OrionldDetailsString);
    ciP->httpStatusCode = oError.code;
  }else
  {
    ciP->httpStatusCode = SccNoContent;
  }
  
  

  return true;
}
