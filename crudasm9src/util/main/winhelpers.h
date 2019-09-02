// =======================================================================================================================================================
// winhelpers.h - Copyright (C) 2014 Willow Schlanger. All rights reserved.
//
// This is part of Project Infrared by Willow Schlanger. Home page: http://www.willowschlanger.info
//
// -------------------------------------------------------------------------------------------------------------------------------------------------------
//
// This PARTICULAR source file, winhelpers.h, is under the following license:
//
// Copyright (c) 2014, Willow Schlanger
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
//
//     Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
//     Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
//
// =======================================================================================================================================================

#ifndef l_winhelpers__infrared_included
#define l_winhelpers__infrared_included

#ifdef _WIN32

#include <windows.h>
#include <stddef.h>

#include <cstdlib>
#include <cctype>
#include <cstdio>

#include <string>
#include <vector>

#include <iostream>

static std::string int_to_string_2(int num)
{
	using namespace std;
	char s[1024];
	sprintf(s, "%d", num);
	return std::string(s);
}

static std::string make_uppercase(const std::string s)
{
	using namespace std;
	std::string t;
	for(std::string::const_iterator i = s.begin(); i != s.end(); ++i)
		t += toupper(*i);
	return t;
}

static int convert_string_to_int(std::string s)
{
	using namespace std;
	return atoi(s.c_str());
}

static std::wstring utf8_to_utf16(const char *s)
{
	int size_required = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
	
	if(size_required == 0)
		return L"";
	
	wchar_t *wstr = new wchar_t [size_required];
	
	int result = MultiByteToWideChar(CP_UTF8, 0, s, -1, wstr, size_required);
	
	if(result == 0)
	{
		delete [] wstr;
		return L"";
	}
	
	std::wstring str(wstr);
	
	delete [] wstr;
	
	return str;
}

static std::string utf16_to_utf8(const wchar_t *s)
{
	int size_required = WideCharToMultiByte(CP_UTF8, 0, s, -1, NULL, 0, NULL, NULL);
	
	if(size_required == 0)
		return "";
	
	char *cstr = new char [size_required];
	
	int result = WideCharToMultiByte(CP_UTF8, 0, s, -1, cstr, size_required, NULL, NULL);
	
	if(result == 0)
	{
		delete [] cstr;
		return "";
	}
	
	std::string str(cstr);
	
	delete [] cstr;
	
	return str;
}

static bool file_exists(std::string name)
{
	using namespace std;

	FILE *fi = _wfopen(utf8_to_utf16(name.c_str()).c_str(), L"rb");

	if(fi != NULL)
	{
		fclose(fi);
		return true;
	}
	
	return false;
}

// Returns the directory path (excluding any final backslash or forwards-slash) part of a full filename.
// Also converts any forwards slashes to backslashes. User should append a backslash and then a filename
// to make a fully qualified path. The empty string is returned if we only have a filename.
static std::string directory_from_filename(std::string s)
{
	for(size_t i = 0; i < s.size(); ++i)
	{
		if(s[i] == '/')
			s[i] = '\\';
	}
	
	if(s.size() >= 2 && s[0] == '\\' && s[1] == '\\')
		return "";		// UNC path?

	for(size_t i = s.size() - 1; ;)
	{
		if(s[i] == '\\')
		{
			return std::string(s.begin(), s.begin() + i);
		}
		
		if(i == 0)
			break;
		--i;
	}
	
	return "";
}

static std::string get_filename_only(std::string s)
{
	for(size_t i = s.size() - 1; ;)
	{
		if(s[i] == '\\' || s[i] == '/')
		{
			return std::string(s.begin() + i + 1, s.end());
		}
	
		if(i == 0)
			break;
		--i;
	}

	return s;
}

// This returns a normalized version of 'filename'. For our purposes, making everything upper-case suffices.
static std::string normalize_filename(std::string filename)
{
	std::string s = filename;
	
	using namespace std;
	
	for(size_t i = 0; i < s.size(); ++i)
	{
		s[i] = toupper(s[i]);
	}
	
	return s;
}

static char *read_file(std::string filename, long long *out_size_bytes)
{
	long long tmp = 0;
	if(out_size_bytes == NULL)
		out_size_bytes = &tmp;
	*out_size_bytes = 0;
	
	using namespace std;
	
	FILE *fi = _wfopen(utf8_to_utf16(filename.c_str()).c_str(), L"rb");
	
	if(fi == NULL)
		return NULL;
	
	_fseeki64(fi, -1LL, SEEK_END);
	*out_size_bytes = 1LL + _ftelli64(fi);
	rewind(fi);
	
	char c;
	if(fread(&c, 1, 1, fi) != 1)
	{
		// File is probably empty - can't even read 1st byte!
		*out_size_bytes = 0;
		fclose(fi);
		return NULL;
	}
	
	rewind(fi);
	char *fd = new char [*out_size_bytes /*+ 64*/];
	//memset(fd + *out_size_bytes, 0, 64);

	/*
	for(long long pos = 0; pos < *out_size_bytes; )
	{
		long requested_size = 16 * 1024 * 1024;

		if(requested_size > *out_size_bytes - pos)
			requested_size = *out_size_bytes - pos;

		long actually_read = fread(fd + pos, requested_size, 1, fi);
		
		if(actually_read != 1)
		{
			delete [] fd;
			return NULL;	// error reading from file!
		}
		
		pos += requested_size;
	}
	*/
	
	// We don't need to support reading files with a size >=4GB into memory.
	if(fread(fd, *out_size_bytes, 1, fi) != 1)
	{
		delete [] fd;
		*out_size_bytes = 0;	// indicate failure
		return NULL;	// error reading from file!
	}
	
	return fd;
}

static int process_windows_args(std::ostream &os, std::vector<std::string> &args, int &app_num_args)
{
	args.clear();

	LPWSTR app_arg_str = GetCommandLineW();
	app_num_args = 0;
	LPWSTR *app_arg_list = CommandLineToArgvW(app_arg_str, &app_num_args);
	
	char *tmpstr = NULL;
	size_t tmpsize = 0;
	for(int i = 0; i < app_num_args; ++i)
	{
		int size_required = WideCharToMultiByte(CP_UTF8, 0, app_arg_list[i], -1, NULL, 0, NULL, NULL);

		if(size_required == 0)
		{
			delete [] tmpstr;
			os << "fatal: argument " << i << " specified to program is invalid" << std::endl;
			return 1;
		}
		
		if((size_t)(size_required) > tmpsize)
		{
			tmpsize = size_required * 2;
			delete [] tmpstr;
			tmpstr = new char [tmpsize];
		}
		
		int result = WideCharToMultiByte(CP_UTF8, 0, app_arg_list[i], -1, tmpstr, tmpsize, NULL, NULL);
		if(result == 0)
		{
			delete [] tmpstr;
			os << "fatal: argument " << i << " specified to program is invalid" << std::endl;
			return 1;
		}
		
		args.push_back(std::string(tmpstr));
	}
	delete [] tmpstr;
	tmpstr = NULL;
	
	return 0;
}

#endif

#endif	// l_winhelpers__infrared_included
