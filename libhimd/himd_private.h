static unsigned int beword16(unsigned char * c)
{
    return c[0]*256+c[1];
}

static unsigned int beword32(unsigned char * c)
{
    return c[0]*16777216+c[1]*65536+c[2]*256+c[3];
}

void set_status_const(struct himderrinfo * status, enum himdstatus code, const char * msg);
void set_status_printf(struct himderrinfo * status, enum himdstatus code, const char * format, ...);

int descrypt_open(void ** dataptr, struct himderrinfo * status);
int descrypt_decrypt(void * dataptr, unsigned char * block, size_t cryptlen, struct himderrinfo * status);
void descrypt_close(void * dataptr);
