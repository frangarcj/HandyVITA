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
// System object class                                                      //
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// This class provides the glue to bind of of the emulation objects         //
// together via peek/poke handlers and pass thru interfaces to lower        //
// objects, all control of the emulator is done via this class. Update()    //
// does most of the work and each call emulates one CPU instruction and     //
// updates all of the relevant hardware if required. It must be remembered  //
// that if that instruction involves setting SPRGO then, it will cause a    //
// sprite painting operation and then a corresponding update of all of the  //
// hardware which will usually involve recursive calls to Update, see       //
// Mikey SPRGO code for more details.                                       //
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

#define SYSTEM_CPP

//#include <crtdbg.h>
//#define	TRACE_SYSTEM

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "system.h"
#include <psp2/types.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
//#include "error.h"
//#include "zlib.h"

void lynx_decrypt(unsigned char * result, const unsigned char * encrypted, const int length);

int lss_read(void* dest,int varsize, int varcount,LSS_FILE *fp)
{
	ULONG copysize;
	copysize=varsize*varcount;
	if((fp->index + copysize) > fp->index_limit) copysize=fp->index_limit - fp->index;
	memcpy(dest,fp->memptr+fp->index,copysize);
	fp->index+=copysize;
	return copysize;
}

 void _splitpath(const char* path, char* drv, char* dir, char* name, char* ext)
 {
     const char* end; /* end of processed string */
     const char* p;   /* search pointer */
     const char* s;   /* copy pointer */

     /* extract drive name */
     if (path[0] && path[1]==':') {
         if (drv) {
             *drv++ = *path++;
             *drv++ = *path++;
             *drv = '\0';
         }
     } else if (drv)
         *drv = '\0';

    /* search for end of string or stream separator */
     for(end=path; *end && *end!=':'; )
         end++;

     /* search for begin of file extension */
     for(p=end; p>path && *--p!='\\' && *p!='/'; )
         if (*p == '.') {
             end = p;
             break;
         }

     if (ext)
         for(s=end; (*ext=*s++); )
             ext++;

     /* search for end of directory name */
     for(p=end; p>path; )
         if (*--p=='\\' || *p=='/') {
             p++;
             break;
         }

     if (name) {
         for(s=p; s<end; )
             *name++ = *s++;

         *name = '\0';
     }

     if (dir) {
         for(s=path; s<p; )
             *dir++ = *s++;

         *dir = '\0';
     }
}

CSystem::CSystem(const char* gamefile, const char* romfile)
	:mCart(NULL),
	mRom(NULL),
	mMemMap(NULL),
	mRam(NULL),
	mCpu(NULL),
	mMikie(NULL),
	mSusie(NULL)
{


  printf("inside");

#ifdef _LYNXDBG
	mpDebugCallback=NULL;
	mDebugCallbackObject=0;
#endif

	// Select the default filetype
	UBYTE *filememory=NULL;
	UBYTE *howardmemory=NULL;
	ULONG filesize=0;
	ULONG howardsize=0;

	mFileType=HANDY_FILETYPE_LNX;
	if(strcmp(gamefile,"")==0)
	{
		// No file
		filesize=0;
		filememory=NULL;
	}
	else
	{
		// Open the file and load the file
		SceUID fp;

		// Open the cartridge file for reading
		if((fp=sceIoOpen(gamefile,PSP2_O_RDONLY,0777))==NULL)
		{
			printf( "Invalid Cart.\n");
		}
		else
		{
			printf("fopen %s",fp);
		}

		// How big is the file ??
		filesize=sceIoLseek(fp,0,PSP2_SEEK_END);
		sceIoLseek(fp,0,PSP2_SEEK_SET);
		filememory=(UBYTE*) new UBYTE[filesize];

		if(sceIoRead(fp,filememory,filesize)!=filesize)
		{
			printf( "Invalid Cart.\n");
			delete filememory;
		}

		printf("LEIDOOOOO");

		sceIoClose(fp);
	}

	// Now try and determine the filetype we have opened
	if(filesize)
	{
		char clip[11];
		memcpy(clip,filememory,11);
		clip[4]=0;
		clip[10]=0;

		if(!strcmp(&clip[6],"BS93")) mFileType=HANDY_FILETYPE_HOMEBREW;
		else if(!strcmp(&clip[0],"LYNX")) mFileType=HANDY_FILETYPE_LNX;
		else if(!strcmp(&clip[0],LSS_VERSION_OLD)) mFileType=HANDY_FILETYPE_SNAPSHOT;
		else
		{
			printf( "Invalid Cart.\n");
			delete filememory;
		}
	}

	mCycleCountBreakpoint=0xffffffff;

// Create the system objects that we'll use

	// Attempt to load the cartridge errors caught above here...

	mRom = new CRom(romfile);

	// An exception from this will be caught by the level above

	switch(mFileType)
	{
		case HANDY_FILETYPE_LNX:
			mCart = new CCart(filememory,filesize);
			if(mCart->CartHeaderLess())
			{
				FILE	*fp;
				char drive[3],dir[256],cartgo[256];
				mFileType=HANDY_FILETYPE_HOMEBREW;
				_splitpath(romfile,drive,dir,NULL,NULL);
				strcpy(cartgo,drive);
				strcat(cartgo,dir);
				strcat(cartgo,"howard.o");

				// Open the howard file for reading
				if((fp=fopen(cartgo,"rb"))==NULL)
				{
                    printf( "Invalid Cart.\n");
                    delete filememory;
				}

				// How big is the file ??
				fseek(fp,0,SEEK_END);
				howardsize=ftell(fp);
				fseek(fp,0,SEEK_SET);
				howardmemory=(UBYTE*) new UBYTE[filesize];

				if(fread(howardmemory,sizeof(char),howardsize,fp)!=howardsize)
				{
					delete howardmemory;
                    printf( "Invalid Cart.\n");
                    delete filememory;
				}

				fclose(fp);

				// Pass it to RAM to load
				mRam = new CRam(howardmemory,howardsize);
			}
			else
			{
				mRam = new CRam(0,0);
			}
			break;
		case HANDY_FILETYPE_HOMEBREW:
			mCart = new CCart(0,0);
			mRam = new CRam(filememory,filesize);
			break;
		case HANDY_FILETYPE_SNAPSHOT:
		case HANDY_FILETYPE_ILLEGAL:
		default:
			mCart = new CCart(0,0);
			mRam = new CRam(0,0);
			break;
	}

	// These can generate exceptions

	mMikie = new CMikie(*this);
	mSusie = new CSusie(*this);

// Instantiate the memory map handler

	mMemMap = new CMemMap(*this);

// Now the handlers are set we can instantiate the CPU as is will use handlers on reset

	mCpu = new C65C02(*this);

// Now init is complete do a reset, this will cause many things to be reset twice
// but what the hell, who cares, I don't.....

	Reset();

// If this is a snapshot type then restore the context

	if(mFileType==HANDY_FILETYPE_SNAPSHOT)
	{
		if(!ContextLoad(gamefile))
		{
			Reset();
			printf( "Invalid Snapshot.\n");
		}
	}
	if(filesize) delete filememory;
	if(howardsize) delete howardmemory;
}

CSystem::~CSystem()
{
	// Cleanup all our objects

	if(mCart!=NULL) delete mCart;
	if(mRom!=NULL) delete mRom;
	if(mRam!=NULL) delete mRam;
	if(mCpu!=NULL) delete mCpu;
	if(mMikie!=NULL) delete mMikie;
	if(mSusie!=NULL) delete mSusie;
	if(mMemMap!=NULL) delete mMemMap;
}

bool CSystem::IsZip(char *filename)
{
	UBYTE buf[2];
	FILE *fp;

	if((fp=fopen(filename,"rb"))!=NULL)
	{
		fread(buf, 2, 1, fp);
		fclose(fp);
		return(memcmp(buf,"PK",2)==0);
	}
	if(fp)fclose(fp);
	return FALSE;
}

void CSystem::HLE_BIOS_init()
{
	printf( "[handy] HLE_BIOS_Init\r\n");

	unsigned char buffer[8];

	int blocksize = (1 + mCart->mMaskBank0) / 256;

	int blockcount = 0x100 - mCart->Peek(0);
	int start = 1 + blockcount * 51;

	int loadaddr;

	if(blockcount == 1) // cc65
	{
        unsigned char buff[100];
		unsigned char res[100];

		for (int i = 0; i < 52; ++i) // first encrypted loader
		{
			buff[i] = mCart->Peek0();
		}

		lynx_decrypt(res, buff, 51);

		for (int i = 0; i < 100; ++i)
		{
			Poke_CPU(0x200 + i, res[i]);
		}

		loadaddr = 0x200;
	}
	else // epyx
	{
		blockcount = 0x100 - mCart->Peek(start);
		start += 1 + blockcount * 51;

		int dir_idx = 1;

		for (int i = 0; i < 8; ++i) //exec directory
		{
			buffer[i] = mCart->Peek(start + (dir_idx * 8) + i);
		}

		int offset = ((int)buffer[0]) * blocksize + (((int)buffer[2]) << 8) + buffer[1];

		loadaddr = (((int)buffer[5]) << 8) + buffer[4];
		int filesize = (((int)buffer[7]) << 8) + buffer[6];

		for (int i = 0; i < filesize; ++i)
		{
			Poke_CPU(i + loadaddr,  mCart->Peek(offset + i));
		}
	}

	C6502_REGS regs;
	mCpu->GetRegs(regs);
	regs.PC=(UWORD)loadaddr;
	mCpu->SetRegs(regs);
}

void CSystem::Reset(void)
{
	gSystemCycleCount=0;
	gNextTimerEvent=0;
	gCPUBootAddress=0;
	gBreakpointHit=FALSE;
	gSingleStepMode=FALSE;
	gSingleStepModeSprites=FALSE;
	gSystemIRQ=FALSE;
	gSystemNMI=FALSE;
	gSystemCPUSleep=FALSE;
	gSystemHalt=FALSE;

	gThrottleLastTimerCount=0;
	gThrottleNextCycleCheckpoint=0;

	gTimerCount=0;

	gAudioBufferPointer=0;
	gAudioLastUpdateCycle=0;
	memset(gAudioBuffer,128,HANDY_AUDIO_BUFFER_SIZE);

#ifdef _LYNXDBG
	gSystemHalt=TRUE;
#endif

	mMemMap->Reset();
	mCart->Reset();
	mRom->Reset();
	mRam->Reset();
	mMikie->Reset();
	mSusie->Reset();
	mCpu->Reset();

	if(!mRom->mValid)
	{
		HLE_BIOS_init();
	}

	// Homebrew hashup

	if(mFileType==HANDY_FILETYPE_HOMEBREW)
	{
		mMikie->PresetForHomebrew();

		C6502_REGS regs;
		mCpu->GetRegs(regs);
		regs.PC=(UWORD)gCPUBootAddress;
		mCpu->SetRegs(regs);
	}
}

bool CSystem::ContextSave(const char *context)
{
	FILE *fp;
	bool status=1;

	if((fp=fopen(context,"wb"))==NULL) return false;

	if(!fprintf(fp,LSS_VERSION)) status=0;

	// Save ROM CRC
	ULONG checksum=mCart->CRC32();
	if(!fwrite(&checksum,sizeof(ULONG),1,fp)) status=0;

	if(!fprintf(fp,"CSystem::ContextSave")) status=0;

	if(!fwrite(&mCycleCountBreakpoint,sizeof(ULONG),1,fp)) status=0;
	if(!fwrite(&gSystemCycleCount,sizeof(ULONG),1,fp)) status=0;
	if(!fwrite(&gNextTimerEvent,sizeof(ULONG),1,fp)) status=0;
	if(!fwrite(&gCPUWakeupTime,sizeof(ULONG),1,fp)) status=0;
	if(!fwrite(&gCPUBootAddress,sizeof(ULONG),1,fp)) status=0;
	if(!fwrite(&gIRQEntryCycle,sizeof(ULONG),1,fp)) status=0;
	if(!fwrite(&gBreakpointHit,sizeof(ULONG),1,fp)) status=0;
	if(!fwrite(&gSingleStepMode,sizeof(ULONG),1,fp)) status=0;
	if(!fwrite(&gSystemIRQ,sizeof(ULONG),1,fp)) status=0;
	if(!fwrite(&gSystemNMI,sizeof(ULONG),1,fp)) status=0;
	if(!fwrite(&gSystemCPUSleep,sizeof(ULONG),1,fp)) status=0;
	if(!fwrite(&gSystemCPUSleep_Saved,sizeof(ULONG),1,fp)) status=0;
	if(!fwrite(&gSystemHalt,sizeof(ULONG),1,fp)) status=0;
	if(!fwrite(&gThrottleMaxPercentage,sizeof(ULONG),1,fp)) status=0;
	if(!fwrite(&gThrottleLastTimerCount,sizeof(ULONG),1,fp)) status=0;
	if(!fwrite(&gThrottleNextCycleCheckpoint,sizeof(ULONG),1,fp)) status=0;

	ULONG tmp=gTimerCount;
	if(!fwrite(&tmp,sizeof(ULONG),1,fp)) status=0;

	if(!fwrite(gAudioBuffer,sizeof(UBYTE),HANDY_AUDIO_BUFFER_SIZE,fp)) status=0;
	if(!fwrite(&gAudioBufferPointer,sizeof(ULONG),1,fp)) status=0;
	if(!fwrite(&gAudioLastUpdateCycle,sizeof(ULONG),1,fp)) status=0;

	// Save other device contexts
	if(!mMemMap->ContextSave(fp)) status=0;
	if(!mCart->ContextSave(fp)) status=0;
//	if(!mRom->ContextSave(fp)) status=0; We no longer save the system ROM
	if(!mRam->ContextSave(fp)) status=0;
	if(!mMikie->ContextSave(fp)) status=0;
	if(!mSusie->ContextSave(fp)) status=0;
	if(!mCpu->ContextSave(fp)) status=0;

	fclose(fp);
	return status;
}

size_t	CSystem::MemoryContextSave(const char* tmpfilename, char *context)
{
    size_t ret = 0;

    if(!ContextSave(tmpfilename))
    {
        return 0;
    }

    FILE *fp;
    UBYTE *filememory = NULL;
	ULONG filesize = 0;

    if((fp = fopen(tmpfilename,"rb")) == NULL)
    {
        return 0;
    }

    fseek(fp,0,SEEK_END);
    filesize = ftell(fp);
    fseek(fp,0,SEEK_SET);

    if(NULL == context)
    {
        filememory = (UBYTE*) new UBYTE[filesize];
    }
    else
    {
        filememory = (UBYTE*) context;
    }

    if(fread(filememory,sizeof(char),filesize,fp) == filesize)
    {
        ret = filesize;
    }

    fclose(fp);

    if(NULL == context)
    {
        delete filememory;
    }

    remove(tmpfilename);

    return ret;
}

bool CSystem::MemoryContextLoad(const char *context, size_t size)
{
    LSS_FILE *fp;
	bool status=1;
	UBYTE *filememory=(UBYTE*)context;
	ULONG filesize=size;

	// Setup our read structure
	fp = new LSS_FILE;
	fp->memptr=filememory;
	fp->index=0;
	fp->index_limit=filesize;

	char teststr[100];
	// Check identifier
	if(!lss_read(teststr,sizeof(char),4,fp)) status=0;
	teststr[4]=0;

	if(strcmp(teststr,LSS_VERSION)==0 || strcmp(teststr,LSS_VERSION_OLD)==0)
	{
		bool legacy=FALSE;
		if(strcmp(teststr,LSS_VERSION_OLD)==0)
		{
			legacy=TRUE;
		}
		else
		{
			ULONG checksum;
			// Read CRC32 and check against the CART for a match
			lss_read(&checksum,sizeof(ULONG),1,fp);
			if(mCart->CRC32()!=checksum)
			{
				delete fp;
				printf( "[handy]LSS Snapshot CRC does not match the loaded cartridge image, aborting load.\n");
				return 0;
			}
		}

		// Check our block header
		if(!lss_read(teststr,sizeof(char),20,fp)) status=0;
		teststr[20]=0;
		if(strcmp(teststr,"CSystem::ContextSave")!=0) status=0;

		if(!lss_read(&mCycleCountBreakpoint,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gSystemCycleCount,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gNextTimerEvent,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gCPUWakeupTime,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gCPUBootAddress,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gIRQEntryCycle,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gBreakpointHit,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gSingleStepMode,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gSystemIRQ,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gSystemNMI,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gSystemCPUSleep,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gSystemCPUSleep_Saved,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gSystemHalt,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gThrottleMaxPercentage,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gThrottleLastTimerCount,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gThrottleNextCycleCheckpoint,sizeof(ULONG),1,fp)) status=0;

		ULONG tmp;
		if(!lss_read(&tmp,sizeof(ULONG),1,fp)) status=0;
		gTimerCount=tmp;

		if(!lss_read(gAudioBuffer,sizeof(UBYTE),HANDY_AUDIO_BUFFER_SIZE,fp)) status=0;
		if(!lss_read(&gAudioBufferPointer,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gAudioLastUpdateCycle,sizeof(ULONG),1,fp)) status=0;

		if(!mMemMap->ContextLoad(fp)) status=0;
		// Legacy support
		if(legacy)
		{
			if(!mCart->ContextLoadLegacy(fp)) status=0;
			if(!mRom->ContextLoad(fp)) status=0;
		}
		else
		{
			if(!mCart->ContextLoad(fp)) status=0;
		}
		if(!mRam->ContextLoad(fp)) status=0;
		if(!mMikie->ContextLoad(fp)) status=0;
		if(!mSusie->ContextLoad(fp)) status=0;
		if(!mCpu->ContextLoad(fp)) status=0;
	}
	else
	{
		printf( "[handy]Not a recognised LSS file\n");
	}

	delete fp;

	return status;
}

bool CSystem::ContextLoad(const char *context)
{
	LSS_FILE *fp;
	bool status=1;
	UBYTE *filememory=NULL;
	ULONG filesize=0;

	{
		FILE *fp;
		// Just open an read into memory
		if((fp=fopen(context,"rb"))==NULL) status=0;

		fseek(fp,0,SEEK_END);
		filesize=ftell(fp);
		fseek(fp,0,SEEK_SET);
		filememory=(UBYTE*) new UBYTE[filesize];

		if(fread(filememory,sizeof(char),filesize,fp)!=filesize)
		{
			fclose(fp);
			return 1;
		}
		fclose(fp);
	}

	// Setup our read structure
	fp = new LSS_FILE;
	fp->memptr=filememory;
	fp->index=0;
	fp->index_limit=filesize;

	char teststr[100];
	// Check identifier
	if(!lss_read(teststr,sizeof(char),4,fp)) status=0;
	teststr[4]=0;

	if(strcmp(teststr,LSS_VERSION)==0 || strcmp(teststr,LSS_VERSION_OLD)==0)
	{
		bool legacy=FALSE;
		if(strcmp(teststr,LSS_VERSION_OLD)==0)
		{
			legacy=TRUE;
		}
		else
		{
			ULONG checksum;
			// Read CRC32 and check against the CART for a match
			lss_read(&checksum,sizeof(ULONG),1,fp);
			if(mCart->CRC32()!=checksum)
			{
				delete fp;
				delete filememory;
				printf( "[handy]LSS Snapshot CRC does not match the loaded cartridge image, aborting load.\n");
				return 0;
			}
		}

		// Check our block header
		if(!lss_read(teststr,sizeof(char),20,fp)) status=0;
		teststr[20]=0;
		if(strcmp(teststr,"CSystem::ContextSave")!=0) status=0;

		if(!lss_read(&mCycleCountBreakpoint,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gSystemCycleCount,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gNextTimerEvent,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gCPUWakeupTime,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gCPUBootAddress,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gIRQEntryCycle,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gBreakpointHit,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gSingleStepMode,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gSystemIRQ,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gSystemNMI,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gSystemCPUSleep,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gSystemCPUSleep_Saved,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gSystemHalt,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gThrottleMaxPercentage,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gThrottleLastTimerCount,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gThrottleNextCycleCheckpoint,sizeof(ULONG),1,fp)) status=0;

		ULONG tmp;
		if(!lss_read(&tmp,sizeof(ULONG),1,fp)) status=0;
		gTimerCount=tmp;

		if(!lss_read(gAudioBuffer,sizeof(UBYTE),HANDY_AUDIO_BUFFER_SIZE,fp)) status=0;
		if(!lss_read(&gAudioBufferPointer,sizeof(ULONG),1,fp)) status=0;
		if(!lss_read(&gAudioLastUpdateCycle,sizeof(ULONG),1,fp)) status=0;

		if(!mMemMap->ContextLoad(fp)) status=0;
		// Legacy support
		if(legacy)
		{
			if(!mCart->ContextLoadLegacy(fp)) status=0;
			if(!mRom->ContextLoad(fp)) status=0;
		}
		else
		{
			if(!mCart->ContextLoad(fp)) status=0;
		}
		if(!mRam->ContextLoad(fp)) status=0;
		if(!mMikie->ContextLoad(fp)) status=0;
		if(!mSusie->ContextLoad(fp)) status=0;
		if(!mCpu->ContextLoad(fp)) status=0;
	}
	else
	{
		gError->Warning("Not a recognised LSS file");
	}

	delete fp;
	delete filememory;

	return status;
}

#ifdef _LYNXDBG

void CSystem::DebugTrace(int address)
{
	char message[1024+1];
	int count=0;

	sprintf(message,"%08x - DebugTrace(): ",gSystemCycleCount);
	count=strlen(message);

	if(address)
	{
		if(address==0xffff)
		{
			C6502_REGS regs;
			char linetext[1024];
			// Register dump
			GetRegs(regs);
			sprintf(linetext,"PC=$%04x SP=$%02x PS=0x%02x A=0x%02x X=0x%02x Y=0x%02x",regs.PC,regs.SP, regs.PS,regs.A,regs.X,regs.Y);
			strcat(message,linetext);
			count=strlen(message);
		}
		else
		{
			// The RAM address contents should be dumped to an open debug file in this function
			do
			{
				message[count++]=Peek_RAM(address);
			}
			while(count<1024 && Peek_RAM(address++)!=0);
		}
	}
	else
	{
		strcat(message,"CPU Breakpoint");
		count=strlen(message);
	}
	message[count]=0;

	// Callback to dump the message
	if(mpDebugCallback)
	{
		(*mpDebugCallback)(mDebugCallbackObject,message);
	}
}

void CSystem::DebugSetCallback(void (*function)(ULONG objref,char *message),ULONG objref)
{
	mDebugCallbackObject=objref;
	mpDebugCallback=function;
}


#endif
