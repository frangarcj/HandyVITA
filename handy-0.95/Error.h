//
// Copyright (c) 2004 K. Wilkins
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from
// the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//

//////////////////////////////////////////////////////////////////////////////
//                       Handy - An Atari Lynx Emulator                     //
//                          Copyright (c) 1996,1997                         //
//                                 K. Wilkins                               //
//////////////////////////////////////////////////////////////////////////////
// Generic error handler class                                              //
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// This class provides error handler facilities for the Lynx emulator, I    //
// shamelessly lifted most of the code from Stella by Brad Mott.            //
//                                                                          //
//    K. Wilkins                                                            //
// August 1997                                                              //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////
// Revision History:                                                        //
// -----------------                                                        //
//                                                                          //
// 01Aug1997 KW Document header added & class documented.                   //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#ifndef ERROR_H
#define ERROR_H

#include <sstream>

using namespace std;

#define MAX_ERROR_MSG	512
#define MAX_ERROR_DESC	2048

//class CLynxException : public CException
class CLynxException
{
	public:
		// Constructor
		CLynxException() {}

		// Copy Constructor
		CLynxException(CLynxException& err)
		{
			int MsgCount,DescCount;

			MsgCount = int(err.Message().tellp()) + 1;
			DescCount = int(err.Description().tellp()) + 1;
			if(MsgCount>MAX_ERROR_MSG) MsgCount=MAX_ERROR_MSG;
			if(DescCount>MAX_ERROR_DESC) DescCount=MAX_ERROR_DESC;

			strncpy(mMsg,err.Message().str().c_str(),MsgCount);
			mMsg[MsgCount-1]='\0';
			strncpy(mDesc,err.Description().str().c_str(),DescCount);
			mDesc[DescCount-1]='\0';
		}

		// Destructor
		virtual ~CLynxException()
		{
			//mMsgStream.rdbuf()->freeze(0);
			//mDescStream.rdbuf()->freeze(0);
		}

  public:
		// Answer stream which should contain the one line error message
		ostringstream& Message() { return mMsgStream; }

		// Answer stream which should contain the multiple line description
		ostringstream& Description() { return mDescStream; }

  public:
		// Overload the assignment operator
		CLynxException& operator=(CLynxException& err)
		{
			mMsgStream.seekp(0);

			mMsgStream.write(err.Message().str().c_str(), int(err.Message().tellp()));
			//err.Message().rdbuf()->freeze(0);

			mDescStream.seekp(0);

			mDescStream.write(err.Description().str().c_str(), int(err.Description().tellp()));
			//err.Description().rdbuf()->freeze(0);

			return *this;
		}

		// Overload the I/O output operator
		friend ostream& operator<<(ostream& out, CLynxException& err)
		{
			out.write(err.Message().str().c_str(), int(err.Message().tellp()));
			//err.Message().rdbuf()->freeze(0);

			if(err.Description().tellp() != 0)
			{
				out << endl << endl;

				out.write(err.Description().str().c_str(), int(err.Description().tellp()));
				//err.Description().rdbuf()->freeze(0);
			}

			return out;
		}

  private:
		// Contains the one line error code message
		ostringstream mMsgStream;

		// Contains a multiple line description of the error and ways to
		// solve the problem
		ostringstream mDescStream;

  public:
		// CStrings to hold the data after its been //thrown

		char mMsg[MAX_ERROR_MSG];
		char mDesc[MAX_ERROR_DESC];
};
#endif
