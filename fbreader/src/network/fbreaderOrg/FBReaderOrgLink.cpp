/*
 * Copyright (C) 2008-2009 Geometer Plus <contact@geometerplus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <ZLNetworkUtil.h>
#include <ZLNetworkXMLParserData.h>

#include "FBReaderOrgLink.h"
#include "FBReaderOrgDataParser.h"
#include "../NetworkBookInfo.h"

FBReaderOrgLink::FBReaderOrgLink() : NetworkLink("LitRes.Ru", "litres.ru") {
}

static const std::string URL_PREFIX = "http://www.fbreader.org/library/";

shared_ptr<ZLNetworkData> FBReaderOrgLink::simpleSearchData(NetworkBookList &books, const std::string &pattern) {
	return new ZLNetworkXMLParserData(
		URL_PREFIX + "simple_query.php?type=xml&library=1&pattern=" + ZLNetworkUtil::htmlEncode(pattern),
		new FBReaderOrgDataParser(books)
	);
}

void FBReaderOrgLink::addSubPattern(std::string &url, const std::string &name, const std::string &value) {
	if (!value.empty()) {
		url += "&" + name + "=" + ZLNetworkUtil::htmlEncode(value);
	}
}

shared_ptr<ZLNetworkData> FBReaderOrgLink::advancedSearchData(NetworkBookList &books, const std::string &title, const std::string &author, const std::string &series, const std::string &tag, const std::string &annotation) {
	std::string request = URL_PREFIX + "advanced_query.php?type=xml&library=1";
	addSubPattern(request, "title", title);
	addSubPattern(request, "author", author);
	addSubPattern(request, "series", series);
	addSubPattern(request, "tag", tag);
	addSubPattern(request, "annotation", annotation);

	return new ZLNetworkXMLParserData(request, new FBReaderOrgDataParser(books));
}
