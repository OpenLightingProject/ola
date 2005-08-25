
#define GET_WIDGET_PARAMS 3
#define GET_WIDGET_PARAMS_REPLY 3
#define SET_WIDGET_PARAMS 4
#define SET_DMX_RX_MODE 5
#define SET_DMX_TX_MODE 6
#define SEND_DMX_RDM_TX 7
#define RECEIVE_DMX_ON_CHANGE 8



typedef struct dmxusb
{
  int             x_comhandle;    /* holds the comport handle */
  struct termios  x_tty;          /* used for (re)storing original TTY attributes */
  char            x_device[256];  /* the device/path name as plain text */
  int             x_port;                       /* port number to open */
} t_dmxusb;




// send a packet, overlapped code has not been implemented yet
int SendData(t_dmxusb *x, int label, unsigned char *data, int length)
{
  unsigned short bytes_written = 0;

  unsigned char header[4];
  header[0] = 0x7E;
  header[1] = label;
  header[2] = length & 0xFF;
  header[3] = length >> 8;

  bytes_written = write(x->x_comhandle, (unsigned char *)header, 4);
  if (bytes_written != 4) return -1;

  bytes_written = write(x->x_comhandle, (unsigned char *)data, length);
  if (bytes_written != length) return -1;

  unsigned char end_code = 0xE7;
  bytes_written = write(x->x_comhandle, (unsigned char *)&end_code, 1);
  if (bytes_written != 1) return -1;

  return 0;
}


// ------------------------
// this is my function to position a light on pan/tilt values
//
void position(t_dmxusb *x, int channel, 
              unsigned short pan, unsigned short tilt) {
  unsigned char data[512+4] = "";;


  data[channel-1] = (pan & 0xFF00) >> 8;
  data[channel] = (tilt & 0xFF00) >> 8;
  data[channel+1] = (pan & 0xFF);
  data[channel+2] = (tilt & 0xFF);
  SendData(x, SEND_DMX_RDM_TX, data, channel-1+4 );

}


int recvframe(t_dmxusb *x, unsigned char *data, size_t datalen,
              int *error_occured)
{
  unsigned short length = 0;
  unsigned short bytes_read = 0;
  unsigned char byte = 0;

  *error_occured=0;             // reset
  bzero(data, datalen);

  // wait until we receive the good packet
  while (byte != 5)
    {
      // waiting to get a packet begin code
      while (byte != 0x7E)
        bytes_read = read(x->x_comhandle,(unsigned char *)&byte,1);
      if (bytes_read != 1) {
        fprintf(stderr, "got to much, expected start of packet\n");
        return -1;
      }
      if (byte != 0x7E) continue;

      // waiting to get the good message label
      bytes_read = read(x->x_comhandle,(unsigned char *)&byte,1);
      if (bytes_read != 1) {
        fprintf(stderr, "got to much, expected message label\n");
        return -1;
      }
    }

  // ok now we have the label, everything else was eaten up
  // next get the data length LSB
  bytes_read = read(x->x_comhandle,(unsigned char *)&byte,1);
  if (bytes_read != 1) { 
    fprintf(stderr, "got to much, expected data length LSB\n");
    return -1;
  }
  length = byte;
  // data length MSB
  bytes_read = read(x->x_comhandle,(unsigned char *)&byte,1);
  if (bytes_read != 1) {
    fprintf(stderr, "got to much, expected data length MSB\n");
    return -1;
  }
  length += byte<<8;

  if (length > datalen) {
    fprintf(stderr, "length > datalen of buffer\n");
    return -1;
  }

  bytes_read = read_multiplt_bytes(x->x_comhandle,(char *)data,length);
  if (bytes_read != length) {
    fprintf(stderr, "did not read enough or read to much. bytes_read=%d, "
            "expected %d\n", bytes_read, length);
    return -1;
  }
  *error_occured = data[0];

  while (byte != 0xE7) {
    bytes_read = read(x->x_comhandle,(unsigned char *)&byte,1);
    if (bytes_read != 1) {
      fprintf(stderr, "tried to read end of packet but got %d bytes\n", 
              bytes_read);
      return -1;
    }
    if (byte != 0xE7) {
      fprintf(stderr, "expected end of packet but got 0x%02x\n", byte);
      continue;
    }
  }

  return 0;
}


nt enttec_connect(t_dmxusb *x, char *bsdPath)
{
  int res = 0;
  char found_one_pro_unit = 0;
  int i;
  int size;


  fprintf(stderr, "\nEnttec Pro - C - Mac OSX - Receiver Test\n");
  fprintf(stderr, "\nScanning serial devices... ");

#ifdef MAC
  kern_return_t kernResult; // on PowerPC this is an int (4 bytes)
  io_iterator_t serialPortIterator;


  kernResult = FindModems(&serialPortIterator);
  kernResult = GetModemPath(serialPortIterator, bsdPath, strlen(bsdPath));


  while (bsdPath[0] != 0)
    {
      fprintf(stderr, "Testing port: %s\n",bsdPath);

      if (strncmp(bsdPath, "/dev/cu.usbserial-", 18) == 0)
        {
          fprintf(stderr, "First matching port: %s\n",bsdPath);
          fprintf(stderr, "Opening port...");

          res = dmxusb_open_devices(x, bsdPath);
          if (!res) {
            fprintf(stderr, "FAILED\n");
            return -1;
          }
          else
            fprintf(stderr, "OK\n");

          fprintf(stderr, "Sending GET_WIDGET_PARAMS packet... ");
          res = SendData(x, GET_WIDGET_PARAMS,(unsigned char *)&size,2);
          if (res)
            {
              fprintf(stderr, "FAILED\n");
              dmxusb_close_devices(x);
              return -1;
            }
          fprintf(stderr, "Done\n");

          fprintf(stderr, "Waiting for GET_WIDGET_PARAMS_REPLY packet... ");
          res=ReceiveData(x, GET_WIDGET_PARAMS_REPLY,(unsigned char *)&params_,sizeof(DMXUSBPROParamsType));
          if(res) {
            fprintf(stderr, "FAILED\n");
            dmxusb_close_devices(x);
            return -1;
          }
          fprintf(stderr, "Done\n");

          fprintf(stderr, "This device is a Enttec USB DMX Pro !\n\n");
          found_one_pro_unit = 1;
          break;
        }

      IOIteratorNext(serialPortIterator);
      kernResult = GetModemPath(serialPortIterator, bsdPath, strlen(bsdPath));
    }
#elif defined(LINUX)
  res = dmxusb_open_devices(x, bsdPath);
  if (!res) {
    fprintf(stderr, "FAILED\n");
    return -1;
  }
  else
    fprintf(stderr, "OK\n");

/*   fprintf(stderr, "Sending GET_WIDGET_PARAMS packet... "); */
/*   res = SendData(x, GET_WIDGET_PARAMS,(unsigned char *)&size,2); */
/*   if (res) */
/*     { */
/*       fprintf(stderr, "FAILED\n"); */
/*       dmxusb_close_devices(x); */
/*       return -1; */
/*     } */
/*   fprintf(stderr, "Done\n"); */
  
/*   fprintf(stderr, "Waiting for GET_WIDGET_PARAMS_REPLY packet... "); */
/*   res=ReceiveData(x, GET_WIDGET_PARAMS_REPLY,(unsigned char *)&params_,sizeof(DMXUSBPROParamsType)); */
/*   if(res) { */
/*     fprintf(stderr, "FAILED\n"); */
/*     dmxusb_close_devices(x); */
/*     return -1; */
/*   } */
/*   fprintf(stderr, "Done\n"); */
  
  fprintf(stderr, "This device is a Enttec USB DMX Pro !\n\n");
  found_one_pro_unit = 1;  
#endif

#ifdef MAC
  IOObjectRelease(serialPortIterator);  // Release the iterator.
#endif

  if (!found_one_pro_unit)
    {
      fprintf(stderr, "No Enttec USB DMX Pro found... exiting\n\n");
      return -1;
    }
#ifdef SEND_ON_CHANGE_ONLY
  unsigned char send_on_change_flag = 0;
  fprintf(stderr, "Set the widget to send DMX only when signal change... ");
  res = SendData(x, RECEIVE_DMX_ON_CHANGE,&send_on_change_flag,1);
  if (res)
    {
      fprintf(stderr, "FAILED\n");
      dmxusb_close_devices(x);
      return -1;
    }
  fprintf(stderr, "Done\n");
#endif
  return 0;
}


// ----------------------------------------
// this is how i can test receiving packets

int main(int argc, char **argv) {
  t_dmxusb x;
  unsigned char buf[600];
  unsigned char oldbuf[600];
  int j;
  int rc;
  int had_error;

  enttec_connect(&x, "/dev/ttyUSB0");
  while (1) {
    rc = recvframe(&x, buf, sizeof(buf), &had_error);
    if (-1 == rc)
      return -1;
    for (j=0; j < sizeof(buf); j++) {
      if (oldbuf[j] != buf[j]) {
        fprintf(stdout, "channel %d -> val %d\n", j, buf[j]);
      }
    }
    memcpy(oldbuf, buf, sizeof(buf));

  }
