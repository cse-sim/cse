// Copyright (c) 1997-2020 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// a205face.cpp: Interface to ASHRAE205 equipment data representations

#include "cnglob.h"
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

	std::string rs_id = x.value("RS_ID", "?");

	for (auto it = x.begin(); it != x.end(); ++it)
	{
		std::cout << "key: " << it.key() << ", value:" << it.value() << '\n';
	}

	return rc;
}

// a205face.cpp end