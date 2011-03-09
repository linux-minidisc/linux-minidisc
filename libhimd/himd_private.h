static inline unsigned int beword16(const unsigned char * c)
{
    return c[0]*256+c[1];
}

static inline unsigned int beword32(const unsigned char * c)
{
    return c[0]*16777216+c[1]*65536+c[2]*256+c[3];
}


static inline void setbeword16(unsigned char * c, unsigned int val)
{
    c[0] = val >> 8;
    c[1] = val & 0xFF;
}

static inline void setbeword32(unsigned char * c, unsigned int val)
{
    c[0] = (val >> 24) & 0xFF;
    c[1] = (val >> 16) & 0xFF;
    c[2] = (val >> 8)  & 0xFF;
    c[3] =  val        & 0xFF;
}

void set_status_const(struct himderrinfo * status, enum himdstatus code, const char * msg);
void set_status_printf(struct himderrinfo * status, enum himdstatus code, const char * format, ...);

int descrypt_open(void ** dataptr, const unsigned char * trackkey, 
                  unsigned int ekbnum, struct himderrinfo * status);
int descrypt_decrypt(void * dataptr, unsigned char * block, size_t cryptlen,
                     const unsigned char * fragkey, struct himderrinfo * status);
void descrypt_close(void * dataptr);
