// Copyright (c) 1997-2021 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// a205face.cpp: Interface to ASHRAE Std 205 equipment data representations

#include "cnglob.h"

#include "a205face.h"



#if 0
// experimental code 9/2020 ?
#include <algorithm>
#include <fstream>
#include <iostream>
#include <istream>

#include "json.hpp"

using json = nlohmann::json;

RC TestCBOR(
	const char* filePath)
{
	RC rc = RCOK;

	std::ifstream fileStream(filePath);

	const char* ext = strpathparts(filePath, STRPPEXT);
	bool isCBOR = stricmp(ext, ".cbor") == 0;

	json x = 
		isCBOR ? json::from_cbor( fileStream)
			   : json::parse(fileStream);

#if 0
	MYLOADER L( x, otherstuff)

	std::string rs_id = L.get("RS_ID", "?");

	fluidType = L.getEnum("Fluid", enumTable)

	timeStamp = L.getTimeStamp()

	string z = x.value("name")
#endif


	for (auto it = x.begin(); it != x.end(); ++it)
	{
		std::cout << "key: " << it.key() << ", value:" << it.value() << '\n';
	}

	return rc;
}
#endif

// a205face.cpp end