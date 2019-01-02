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


#include "../PrecompiledHeadersServer.h"
#include "OrthancRestApi.h"

#include "../../Core/DicomParsing/DicomDirWriter.h"
#include "../../Core/FileStorage/StorageAccessor.h"
#include "../../Core/Compression/HierarchicalZipWriter.h"
#include "../../Core/HttpServer/FilesystemHttpSender.h"
#include "../../Core/Logging.h"
#include "../../Core/Toolbox.h"
#include "../ServerContext.h"

#include <stdio.h>
#include <map>
#include <string>

#if defined(_MSC_VER)
#define snprintf _snprintf
#endif

extern std::map<std::string, int> session;

namespace Orthanc
{
	bool ParseJsonRequest(Json::Value& result, const std::string request)
	{
		result.clear();
		Json::Reader reader;
		return reader.parse(request, result);
	}
    void ParseArgs(IHttpHandler::Arguments& result, const std::string cookies)
    {
      result.clear();
    
      size_t pos = 0;
      while (pos != std::string::npos)
      {
        size_t nextSemicolon = cookies.find("&", pos);
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

	static void signup(RestApiPostCall& call)
	{
		IHttpHandler::Arguments result;
		ParseArgs(result,call.GetBodyData());

		std::string name = result["username"];
		std::string mail = result["email"];
		Toolbox::UrlDecode(name);
		Toolbox::UrlDecode(mail);

		std::string md5;
		Toolbox::ComputeMD5(md5, result["password"]);
		//std::cout << md5 << std::endl;

		ServerContext& context = OrthancRestApi::GetContext(call);
		int id = context.GetIndex().SignUp(name,md5,mail);

		Json::Value root;
		root["id"] = id;

		Json::StyledWriter swriter;
		std::string str = swriter.write(root);
		call.GetOutput().AnswerBuffer(str, "application/json");
	}

	static void signin(RestApiPostCall& call)
	{
		IHttpHandler::Arguments result;
		ParseArgs(result,call.GetBodyData());

		//std::cout << call.GetBodyData() << std::endl;

		std::string md5;
		Toolbox::ComputeMD5(md5, result["password"]);
		//std::cout << md5 << std::endl;

		ServerContext& context = OrthancRestApi::GetContext(call);
		int id = context.GetIndex().SignIn(result["username"], md5);

		if(id > 0) //sign in sucess
		{
			Toolbox::ComputeMD5(md5, result["username"]);
			session.insert(std::pair<std::string, int>(md5, id));
			call.GetOutput().SetCookie("session", md5);
		}
		else
		{
			call.GetOutput().SetCookie("session", "");
		}

		Json::Value root;
		root["id"] = id;
		root["mail"] = context.GetIndex().getUserMail(id);

		Json::StyledWriter swriter;
		std::string str = swriter.write(root);
		call.GetOutput().AnswerBuffer(str, "application/json");
	}

	static void signout(RestApiPostCall& call)
	{
		call.GetOutput().SetCookie("username", "");
		call.GetOutput().SetCookie("password", "");

		Json::Value root;
		root["err"] = 0;
		Json::StyledWriter swriter;
		std::string str = swriter.write(root);
		call.GetOutput().AnswerBuffer(str, "application/json");
	}

	static void check(RestApiPostCall& call)
	{
		IHttpHandler::Arguments result;
		ParseArgs(result,call.GetBodyData());
		std::string name= result["username"];
		Toolbox::UrlDecode(name);
		int count = OrthancRestApi::GetContext(call).GetIndex().GetScanTimes(name);

		Json::Value root;
		root["scan_times"] = count;
		Json::StyledWriter swriter;
		std::string str = swriter.write(root);
		call.GetOutput().AnswerBuffer(str, "application/json");
	}

	static void TopUp(RestApiPostCall& call)
	{
		IHttpHandler::Arguments result;
		ParseArgs(result,call.GetBodyData());
		std::string name = result["username"];
		std::string key= result["key"];

		Json::Value ask;
		ask["err"] = OrthancRestApi::GetContext(call).GetIndex().TopUp(name,key);
		Json::StyledWriter swriter;
		std::string str = swriter.write(ask);
		call.GetOutput().AnswerBuffer(str, "application/json");
	}

	static void Reduce(RestApiPostCall& call)
	{
		IHttpHandler::Arguments result;
		ParseArgs(result,call.GetBodyData());
		std::string name = result["username"];

		Json::Value ask;
		ask["err"] = OrthancRestApi::GetContext(call).GetIndex().Reduce(name);
		Json::StyledWriter swriter;
		std::string str = swriter.write(ask);
		call.GetOutput().AnswerBuffer(str, "application/json");
	}

	static void Produce(RestApiPostCall& call)
	{
		IHttpHandler::Arguments request;
		ParseArgs(request,call.GetBodyData());
		std::string value = request["value"];

		srand(time(0));
		long long i = rand();
		std::string md5;
		Toolbox::ComputeMD5(md5, std::to_string(i));

		Json::Value ask;
		ask["id"] = OrthancRestApi::GetContext(call).GetIndex().Produce(md5,atoi(value.c_str()));
		if(ask["id"] != -1)
			ask["key"] = md5;
		Json::StyledWriter swriter;
		std::string str = swriter.write(ask);
		call.GetOutput().AnswerBuffer(str, "application/json");
	}

	static void GetTableInfo(RestApiPostCall& call)
	{
		IHttpHandler::Arguments request;
		ParseArgs(request,call.GetBodyData());
		std::string table_name = request["table"];
		std::vector<Json::Value> table;
		OrthancRestApi::GetContext(call).GetIndex().GetTableInfo(table,table_name);

		Json::StyledWriter swriter;
		std::string str;
		for(auto val = table.begin();val != table.end();++val){
			str += swriter.write(*val);
		}
		call.GetOutput().AnswerBuffer(str, "application/json");
	}

  void OrthancRestApi::RegisterAuth()
  {
    Register("/signup", signup);
    Register("/signin", signin);
    Register("/signout", signout);
	Register("/check",check);
	Register("/topup",TopUp);
	Register("/reduce",Reduce);
	Register("/produce",Produce);
	Register("/tableinfo",GetTableInfo);
  }
}
