int lte_sdio_tty_lldm_open(struct tty_struct *tty_lldm, struct file *filep);
int lte_sdio_tty_lldm_close(struct tty_struct *tty_lldm, struct file *filep);
ssize_t lte_sdio_tty_lldm_write(struct tty_struct *tty_lldm, const unsigned char *buf, int count);


ssize_t lte_sdio_tty_lldm_write_room(struct tty_struct *tty_lldm);
int lte_sdio_tty_lldm_chars_in_buffer(struct tty_struct *tty_lldm);
void lte_sdio_tty_lldm_send_xchar(struct tty_struct *tty_lldm, char ch);
void lte_sdio_tty_lldm_throttle(struct tty_struct *tty_lldm);
void lte_sdio_tty_lldm_unthrottle(struct tty_struct *tty_lldm);
void lte_sdio_tty_lldm_set_termios(struct tty_struct *tty_lldm, struct ktermios *old_termios);
int lte_sdio_tty_lldm_break_ctl(struct tty_struct *tty_lldm, int break_state);
int lte_sdio_tty_lldm_tiocmget(struct tty_struct *tty_lldm, struct file *file);
int lte_sdio_tty_lldm_tiocmset(struct tty_struct *tty_lldm, struct file *file,unsigned int set, unsigned int clear);
int lte_sdio_tty_lldm_ioctl(struct tty_struct *tty_lldm, struct file *file,unsigned int cmd, unsigned long arg);
int lte_sdio_tty_lldm_init(void);
int lte_sdio_tty_lldm_deinit(void);