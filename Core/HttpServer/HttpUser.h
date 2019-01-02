/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#pragma once

#include <list>
#include <string>
#include <stdint.h>
#include <map>

extern std::map<std::string, int> session;

namespace Orthanc
{
class MyUserAuthFilter : public IUserAuthFilter
{
private:
  ServerContext& context_;

  std::set<std::string> uncheckedResources_;
  std::list<std::string> uncheckedFolders_;

public:
  MyUserAuthFilter(ServerContext& context) : 
    context_(context)
  {
	uncheckedResources_.insert("/signup");
	uncheckedResources_.insert("/signin");
	uncheckedResources_.insert("/signout");
	uncheckedResources_.insert("/check");
	uncheckedResources_.insert("/topup");
	uncheckedResources_.insert("/reduce");
	uncheckedResources_.insert("/produce");
	uncheckedResources_.insert("/tableinfo");
	uncheckedResources_.insert("/");
	uncheckedFolders_.push_back("/hi/");
	uncheckedFolders_.push_back("/app/");
	uncheckedFolders_.push_back("/plugins/");
	uncheckedFolders_.push_back("/system");
	uncheckedFolders_.push_back("/users");
	uncheckedFolders_.push_back("/cards");
	uncheckedFolders_.push_back("/box/");
  }

  bool IsAllowed(HttpMethod method,
                           const char* uri,
                           const char* ip,
                           int userId,
                           const IHttpHandler::Arguments& httpHeaders,
                           const IHttpHandler::GetArguments& getArguments)
  {
    if(uncheckedResources_.find(uri) != uncheckedResources_.end()) return 1;
    for(std::list<std::string>::const_iterator it = uncheckedFolders_.begin(); it != uncheckedFolders_.end(); ++it)
    {
      if(Toolbox::StartsWith(uri, *it)) return 1;
    }
	return 0;
  }

  int getUserId(const IHttpHandler::Arguments& httpHeaders)
  {
    IHttpHandler::Arguments cookies;

    for (IHttpHandler::Arguments::const_iterator
           it = httpHeaders.begin(); it != httpHeaders.end(); ++it)
    {
	  if(it->first.compare("cookie")==0) ParseCookies(cookies, it->second);
    }

	if(cookies["session"].length() < 8) return 0;

	std::map<std::string,int>::iterator it;
	it = session.find(cookies["session"]);
	if (it != session.end()) return it->second;

	return 0;
  }

  void ParseCookies(IHttpHandler::Arguments& result, const std::string cookies)
  {
    result.clear();

    size_t pos = 0;
    while (pos != std::string::npos)
    {
      size_t nextSemicolon = cookies.find(";", pos);
      std::string cookie;

      if (nextSemicolon == std::string::npos)
      {
        cookie = cookies.substr(pos);
        pos = std::string::npos;
      }
      else
      {
        cookie = cookies.substr(pos, nextSemicolon - pos);
        pos = nextSemicolon + 1;
      }

      size_t equal = cookie.find("=");
      if (equal != std::string::npos)
      {
        std::string name = Toolbox::StripSpaces(cookie.substr(0, equal));
        std::string value = Toolbox::StripSpaces(cookie.substr(equal + 1));
        result[name] = value;
      }
    }
  }
};
}
