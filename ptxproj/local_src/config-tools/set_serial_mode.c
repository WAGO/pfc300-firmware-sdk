//------------------------------------------------------------------------------
/// Copyright (c) WAGO GmbH & Co. KG
///
/// PROPRIETARY RIGHTS are involved in the subject matter of this material.
/// All manufacturing, reproduction, use and sales rights pertaining to this
/// subject matter are governed by the license agreement. The recipient of this
/// software implicitly accepts the terms of the license.
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
///
///  \file     set_serial_mode.c
///
///  \version  $Revision$
///
///  \brief    This config tool sets the PFCXXX transceiver to RS232 or RS485
///           Based on tty0modeswitch
///
///  \author   WAGO GmbH & Co. KG
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Include files
//------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <linux/serial.h>
#include <termios.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <ct_error_handling.h>

#define TTY "/dev/serial"
#define RS_SEL_PATH "/sys/class/leds/rs-sel/brightness"
#define BOARD_PATH "/sys/class/wago/system/board_variant"

int ReadBoardVariant(char * pszOut, int iMaxLen)
{
  int fd;
  int iRet = -1;
  fd = open((const char *)BOARD_PATH, O_RDONLY);
  if (fd >= 0)
  {
    if (read(fd, pszOut, iMaxLen) > 0)
    {
      iRet = 0;
    }
    close(fd);
  }
  return iRet;
}

void usage(const char *bin)
{
	printf("Usage: %s <mode>\n", bin);
	printf("Switch mode of onboard serial interface %s\n\n", TTY);
 	printf("Available modes:\n");
 	printf("	rs232		Set %s to: 'RS232'\n", TTY);
 	printf("	rs485		Set %s to: 'RS485'\n", TTY);
}

int main(int argc, char **argv)
{
    int ret = SUCCESS;

    const char *serial = TTY;
	int serialFd;
	struct serial_rs485 srs;	// Mode selection (RS232 or RS485)
	struct termios tp;			// Communication params
	struct stat StatInfo;		// file infos provided by stat()
	int iRSSelFound, iValue;	// RS_SEL used by TP600, EC, CC100 V2
	FILE *pFile;
	char szFileMode[5];
	strcpy(szFileMode, "rb");
	char szOut[256] = "";
  
	if(argc != 2)
	{
        usage(argv[0]);
	    exit(INVALID_PARAMETER);
    }

    if(    (0 == strcmp("-h", argv[1]))
        || (0 == strcmp("--help", argv[1])) )
    {
        usage(argv[0]);
        exit(SUCCESS);
    }

    // detect RS_SEL method (only TP600, EC, CC100 V2)
    if(stat(RS_SEL_PATH, &StatInfo) == 0)
        iRSSelFound = 1;
    else
        iRSSelFound = 0;

    // detect CC100 device, only rs485 available no switching possible
    if (ReadBoardVariant(&szOut[0], sizeof(szOut)) == 0)
    {
      if (strncmp(szOut, "CC100", 5) == 0)
      {
        if (iRSSelFound == 0)
        {
            //CC100 751-9301 751-9401 with fix RS485 on Com1
            if(strcmp(argv[1], "rs485") == 0)
            {
              return 0; //success
            }
            if (strcmp(argv[1], "rs232") == 0)
            {
              //perror("ERROR: rs232 is not available ");
              return -1; //not available
            }
        }
      }
    }
    szOut[0] = '\0';

    if(SUCCESS == ret)
    {
	    serialFd = open(serial, O_RDWR);
    	if(serialFd < 0)
    	{
	    	perror("ERROR: can't open serial interface ");
    	    ret = FILE_OPEN_ERROR;
        }
    }

    if(SUCCESS == ret)
    {
        if (tcgetattr(serialFd, &tp) < 0)
    	{
      		perror("ERROR: tcgetattr() - Can't read settings");
            ret = SYSTEM_CALL_ERROR;
      }
      if( (iRSSelFound == 0 )
        &&(ioctl(serialFd, TIOCGRS485, &srs) < 0))
      {
        perror("ERROR: ioctl(TIOCGRS485)  - Can't read settings");
        ret = SYSTEM_CALL_ERROR;
      }
    }

    if(SUCCESS == ret)
    {
        if( !strcmp(argv[1], "rs485"))
    	{
            /* Enable RS485 mode */
        	srs.flags = SER_RS485_ENABLED | SER_RS485_RTS_ON_SEND;
		    tp.c_iflag |= IGNBRK;  	//Ignore break condition(required by RS485?)
		    tp.c_cflag |= CREAD;   //it was detected by coincidence that this bit need to be set 
			/* vtpctp, EC, CC100 V2 enable rs485 */
			if ( iRSSelFound == 1 ) {
				strcpy(szFileMode, "rb+");
				pFile = fopen(RS_SEL_PATH, szFileMode);
				if (pFile != NULL)
				{
					fseek((FILE*)pFile, 0, SEEK_SET);
					iValue = 0;
					sprintf(&szOut[0], "%d", iValue);
					if (fwrite (&szOut[0] , sizeof(unsigned char), sizeof(szOut), (FILE*)pFile) <= 0) {
						perror("ERROR: RS_SEL can't change serial mode ");
						ret = SYSTEM_CALL_ERROR;
					}
					fclose((FILE *)pFile);
				}
			}
        }
        else if( !strcmp(argv[1], "rs232"))
        {
		    tp.c_iflag &= ~IGNBRK;
		    tp.c_cflag &= ~CREAD;
		    srs.flags &= ~(SER_RS485_ENABLED | SER_RS485_RTS_ON_SEND);
			/* vtpctp, EC, CC100 V2 disable rs485 */
			if ( iRSSelFound == 1 ) {
				strcpy(szFileMode, "rb+");
				pFile = fopen(RS_SEL_PATH, szFileMode);
				if (pFile != NULL)
				{
					fseek((FILE*)pFile, 0, SEEK_SET);
					iValue = 255;
					sprintf(&szOut[0], "%d", iValue);
					if (fwrite (&szOut[0] , sizeof(unsigned char), sizeof(szOut), (FILE*)pFile) <= 0) {
						perror("ERROR: RS_SEL can't change serial mode ");
						ret = SYSTEM_CALL_ERROR;
					}
					fclose((FILE *)pFile);
				}
			}
        }
        else
        {
            usage(argv[0]);
            ret = INVALID_PARAMETER;
        }
    }

	/* vtpctp, EC, CC100 V2 ioctl not implemented */
	if ( iRSSelFound == 0 ) {
	    if(SUCCESS == ret)
	    {
		    if(ioctl(serialFd, TIOCSRS485, &srs) < 0)
	        {
	    	perror("ERROR: can't change serial mode ");
	            ret = SYSTEM_CALL_ERROR;
	        }
	    }
	}

    if(SUCCESS == ret)
    {
        tcflush(serialFd, TCIOFLUSH); // Flush buffers

        if (tcsetattr(serialFd, TCSANOW, &tp) < 0) // Set attributes
    	{
      		perror("ERROR: tcsetattr() - Can't write settings");
            ret = SYSTEM_CALL_ERROR;
        }
    }

    if(serialFd != -1)
    {
        close(serialFd);
    }

    return ret;

}
