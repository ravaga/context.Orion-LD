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
#include "logMsg/logMsg.h"                                    // LM_*

#include "orionld/types/OrionldResponseErrorType.h"           // OrionldResponseErrorType
#include "orionld/types/OrionldProblemDetails.h"              // OrionldProblemDetails
#include "orionld/common/orionldError.h"                      // Own interface



// ----------------------------------------------------------------------------
//
// orionldError -
//
void orionldError
(
  OrionldProblemDetails*    pdP,
  OrionldResponseErrorType  errorType,
  const char*               title,
  const char*               detail,
  int                       status
)
{
  //
  // Id pdP already contains an error, then that first error prevails
  //
  //  if (pdP->type != 0)
  {
    pdP->type   = errorType;
    pdP->title  = (char*) title;
    pdP->detail = (char*) detail;
    pdP->status = status;
  }

  LM_E(("%s: %s (status code: %d", title, detail, status));
}