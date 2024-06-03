#ifndef SRC_LIB_ORIONLD_HTTP_HTTPREQUEST_H_
#define SRC_LIB_ORIONLD_HTTP_HTTPREQUEST_H_

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
extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
}

#include "orionld/types/OrionldProblemDetails.h"               // OrionldProblemDetails
#include "orionld/types/HttpKeyValue.h"                        // HttpKeyValue



// -----------------------------------------------------------------------------
//
// httpRequest -
//
// RETURN VALUE
//   -1 on fatal error (details saved in pd)
//   
// PARAMETERS
//   verb:
//   ip:
//   url:       with or without URL parameters
//   body:      request payload body as KjNode tree
//   uriParams: URI parameters, as an array of HttpKeyValue - NULL if "url" already contains the URL parameters
//   headers:   HTTP Headers, as an array of HttpKeyValue - both input and output
//   tmo:       Timeout of reading the response? in milliseconds
//   response:  response payload body, as a KjNode tree
//   pdP:       ProblemDetails reference
//
extern int httpRequest
(
  const char*             verb,
  const char*             ip,
  const char*             url,
  KjNode*                 body,
  HttpKeyValue*           uriParams,
  HttpKeyValue*           headers,
  int                     tmo,
  KjNode**                response,
  OrionldProblemDetails*  pdP
);

#endif  // SRC_LIB_ORIONLD_HTTP_HTTPREQUEST_H_
