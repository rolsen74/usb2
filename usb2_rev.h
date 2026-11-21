#define VERSION	 1
#define REVISION 23
#define DATE	 "21.11.2026"
#define VERS	 "usb2.kmod 1.23"
#ifdef SIS163U_RX_TRACE
#define VSTRING	 VERS " SiS163U RX trace (" DATE ")\r\n"
#define VERSTAG	 "\0$VER: " VERS " SiS163U RX trace (" DATE ")"
#else
#define VSTRING	 VERS " (" DATE ")\r\n"
#define VERSTAG	 "\0$VER: " VERS " (" DATE ")"
#endif
